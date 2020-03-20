#ifndef CUBE_bytes_h
#define CUBE_bytes_h

#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "value.h"

bool bytesMethods(char *method, int argCount);

bool bytesContains(Value bytesV, Value delimiterV, Value *result);

#endif