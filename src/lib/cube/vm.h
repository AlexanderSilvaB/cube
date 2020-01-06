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
  ObjPackage *package;
  ObjPackage *nextPackage;
} CallFrame;

typedef struct TryFrame_t
{
  uint8_t *ip;
  struct TryFrame_t *next;
}TryFrame;


typedef struct TaskFrame_t
{
  CallFrame frames[FRAMES_MAX];
  int frameCount;
  Value stack[STACK_MAX];
  Value *stackTop;
  Value currentArgs;
  ObjUpvalue *openUpvalues;
  int currentFrameCount;
  bool eval;
  struct TaskFrame_t *next;
  char* name;
  const char *currentScriptName;
  Value result;
  bool finished;
  char *error;
  uint64_t endTime;
  uint64_t startTime;
  TryFrame *tryFrame;
}TaskFrame;

typedef struct
{
  TaskFrame *taskFrame;
  TaskFrame *ctf;
  CallFrame *frame;

  Table globals;
  Table strings;
  ObjString *initString;
  ObjList *paths;

  const char *argsString;
  const char *extension;
  const char *nativeExtension;
  const char *scriptName;

  bool gc;
  bool autoGC;

  size_t bytesAllocated;
  size_t nextGC;

  Obj *objects;

  bool newLine;
  bool ready;
  int exitCode;
  bool running;
  Value repl;
  bool print;
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
