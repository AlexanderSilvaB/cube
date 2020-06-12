#ifndef CLOX_vm_h
#define CLOX_vm_h

#include "cubeext.h"
#include "object.h"
#include "table.h"
#include "value.h"

#define DISABLE_GC                                                                                                     \
    bool __gc = vm.gc;                                                                                                 \
    vm.gc = false
#define RESTORE_GC vm.gc = __gc;

#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * UINT16_COUNT)

typedef enum
{
    CALL_FRAME_TYPE_FUNCTION,
    CALL_FRAME_TYPE_PACKAGE
} CallFrameType;

typedef struct
{
    ObjClosure *closure;
    uint8_t *ip;
    Value *slots;
    ObjInstance *instance;
    ObjClass *klass;
    CallFrameType type;
    ObjPackage *package;
    ObjPackage *nextPackage;
    bool require;
} CallFrame;

typedef struct TryFrame_t
{
    uint8_t *ip;
    CallFrame *frame;
    struct TryFrame_t *next;
} TryFrame;

typedef struct TaskFrame_t
{
    CallFrame frames[FRAMES_MAX];
    int frameCount;
    Value stack[STACK_MAX];
    Value *stackTop;
    Value currentArgs;
    ObjUpvalue *openUpvalues;
    int currentFrameCount;
    bool popCallFrame;
    bool eval;
    struct TaskFrame_t *next;
    char *name;
    const char *currentScriptName;
    Value result;
    bool finished;
    bool waiting;
    bool aborted;
    bool busy;
    struct TaskFrame_t *parent;
    char *error;
    uint64_t endTime;
    uint64_t startTime;
    TryFrame *tryFrame;
    void *threadFrame;
} TaskFrame;

typedef enum
{
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR,
    INTERPRET_WAIT,
    INTERPRET_CONTINUE
} InterpretResult;

typedef struct ThreadFrame_t
{
    bool running;
    int tasksCount;
    int id;
    TaskFrame *taskFrame;
    TaskFrame *ctf;
    CallFrame *frame;
    InterpretResult result;
} ThreadFrame;

typedef struct
{
    ThreadFrame threadFrames[MAX_THREADS];

    // int tasksCount;
    // int id;
    // TaskFrame *taskFrame;
    // TaskFrame *ctf;
    // CallFrame *frame;

    Table globals;
    Table strings;
    Table extensions;
    ObjString *initString;
    ObjList *paths;
    ObjPackage *stdPackage;
    ObjList *packages;

    const char *argsString;
    const char *extension;
    const char *nativeExtension;
    const char *scriptName;

    bool gc;
    bool autoGC;
    bool skipWaitingTasks;

    size_t bytesAllocated;
    size_t nextGC;

    Obj *objects;

    bool newLine;
    bool ready;
    int exitCode;
    bool running;
    Value repl;
    bool print;
    bool debug;
    bool continueDebug;
    bool waitingDebug;
    bool forceInclude;
    DebugInfo debugInfo;
} VM;

extern VM vm;

void initVM(const char *path, const char *scriptName);
void freeVM();
void addPath(const char *path);
void loadArgs(int argc, const char *argv[], int argStart);
ThreadFrame *currentThread();

InterpretResult interpret(const char *source, const char *path);
InterpretResult compileCode(const char *source, const char *path, const char *output);
void push(Value value);
Value pop();
Value peek(int distance);

bool isFalsey(Value value);
void runtimeError(const char *format, ...);

#endif
