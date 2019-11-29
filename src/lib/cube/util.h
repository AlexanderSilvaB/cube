#ifndef caboose_util_h
#define caboose_util_h

#include <stdbool.h>

char *readFile(const char *path, bool verbose);
char *getFolder(const char* path);

#endif