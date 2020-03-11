#ifndef CUBE_collections_h
#define CUBE_collections_h

#include <stdbool.h>
#include <string.h>

#include "object.h"
#include "value.h"

bool listMethods(char *method, int argCount);

bool dictMethods(char *method, int argCount);

bool listContains(Value listV, Value search, Value *result);
bool dictContains(Value dictV, Value keyV, Value *result);

void addList(ObjList *list, Value value);
void rmList(ObjList *list, Value value);
void stretchList(ObjList *list1, ObjList *list2);
void shrinkList(ObjList *list, int num);

ObjList *copyList(ObjList *oldList, bool shallow);
ObjDict *copyDict(ObjDict *oldDict, bool shallow);

#endif