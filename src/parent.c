#include "include/line_utils.h"
#include "include/os_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

//TODO одинаковые имена файлов

int cleanup(os_pipe *pipe1, os_pipe *pipe2, os_process *process1, os_process *process2, char* file1, char* file2) {
    int result = 0;
    int exit_code = 0;
    if (pipe1 != NULL) {
        if (os_pipe_close_writer(pipe1) != 0) {
            result = 1;
        }
    }
    if (pipe2 != NULL) {
        if (os_pipe_close_writer(pipe2) != 0) {
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

    os_destroy_pipe(pipe1);
    os_destroy_pipe(pipe2);

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

    if (file1_len == 0 || file2_len == 0) {
        free(file1);
        free(file2);
        return 1;
    }

    os_pipe *pipe1 = NULL;
    os_pipe *pipe2 = NULL;
    os_process *process_1 = NULL;
    os_process *process_2 = NULL;

    if (os_create_pipe(&pipe1) != 0){
        cleanup(pipe1, pipe2, process_1, process_2, file1, file2);
        return 1;
    }
    if (os_create_pipe(&pipe2) != 0){
        cleanup(pipe1, pipe2, process_1, process_2, file1, file2);
        return 1;
    }


    if (os_create_process(&process_1, "./child", pipe1, file1) != 0) {
        cleanup(pipe1, pipe2, process_1, process_2, file1, file2);
        return 1;
    }
    if (os_create_process(&process_2, "./child", pipe2, file2) != 0) {
        cleanup(pipe1, pipe2, process_1, process_2, file1, file2);
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
            cleanup(pipe1, pipe2, process_1, process_2, file1, file2);
            free(line);
            return 1;
        }
        int choice = rand() % 100;
        if (choice < 80) {
            if (os_pipe_write(pipe1, line, length) < 0){
                cleanup(pipe1, pipe2, process_1, process_2, file1, file2);
                free(line);
                return 1;
            }
            free(line);
        } else {
            if (os_pipe_write(pipe2, line, length) < 0){
                cleanup(pipe1, pipe2, process_1, process_2, file1, file2);
                free(line);
                return 1;
            }
            free(line);
        }
    }
    int result = cleanup(pipe1, pipe2, process_1, process_2, file1, file2);
    return result;
}