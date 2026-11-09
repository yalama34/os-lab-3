#define _GNU_SOURCE
#include <sys/types.h>
#include <unistd.h>
#include "../include/os_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/wait.h>

struct os_pipe{
    int fd[2];
};

struct os_process{
    pid_t pid;
};

int os_create_pipe(os_pipe **out_pipe){
    if (out_pipe == NULL) {
        return -1;
    }
    *out_pipe = NULL;
    os_pipe *new_pipe = malloc(sizeof(os_pipe));
    if (new_pipe == NULL) {
        return -1;
    }
    if (pipe2(new_pipe->fd, O_CLOEXEC) == -1) {
        free(new_pipe);
        return -1;
    }
    *out_pipe = new_pipe;
    return 0;
}

int os_create_process(os_process **process, const char *executable, os_pipe *input, const char *output_file){
    if (process == NULL || executable == NULL || input == NULL || output_file == NULL) {
        return -1;
    }
    if (input->fd[0] == -1 || input->fd[1] == -1) {
        return -1;
    }
    *process = NULL;
    os_process *new_process = malloc(sizeof(os_process));
    if (new_process == NULL) {
        perror("Allocation error");
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
    if (pid == 0){
        if (close(input->fd[1]) == -1) {
            perror("close");
            _exit(1);
        }
        if (dup2(input->fd[0], STDIN_FILENO) == -1){
            perror("dup2");
            _exit(1);
        }

        if (input->fd[0] != STDIN_FILENO) {
            if (close(input->fd[0]) == -1){
                perror("close");
                _exit(1);
            }
        
        }
        int file_fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (file_fd == -1){
            perror("open");
            _exit(1);
        }
        if (dup2(file_fd, STDOUT_FILENO) == -1){
            perror("dup2");
            _exit(1);
        }
        if (file_fd != STDOUT_FILENO) {
            if (close(file_fd) == -1){
                perror("close");
                _exit(1);
            }
        }
        char *args[] = {(char *)executable, NULL};
        execvp(executable, args);
        perror("execvp");
        _exit(127);

    }
    return -1;
}

int os_pipe_write(os_pipe *pipe, const void *data, size_t size){
    if (pipe == NULL || pipe->fd[1] == -1 || (data == NULL && size > 0)) {
        return -1;
    }
    size_t written = 0;
    const char *p = data;
    while (written < size) {
        ssize_t cur_written = write(pipe->fd[1], p, size - written);
        if (cur_written > 0){
            written += cur_written;
            p += cur_written;
        } else if (cur_written == 0) {
            return -1;
        } else {
            perror("write");
            return -1;
        }
    }
    return 0;
}

int os_pipe_close_writer(os_pipe *pipe){
    if (pipe == NULL || pipe->fd[1] == -1){
        return -1;
    }
    if (close(pipe->fd[1]) == -1) {
        return -1;
    }
    pipe->fd[1] = -1;
    return 0;
}

int os_process_wait(os_process *process, int *exit_code){
    if (process == NULL || exit_code == NULL) {
        return -1;
    }
    int status;
    pid_t result = waitpid(process->pid, &status, 0);
    if (result != process->pid){
        perror("waitpid");
        return -1;
    } else {
        if (!WIFEXITED(status)){
            return -1;
        }
        *exit_code = WEXITSTATUS(status);
        return 0;
    }
}

void os_destroy_pipe(os_pipe *pipe){
    if (pipe == NULL){
        return;
    }
    if (pipe->fd[0] != -1){
        if (close(pipe->fd[0]) == -1){
            perror("close");
        }
    }
    if (pipe->fd[1] != -1){
        if (close(pipe->fd[1]) == -1){
            perror("close");
        }
    }
    free(pipe);
    return;
}

void os_destroy_process(os_process *process){
    if (process == NULL){
        return;
    }
    free(process);
    return;
}