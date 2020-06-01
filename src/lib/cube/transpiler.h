#ifndef CLOX_transpiler_h
#define CLOX_transpiler_h

#include "object.h"
#include "vm.h"

bool transpile(const char *source, const char *path);

#endif
