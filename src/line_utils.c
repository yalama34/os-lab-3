#include "include/line_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

int read_line(FILE *stream, char **line, size_t *length){
    if (stream == NULL || line == NULL || length == NULL){
        return -1;
    }
    size_t buf_cap = 256;
    size_t buf_growth = 2;
    char *buf = malloc(buf_cap * sizeof(char));
    if (buf == NULL) {
        return -1;
    }
    int ch;
    size_t buf_size = 0;
    while((ch = getc(stream)) != EOF){
        buf[buf_size] = ch;
        if((++buf_size) >= buf_cap){
            buf_cap *= buf_growth;
            char *tmp = realloc(buf, buf_cap);
            if (tmp == NULL){
                free(buf);
                return -1;
            }
            buf = tmp;
        }
        if (ch == '\n') {
            break;
        }
    }
    if (buf_size == 0) {
        free(buf);
        return 0;
    }
    buf[buf_size] = '\0';
    *line = buf;
    *length = buf_size;
    return 1;
}

void remove_vowels(char *line) {
    if (line == NULL) {
        return;
    }
    size_t j = 0;
    for (size_t i = 0; line[i] != '\0'; i++) {
        char ch = line[i];
        int is_vowel = (ch == 'a' || ch == 'e' || ch == 'i' || ch == 'o' || ch == 'u'  || ch == 'y' || ch == 'A' || ch == 'E' || ch == 'I' || ch == 'O' || ch == 'U' || ch == 'Y');
        if (!is_vowel) {
            line[j] = ch;
            j++;
        }
    }

    line[j] = '\0';
}