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
Value boolNative(int argCount, Value* args);
Value numNative(int argCount, Value* args);
Value intNative(int argCount, Value* args);
Value strNative(int argCount, Value* args);
Value listNative(int argCount, Value *args);
Value bytesNative(int argCount, Value *args);
Value makeNative(int argCount, Value *args);
Value ceilNative(int argCount, Value* args);
Value floorNative(int argCount, Value* args);
Value roundNative(int argCount, Value* args);
Value powNative(int argCount, Value* args);
Value expNative(int argCount, Value* args);
Value lenNative(int argCount, Value* args);
Value typeNative(int argCount, Value* args);
Value envNative(int argCount, Value* args);
Value openNative(int argCount, Value* args);
Value copyNative(int argCount, Value* args);

#endif
