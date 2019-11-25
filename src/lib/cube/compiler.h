#ifndef _CUBE_COMPILER_H_
#define _CUBE_COMPILER_H_

#include "chunk.h"
#include "object.h"

ObjFunction* compile(const char* source);

ObjFunction* readByteCode(const char* path, uint32_t hash);
void writeByteCode(const char* path, ObjFunction* func, uint32_t hash);

#endif
