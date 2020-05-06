#ifndef CUBE_NATIVE_h
#define CUBE_NATIVE_h

#include "common.h"
#include "cubeext.h"
#include "linkedList.h"
#include "object.h"

typedef struct
{
    bool is_func;
    char *name;
} symbol_t;

NativeTypes getNativeType(const char *name);
void closeNativeLib(ObjNativeLib *lib);
Value callNative(ObjNativeFunc *func, int argCount, Value *args);
bool nativeLibMethods(char *method, int argCount);

#endif