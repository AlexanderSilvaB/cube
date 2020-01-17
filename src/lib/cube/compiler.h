#ifndef CLOX_compiler_h
#define CLOX_compiler_h

#include "object.h"
#include "vm.h"

ObjFunction *compile(const char *source);
ObjFunction *eval(const char *source);
bool initByteCode(FILE *file);
bool writeByteCode(FILE *file, Value value);
bool finishByteCode(FILE *file);
Value loadByteCode(const char *source, uint32_t *pos, uint32_t total);
void markCompilerRoots();

#endif
