#ifndef SERVER_H
#define SERVER_H

#include <stdbool.h>
#include <stddef.h>

int count_word(char *path, char *str);
void listFilesRecursively(char *basePath, char *str, char *each_string);

#endif
