/*
    Cube is based on Lox (http://craftinginterpreters.com)
*/

#ifndef _CUBE_CUBE_H_
#define _CUBE_CUBE_H_

#include "common.h"
#include "chunk.h"
#include "debug.h"
#include "vm.h"

int repl();
int runFile(const char* path, bool justCompile);
void start();
void stop();
int cube(int argc, const char* argv[]);

#endif
