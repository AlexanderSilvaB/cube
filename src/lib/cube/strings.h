#ifndef caboose_strings_h
#define caboose_strings_h

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "value.h"

bool stringMethods(char* method, int argCount);

bool stringContains(Value strinvV, Value delimiterV, Value *result);

#endif