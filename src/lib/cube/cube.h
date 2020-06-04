/*
    Cube is based on Lox (http://craftinginterpreters.com)
*/

#ifndef _CUBE_CUBE_H_
#define _CUBE_CUBE_H_

#include "cubeext.h"
#include "version.h"
#include <stdbool.h>

int repl();
int runFile(const char *path, const char *output, bool execute, bool binary);
int runCode(const char *source, const char *path, int argc, const char *argv[]);
void start(const char *path, const char *scriptName);
void stop();
int runCube(int argc, const char *argv[]);
void startCube(int argc, const char *argv[]);
void stopCube();
extern void addPath(const char *path);
void debug(bool enable);
void continueDebug();
bool waitingDebug();
DebugInfo debugInfo();

bool addGlobal(const char *name, cube_native_var *var);
cube_native_var *getGlobal(const char *name);
extern void freeVarNative(cube_native_var *var);

#endif
