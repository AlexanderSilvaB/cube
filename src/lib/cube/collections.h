#ifndef caboose_collections_h
#define caboose_collections_h

#include <stdbool.h>
#include <string.h>

#include "value.h"

bool listMethods(char *method, int argCount);

bool dictMethods(char *method, int argCount);

bool listContains(Value listV, Value search, Value *result);
bool dictContains(Value dictV, Value keyV, Value *result);

#endif