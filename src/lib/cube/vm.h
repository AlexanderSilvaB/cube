#ifndef clox_vm_h
#define clox_vm_h

#include "object.h"
#include "table.h"
#include "value.h"

#define DISABLE_GC bool __gc = vm.gc; vm.gc = false
#define RESTORE_GC vm.gc = __gc;

#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT)

typedef struct
{
  ObjClosure *closure;
  uint8_t *ip;
  Value *slots;
} CallFrame;

typedef struct ThreadFrame_t
{
  CallFrame frames[FRAMES_MAX];
  int frameCount;
  Value stack[STACK_MAX];
  Value *stackTop;
  ObjUpvalue *openUpvalues;
  int currentFrameCount;
  bool eval;
  struct ThreadFrame_t *next;
  const char* name;
}ThreadFrame;

typedef struct
{
  CallFrame frames[FRAMES_MAX];
  int frameCount;
  Value stack[STACK_MAX];
  Value *stackTop;
  ObjUpvalue *openUpvalues;
  int currentFrameCount;
  bool eval;

  ThreadFrame *threadFrame;
  ThreadFrame *ctf;

  Table globals;
  Table strings;
  ObjString *initString;
  ObjString *argsString;
  ObjList *paths;

  const char *extension;
  const char *nativeExtension;
  const char *scriptName;
  const char *currentScriptName;

  bool gc;

  size_t bytesAllocated;
  size_t nextGC;

  Obj *objects;
  Obj *listObjects;
  int grayCount;
  int grayCapacity;
  Obj **grayStack;

  bool newLine;
} VM;

typedef enum
{
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR
} InterpretResult;

extern VM vm;

void initVM(const char* path, const char *scriptName);
void freeVM();
void addPath(const char* path);
void loadArgs(int argc, const char *argv[], int argStart);

InterpretResult interpret(const char *source);
InterpretResult compileCode(const char *source, const char* path);
void push(Value value);
Value pop();
Value peek(int distance);

bool isFalsey(Value value);
void runtimeError(const char *format, ...);

#endif
