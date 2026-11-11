#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <semaphore.h>
#include <fcntl.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/os_api.h"

#define BUFFER_SIZE 4096

typedef struct {
    sem_t empty;
    sem_t full;
    size_t length;
    int is_last;
    int finished;
    char data[BUFFER_SIZE];
} shared_buffer;

struct os_shared_memory{
    int fd;
    char *name;
    shared_buffer *buffer;
    int owner;
    int writer_closed;
};

struct os_process{
    pid_t pid;
};

static int wait_semaphore(sem_t *sem){
    int result;
    do {
        result = sem_wait(sem);
    } while (result == -1 && errno == EINTR);
    return result;
}

int os_shm_create(os_shared_memory **memory){
    if (memory == NULL) {
        return -1;
    }
    *memory = NULL;
    os_shared_memory *new_memory = calloc(1, sizeof(os_shared_memory));
    if (new_memory == NULL) {
        return -1;
    }
    new_memory->fd = -1;
    static unsigned long counter = 0;
    char name[64];
    for (int attempt = 0; attempt < 100; attempt++) {
        int n = snprintf(name, sizeof(name), "/lab4_%ld_%lu", (long)getpid(), counter++);
        if (n < 0 || (size_t)n >= sizeof(name)) {
            free(new_memory);
            return -1;
        }
        new_memory->fd = shm_open(name, O_CREAT | O_EXCL | O_RDWR, 0600);
        if (new_memory->fd != -1) {
            break;
        }
        if (errno != EEXIST) {
            perror("shm_open");
            free(new_memory);
            return -1;
        }
    }
    if (new_memory->fd == -1) {
        free(new_memory);
        return -1;
    }
    new_memory->name = strdup(name);
    
    if (new_memory->name == NULL) {
        close(new_memory->fd);
        shm_unlink(name);
        free(new_memory);
        return -1;
    }
    if (ftruncate(new_memory->fd, sizeof(shared_buffer)) == -1) {
        perror("ftruncate");
        close(new_memory->fd);
        shm_unlink(new_memory->name);
        free(new_memory->name);
        free(new_memory);
        return -1;
    }
    void *mapping = mmap(NULL, sizeof(shared_buffer), PROT_READ | PROT_WRITE, MAP_SHARED, new_memory->fd, 0);
    if (mapping == MAP_FAILED) {
        perror("mmap");
        close(new_memory->fd);
        shm_unlink(new_memory->name);
        free(new_memory->name);
        free(new_memory);
        return -1;
    }
    new_memory->buffer = mapping;
    if (sem_init(&new_memory->buffer->empty, 1, 1) == -1) {
        perror("sem_init(empty)");
        munmap(new_memory->buffer, sizeof(shared_buffer));
        close(new_memory->fd);
        shm_unlink(new_memory->name);
        free(new_memory->name);
        free(new_memory);
        return -1;
    }
    if (sem_init(&new_memory->buffer->full, 1, 0) == -1) {
        perror("sem_init(full)");
        sem_destroy(&new_memory->buffer->empty);
        munmap(new_memory->buffer, sizeof(shared_buffer));
        close(new_memory->fd);
        shm_unlink(new_memory->name);
        free(new_memory->name);
        free(new_memory);
        return -1;
    }
    new_memory->owner = 1;
    new_memory->writer_closed = 0;
    *memory = new_memory;
    return 0;
}

int os_shm_open(os_shared_memory **memory, const char *name){
    if (memory == NULL || name == NULL) {
        return -1;
    }
    *memory = NULL;
    os_shared_memory *new_memory = calloc(1, sizeof(os_shared_memory));
    if (new_memory == NULL) {
        return -1;
    }
    new_memory->fd = shm_open(name, O_RDWR, 0);
    if (new_memory->fd == -1) {
        perror("shm_open");
        free(new_memory);
        return -1;
    }
    new_memory->name = strdup(name);
    if (new_memory->name == NULL) {
        close(new_memory->fd);
        free(new_memory);
        return -1;
    }
    void *mapping = mmap(NULL, sizeof(shared_buffer), PROT_READ | PROT_WRITE, MAP_SHARED, new_memory->fd, 0);
    if (mapping == MAP_FAILED) {
        perror("mmap");
        close(new_memory->fd);
        free(new_memory->name);
        free(new_memory);
        return -1;
    }
    new_memory->buffer = mapping;
    new_memory->owner = 0;
    new_memory->writer_closed = 0;
    *memory = new_memory;
    return 0;
}

int os_create_process(os_process **process, const char *executable, os_shared_memory *memory, const char *output_file){
    if (process == NULL || executable == NULL || memory == NULL || memory->name == NULL || output_file == NULL) {
        return -1;
    }
    *process = NULL;
    os_process *new_process = malloc(sizeof(os_process));
    if (new_process == NULL) {
        return -1;
    }
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        free(new_process);
        return -1;
    }
    if (pid > 0) {
        new_process->pid = pid;
        *process = new_process;
        return 0;
    }
    if (pid == 0) {
        int file_fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (file_fd == -1) {
            perror("open");
            _exit(1);
        }
        if (dup2(file_fd, STDOUT_FILENO) == -1) {
            perror("dup2");
            _exit(1);
        }
        if (file_fd != STDOUT_FILENO) {
            if (close(file_fd) == -1) {
                perror("close");
                _exit(1);
            }
        }
        char *args[] = {(char *)executable, memory->name, NULL};
        execvp(executable, args);
        perror("execvp");
        _exit(127);
    }
    return -1;
}

int os_shm_write(os_shared_memory *memory, const char *data, size_t length){
    if (memory == NULL || memory->buffer == NULL || memory->writer_closed || (data == NULL && length != 0)) {
        return -1;
    }
    shared_buffer *buffer = memory->buffer;
    size_t offset = 0;
    do {
        size_t part = length - offset;
        if (part > BUFFER_SIZE) {
            part = BUFFER_SIZE;
        }
        if (wait_semaphore(&buffer->empty) == -1) {
            perror("sem_wait(empty)");
            return -1;
        }
        buffer->length = part;
        buffer->is_last = (offset + part == length);
        buffer->finished = 0;
        if (part > 0) {
            memcpy(buffer->data, data + offset, part);
        }
        if (sem_post(&buffer->full) == -1) {
            perror("sem_post(full)");
            return -1;
        }
        offset += part;
    } while (offset < length);
    return 0;
}

int os_shm_read(os_shared_memory *memory, char **line, size_t *length){
    if (memory == NULL || memory->buffer == NULL || line == NULL || length == NULL) {
        return -1;
    }
    *line = NULL;
    *length = 0;
    size_t capacity = 256;
    size_t used = 0;
    char *result = malloc(capacity);
    if (result == NULL) {
        return -1;
    }
    shared_buffer *buffer = memory->buffer;
    while (1) {
        if (wait_semaphore(&buffer->full) == -1) {
            perror("sem_wait(full)");
            free(result);
            return -1;
        }
        if (buffer->finished) {
            if (sem_post(&buffer->empty) == -1) {
                perror("sem_post(empty)");
                free(result);
                return -1;
            }
            free(result);
            return 0;
        }
        size_t part = buffer->length;
        int last = buffer->is_last;
        if (part > BUFFER_SIZE || used > SIZE_MAX - part - 1) {
            sem_post(&buffer->empty);
            free(result);
            return -1;
        }
        size_t required = used + part + 1;
        if (required > capacity) {
            size_t new_capacity = capacity;
            while (new_capacity < required) {
                if (new_capacity > SIZE_MAX / 2) {
                    new_capacity = required;
                    break;
                }
                new_capacity *= 2;
            }
            char *tmp = realloc(result, new_capacity);
            if (tmp == NULL) {
                sem_post(&buffer->empty);
                free(result);
                return -1;
            }
            result = tmp;
            capacity = new_capacity;
        }
        if (part > 0) {
            memcpy(result + used, buffer->data, part);
        }
        used += part;
        if (sem_post(&buffer->empty) == -1) {
            perror("sem_post(empty)");
            free(result);
            return -1;
        }
        if (last) {
            result[used] = '\0';
            *line = result;
            *length = used;
            return 1;
        }
    }
}

int os_shm_close_writer(os_shared_memory *memory){
    if (memory == NULL || memory->buffer == NULL) {
        return -1;
    }
    if (memory->writer_closed) {
        return 0;
    }
    shared_buffer *buffer = memory->buffer;
    if (wait_semaphore(&buffer->empty) == -1) {
        perror("sem_wait(empty)");
        return -1;
    }
    buffer->length = 0;
    buffer->is_last = 1;
    buffer->finished = 1;
    if (sem_post(&buffer->full) == -1) {
        perror("sem_post(full)");
        return -1;
    }
    memory->writer_closed = 1;
    return 0;
}

int os_process_wait(os_process *process, int *exit_code){
    if (process == NULL || exit_code == NULL) {
        return -1;
    }
    int status;
    pid_t result;
    do {
        result = waitpid(process->pid, &status, 0);
    } while (result == -1 && errno == EINTR);
    if (result != process->pid) {
        perror("waitpid");
        return -1;
    }
    if (!WIFEXITED(status)) {
        return -1;
    }
    *exit_code = WEXITSTATUS(status);
    return 0;
}

void os_shm_destroy(os_shared_memory *memory){
    if (memory == NULL) {
        return;
    }
    if (memory->buffer != NULL) {
        if (memory->owner) {
            if (sem_destroy(&memory->buffer->empty) == -1) {
                perror("sem_destroy(empty)");
            }
            if (sem_destroy(&memory->buffer->full) == -1) {
                perror("sem_destroy(full)");
            }
        }
        if (munmap(memory->buffer, sizeof(shared_buffer)) == -1) {
            perror("munmap");
        }
    }
    if (memory->fd != -1 && close(memory->fd) == -1) {
        perror("close");
    }
    if (memory->owner && memory->name != NULL) {
        if (shm_unlink(memory->name) == -1) {
            perror("shm_unlink");
        }
    }
    free(memory->name);
    free(memory);
}

void os_destroy_process(os_process *process){
    if (process == NULL) {
        return;
    }
    free(process);
}
