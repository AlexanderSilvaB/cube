#ifndef CUBE_NATIVE_h
#define CUBE_NATIVE_h

#include "common.h"
#include "cubeext.h"
#include "object.h"


NativeTypes getNativeType(const char *name);
void closeNativeLib(ObjNativeLib *lib);
Value callNative(ObjNativeFunc *func, int argCount, Value *args);

#endif