#include "include/line_utils.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

int main(){
    while (true){
        char *line = NULL;
        size_t length = 0;
        int result = read_line(stdin, &line, &length);
        if (result == 0){
            free(line);
            break;
        }
        if (result == -1){
            free(line);
            return 1;
        }
        remove_vowels(line);
        if (fputs(line, stdout) == EOF) {
            free(line);
            return 1;
        }
        free(line);
    }
    return 0;
}