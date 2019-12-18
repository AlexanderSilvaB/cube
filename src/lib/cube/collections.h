#ifndef CUBE_collections_h
#define CUBE_collections_h

#include <stdbool.h>
#include <string.h>

#include "value.h"
#include "object.h"

bool listMethods(char *method, int argCount);

bool dictMethods(char *method, int argCount);

bool listContains(Value listV, Value search, Value *result);
bool dictContains(Value dictV, Value keyV, Value *result);

ObjList *copyList(ObjList *oldList, bool shallow);
ObjDict *copyDict(ObjDict *oldDict, bool shallow);

#endif