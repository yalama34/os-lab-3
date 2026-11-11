#include "include/line_utils.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#include "include/line_utils.h"
#include "include/os_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

int main(int argc, char *argv[]){
    if (argc != 2){
        return 1;
    }
    os_shared_memory *memory = NULL;
    if (os_shm_open(&memory, argv[1]) != 0){
        return 1;
    }
    while (true){
        char *line = NULL;
        size_t length = 0;
        int result = os_shm_read(memory, &line, &length);
        if (result == 0){
            free(line);
            break;
        }
        if (result == -1){
            free(line);
            os_shm_destroy(memory);
            return 1;
        }
        remove_vowels(line);
        if (fputs(line, stdout) == EOF){
            free(line);
            os_shm_destroy(memory);
            return 1;
        }
        free(line);
    }
    int result = 0;
    if (fflush(stdout) == EOF){
        result = 1;
    }
    os_shm_destroy(memory);
    return result;
}