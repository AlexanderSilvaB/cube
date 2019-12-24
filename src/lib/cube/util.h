#ifndef CUBE_util_h
#define CUBE_util_h

#include <stdbool.h>

char *readFile(const char *path, bool verbose);
char *getFolder(const char* path);
int countBytes(const void* raw, int maxSize);
void replaceString(char *str, const char* find, const char* replace);
char* findFile(const char* name);
bool existsFile(const char *name);
bool isValidType(const char* name);

#endif