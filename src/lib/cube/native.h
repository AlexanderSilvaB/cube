#ifndef CUBE_NATIVE_h
#define CUBE_NATIVE_h

#include "common.h"
#include "object.h"

typedef enum
{
    TYPE_VOID,
    TYPE_BOOL,
    TYPE_NUMBER,
    TYPE_STRING,
    TYPE_BYTES,
    TYPE_UNKNOWN
}NativeTypes;


NativeTypes getNativeType(const char *name);
void closeNativeLib(ObjNativeLib *lib);
Value callNative(ObjNativeFunc *func, int argCount, Value *args);

#endif