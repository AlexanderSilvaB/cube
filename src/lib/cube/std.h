#ifndef _CUBE_STD_H_
#define _CUBE_STD_H_

#include "common.h"
#include "value.h"

Value clockNative(int argCount, Value* args);
Value timeNative(int argCount, Value* args);
Value exitNative(int argCount, Value* args);
Value inputNative(int argCount, Value* args);
Value printNative(int argCount, Value* args);
Value printlnNative(int argCount, Value* args);
Value randomNative(int argCount, Value* args);

#endif
