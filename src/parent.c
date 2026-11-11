#include "include/line_utils.h"
#include "include/os_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

int cleanup(os_shared_memory *memory1, os_shared_memory *memory2, os_process *process1, os_process *process2, char* file1, char* file2) {
    int result = 0;
    int exit_code = 0;
    if (memory1 != NULL) {
        if (os_shm_close_writer(memory1) != 0) {
            result = 1;
        }
    }
    if (memory2 != NULL) {
        if (os_shm_close_writer(memory2) != 0) {
            result = 1;
        }
    }
    if (process1 != NULL) {
        if (os_process_wait(process1, &exit_code) != 0) {
            result = 1;
        } else if (exit_code != 0) {
            result = 1;
        }
    }
    if (process2 != NULL) {
        if (os_process_wait(process2, &exit_code) != 0) {
            result = 1;
        } else if (exit_code != 0) {
            result = 1;
        }
    }

    free(file1);
    free(file2);

    os_shm_destroy(memory1);
    os_shm_destroy(memory2);

    os_destroy_process(process1);
    os_destroy_process(process2);

    return result;
}

int main(){
    srand(time(NULL));
    char *file1 = NULL;
    char *file2 = NULL;

    size_t file1_len = 0;
    size_t file2_len = 0;

    if (read_line(stdin, &file1, &file1_len) != 1) {
        free(file1);
        return 1;
    }
    if (file1_len > 0 && file1[file1_len - 1] == '\n') {
        file1_len--;
        file1[file1_len] = '\0';
    }
    if (read_line(stdin, &file2, &file2_len) != 1) {
        free(file1);
        free(file2);
        return 1;
    }
    if (file2_len > 0 && file2[file2_len - 1] == '\n') {
        file2_len--;
        file2[file2_len] = '\0';
    }

    if (file1_len == 0 || file2_len == 0 || strcmp(file1, file2) == 0) {
        free(file1);
        free(file2);
        return 1;
    }

    os_shared_memory *memory1 = NULL;
    os_shared_memory *memory2 = NULL;
    os_process *process_1 = NULL;
    os_process *process_2 = NULL;

    if (os_shm_create(&memory1) != 0){
        cleanup(memory1, memory2, process_1, process_2, file1, file2);
        return 1;
    }
    if (os_shm_create(&memory2) != 0){
        cleanup(memory1, memory2, process_1, process_2, file1, file2);
        return 1;
    }

    if (os_create_process(&process_1, "./child", memory1, file1) != 0) {
        cleanup(memory1, memory2, process_1, process_2, file1, file2);
        return 1;
    }
    if (os_create_process(&process_2, "./child", memory2, file2) != 0) {
        cleanup(memory1, memory2, process_1, process_2, file1, file2);
        return 1;
    }

    while (true){
        char *line = NULL;
        size_t length = 0;
        int result = read_line(stdin, &line, &length);
        if (result == 0){
            free(line);
            break;
        }
        if (result == -1){
            cleanup(memory1, memory2, process_1, process_2, file1, file2);
            free(line);
            return 1;
        }
        int choice = rand() % 100;
        if (choice < 80) {
            if (os_shm_write(memory1, line, length) < 0){
                cleanup(memory1, memory2, process_1, process_2, file1, file2);
                free(line);
                return 1;
            }
            free(line);
        } else {
            if (os_shm_write(memory2, line, length) < 0){
                cleanup(memory1, memory2, process_1, process_2, file1, file2);
                free(line);
                return 1;
            }
            free(line);
        }
    }
    int result = cleanup(memory1, memory2, process_1, process_2, file1, file2);
    return result;
}
