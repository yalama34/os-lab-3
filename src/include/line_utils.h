#ifndef LINE_UTILS_H
#define LINE_UTILS_H

#include <stdio.h>
#include <stddef.h>

int read_line(FILE *stream, char **line, size_t *length);

void remove_vowels(char *line);

#endif