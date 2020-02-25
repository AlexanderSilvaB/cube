#ifndef CUBE_files_h
#define CUBE_files_h

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "object.h"

ObjFile* openFile(char* fileName, char* mode);
bool fileMethods(char *method, int argCount);

#endif