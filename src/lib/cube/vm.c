#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <linenoise/linenoise.h>

#include "bytes.h"
#include "collections.h"
#include "common.h"
#include "compiler.h"
#include "debug.h"
#include "enums.h"
#include "files.h"
#include "gc.h"
#include "memory.h"
#include "mempool.h"
#include "native.h"
#include "object.h"
#include "processes.h"
#include "scanner.h"
#include "std.h"
#include "strings.h"
#include "tasks.h"
#include "threads.h"
#include "util.h"
#include "vm.h"

VM vm; // [one]

void *threadFn(void *data);
bool hasTask(ThreadFrame *threadFrame);

#define READ_BYTE() (*frame->ip++)
#define SKIP_BYTE() frame->ip++
#define READ_SHORT() (frame->ip += 2, (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))

#define READ_CONSTANT() (frame->closure->function->chunk.constants.values[READ_SHORT()])

#define READ_STRING() AS_STRING(READ_CONSTANT())

#define BINARY_OP(valueType, op)                                                                                       \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1)))                                                                \
        {                                                                                                              \
            if (IS_STRING(peek(0)) && IS_STRING(peek(1)))                                                              \
            {                                                                                                          \
                if (compareStrings(#op))                                                                               \
                    break;                                                                                             \
            }                                                                                                          \
            else if (IS_ENUM_VALUE(peek(0)) && IS_ENUM_VALUE(peek(1)) && IS_NUMBER(AS_ENUM_VALUE(peek(0))->value) &&   \
                     IS_NUMBER(AS_ENUM_VALUE(peek(1))->value))                                                         \
            {                                                                                                          \
                double b = AS_NUMBER(AS_ENUM_VALUE(pop())->value);                                                     \
                double a = AS_NUMBER(AS_ENUM_VALUE(pop())->value);                                                     \
                push(valueType(a op b));                                                                               \
                break;                                                                                                 \
            }                                                                                                          \
            else if (IS_ENUM_VALUE(peek(0)) && IS_NUMBER(peek(1)) && IS_NUMBER(AS_ENUM_VALUE(peek(0))->value))         \
            {                                                                                                          \
                double b = AS_NUMBER(AS_ENUM_VALUE(pop())->value);                                                     \
                double a = AS_NUMBER(pop());                                                                           \
                push(valueType(a op b));                                                                               \
                break;                                                                                                 \
            }                                                                                                          \
            else if (IS_ENUM_VALUE(peek(1)) && IS_NUMBER(peek(0)) && IS_NUMBER(AS_ENUM_VALUE(peek(1))->value))         \
            {                                                                                                          \
                double b = AS_NUMBER(pop());                                                                           \
                double a = AS_NUMBER(AS_ENUM_VALUE(pop())->value);                                                     \
                push(valueType(a op b));                                                                               \
                break;                                                                                                 \
            }                                                                                                          \
            runtimeError("Operands must be numbers or strings.");                                                      \
            if (!checkTry(frame))                                                                                      \
                return INTERPRET_RUNTIME_ERROR;                                                                        \
        }                                                                                                              \
        else                                                                                                           \
        {                                                                                                              \
                                                                                                                       \
            double b = AS_NUMBER(pop());                                                                               \
            double a = AS_NUMBER(pop());                                                                               \
            push(valueType(a op b));                                                                                   \
        }                                                                                                              \
    } while (false)

#if MAX_THREADS == 1
ThreadFrame *currentThread()
{
    return &vm.threadFrames[0];
}
#else
ThreadFrame *currentThread()
{
    int id = thread_id();
    for (int i = 0; i < MAX_THREADS; i++)
    {
        if (id == vm.threadFrames[i].id)
            return &vm.threadFrames[i];
    }
    return &vm.threadFrames[0];
}
#endif

static ThreadFrame *createThreadFrame()
{
    int i = 0;
    for (i = 0; i < MAX_THREADS; i++)
    {
        if (vm.threadFrames[i].taskFrame == NULL)
            break;
    }
    if (i == MAX_THREADS)
        return currentThread();

    if (i > 0)
    {
        vm.threadFrames[i].running = false;
        vm.threadFrames[i].destroy = false;
        int id = thread_create(threadFn, &vm.threadFrames[i]);
        if (id == 0)
        {
            return currentThread();
        }

        vm.threadFrames[i].id = id;
    }
    else
    {
        int id = thread_id();
        vm.threadFrames[i].destroy = false;
        vm.threadFrames[i].running = true;
        vm.threadFrames[i].id = thread_id();
        vm.threadFrames[i].result = INTERPRET_OK;
    }

    return &vm.threadFrames[i];
}

static void destroyThreadFrame(ThreadFrame *tf)
{
    if (tf == NULL)
        tf = currentThread();

    if (!hasTask(tf))
    {
        printf("Does not have task\n");
        // tf->running = false;
        // tf->destroy = true;
        // thread_join(tf->id);
        // tf->taskFrame = NULL;
        // tf->id = 0;
    }
}

TaskFrame *createTaskFrame(const char *name)
{
    ThreadFrame *threadFrame = createThreadFrame();

    TaskFrame *taskFrame = (TaskFrame *)mp_calloc(1, sizeof(TaskFrame));
    taskFrame->name = (char *)mp_malloc(sizeof(char) * (strlen(name) + 1));
    strcpy(taskFrame->name, name);
    taskFrame->next = NULL;
    taskFrame->finished = false;
    taskFrame->waiting = false;
    taskFrame->parent = NULL;
    taskFrame->aborted = false;
    taskFrame->result = NULL_VAL;
    taskFrame->eval = false;
    taskFrame->stackTop = taskFrame->stack;
    taskFrame->frameCount = 0;
    taskFrame->openUpvalues = NULL;
    taskFrame->currentFrameCount = 0;
    taskFrame->startTime = 0;
    taskFrame->endTime = 0;
    taskFrame->busy = false;
    taskFrame->secure = false;
    taskFrame->tryFrame = NULL;
    taskFrame->error = NULL;
    taskFrame->currentArgs = NULL_VAL;
    taskFrame->threadFrame = threadFrame;

    TaskFrame *tf = threadFrame->taskFrame;
    // TaskFrame *tf = threadFrame->taskFrame;
    if (tf == NULL)
        threadFrame->taskFrame = taskFrame;
    // threadFrame->taskFrame = taskFrame;
    else
    {
        while (tf->next != NULL)
        {
            tf = tf->next;
        }
        tf->next = taskFrame;
    }

    if (threadFrame != currentThread())
    {
        threadFrame->ctf = threadFrame->taskFrame;
        threadFrame->frame = NULL;
    }

    return taskFrame;
}

ThreadFrame *findThreadFrame(const char *name)
{
    for (int i = 0; i < MAX_THREADS; i++)
    {
        ThreadFrame *threadFrame = &vm.threadFrames[i];
        TaskFrame *tf = threadFrame->taskFrame;
        if (tf == NULL)
            continue;

        while (tf != NULL)
        {
            if (strcmp(name, tf->name) == 0)
            {
                return threadFrame;
            }

            tf = tf->next;
        }
    }
    return NULL;
}

TaskFrame *findTaskFrame(const char *name)
{
    for (int i = 0; i < MAX_THREADS; i++)
    {
        ThreadFrame *threadFrame = &vm.threadFrames[i];
        TaskFrame *tf = threadFrame->taskFrame;
        if (tf == NULL)
            continue;

        while (tf != NULL)
        {
            if (strcmp(name, tf->name) == 0)
            {
                return tf;
            }

            tf = tf->next;
        }
    }
    return NULL;
}

void destroyTaskFrame(const char *name)
{
    for (int i = 0; i < MAX_THREADS; i++)
    {
        ThreadFrame *threadFrame = &vm.threadFrames[i];
        TaskFrame *tf = threadFrame->taskFrame;
        if (tf == NULL)
            continue;

        if (strcmp(name, tf->name) == 0)
        {
            tf->aborted = true;
            threadFrame->taskFrame = tf->next;
            // mp_free(tf->name);
            // mp_free(tf);
            break;
        }
        else
        {
            TaskFrame *parent = NULL;
            while (tf != NULL && strcmp(name, tf->name) != 0)
            {
                parent = tf;
                tf = tf->next;
            }

            if (tf != NULL)
            {
                tf->aborted = true;
                parent->next = tf->next;
                // mp_free(tf->name);
                // mp_free(tf);
                break;
            }
        }
    }
}

static void pushTry(CallFrame *frame, uint16_t offset)
{
    ThreadFrame *threadFrame = currentThread();
    TryFrame *
    try
        = (TryFrame *)mp_malloc(sizeof(TryFrame));
    try
        ->next = threadFrame->ctf->tryFrame;
    try
        ->ip = frame->ip + offset;
    try
        ->frame = frame;
    threadFrame->ctf->tryFrame =
    try
        ;
}

static void popTry()
{
    ThreadFrame *threadFrame = currentThread();
    TryFrame *
    try
        = threadFrame->ctf->tryFrame;
    threadFrame->ctf->tryFrame =
    try
        ->next;
    mp_free(try);
}

static void resetStack()
{
    ThreadFrame *threadFrame = currentThread();
    threadFrame->ctf->stackTop = threadFrame->ctf->stack;
    threadFrame->ctf->frameCount = 0;
    threadFrame->ctf->openUpvalues = NULL;
    threadFrame->ctf->currentFrameCount = 0;
}

void runtimeError(const char *format, ...)
{
    ThreadFrame *threadFrame = currentThread();
    char *str = (char *)mp_malloc(sizeof(char) * 1024);
    str[0] = '\0';
    for (int i = threadFrame->ctf->frameCount - 1; i >= 0; i--)
    {
        CallFrame *frame = &threadFrame->ctf->frames[i];

        ObjFunction *function = frame->closure->function;

        // -1 because the IP is sitting on the next instruction to be
        // executed.
        size_t instruction = frame->ip - function->chunk.code - 1;
        int line = getLine(&function->chunk, instruction);
        sprintf(str + strlen(str), "[line %d] in ", line);

        if (function->name == NULL)
        {
            sprintf(str + strlen(str), "%s: ", threadFrame->ctf->currentScriptName);
            i = -1;
        }
        else
        {
            if (frame->module != NULL)
            {
                sprintf(str + strlen(str), "%s.", frame->module->name->chars);
            }

            sprintf(str + strlen(str), "%s(): ", function->name->chars);
        }

        va_list args;
        va_start(args, format);
        vsprintf(str + strlen(str), format, args);
        if (i > 0)
            sprintf(str + strlen(str), "\n");
        va_end(args);
    }

    // sprintf(str + strlen(str), "\nThread[%d] Task[%s]", vm.id, threadFrame->ctf->name);
    sprintf(str + strlen(str), "\nTask[%s]", threadFrame->ctf->name);

    threadFrame->ctf->error = str;
}

static void defineNative(const char *name, NativeFn function, ObjModule *module)
{
    ThreadFrame *threadFrame = currentThread();
    push(OBJ_VAL(copyString(name, (int)strlen(name))));
    push(OBJ_VAL(newNative(function)));
    tableSet(&vm.globals, AS_STRING(threadFrame->ctf->stack[0]), threadFrame->ctf->stack[1]);
    if (module != NULL)
    {
        tableSet(&module->symbols, AS_STRING(threadFrame->ctf->stack[0]), threadFrame->ctf->stack[1]);
    }
    pop();
    pop();
}

void initVM(const char *path, const char *scriptName)
{
    vm.debug = false;
    vm.forceInclude = false;
    vm.ready = false;
    vm.exitCode = 0;
    vm.continueDebug = false;
    vm.waitingDebug = false;
    vm.debugInfo.line = 0;
    vm.debugInfo.path = "";
    // threadFrame->taskFrame = NULL;
    vm.running = true;
    vm.repl = NULL_VAL;
    vm.print = false;
    vm.skipWaitingTasks = false;
    vm.rootPath = NULL_VAL;

    memset(vm.threadFrames, '\0', sizeof(ThreadFrame) * MAX_THREADS);

    // vm.id = 0;
    createTaskFrame("default");

    ThreadFrame *threadFrame = currentThread();
    threadFrame->ctf = threadFrame->taskFrame;
    threadFrame->frame = NULL;

    resetStack();

    vm.gc = false;
#ifdef GC_AUTO
    vm.autoGC = true;
#else
    vm.autoGC = false;
#endif

    vm.objects = NULL;
    vm.bytesAllocated = 0;
    vm.nextGC = 1024 * 1024;

    initTable(&vm.globals);
    initTable(&vm.strings);
    initTable(&vm.extensions);
    vm.paths = initList();
    vm.modules = initList();
    addPath(path);

    vm.extension = ".cube";
#ifdef _WIN32
    vm.nativeExtension = ".dll";
#else
    vm.nativeExtension = ".so";
#endif
    vm.scriptName = scriptName;
    threadFrame->ctf->currentScriptName = scriptName;
    vm.argsString = "__args__";

    vm.initString = NULL;
    vm.initString = AS_STRING(STRING_VAL("init"));
    vm.newLine = false;

    // STD
    initStd();
    vm.stdModule = newModule(AS_STRING(STRING_VAL("std")));
    tableSet(&vm.globals, vm.stdModule->name, OBJ_VAL(vm.stdModule));
    ObjProcess *io = defaultProcess();
    tableSet(&vm.stdModule->symbols, AS_STRING(STRING_VAL("io")), OBJ_VAL(io));
    do
    {
        std_fn *stdFn = linked_list_get(stdFnList);
        if (stdFn != NULL)
        {
            defineNative(stdFn->name, stdFn->fn, vm.stdModule);
            linenoise_add_keyword(stdFn->name);
        }
    } while (linked_list_next(&stdFnList));
    destroyStd();

#ifndef GC_DISABLED
    vm.gc = true;
#endif
    vm.ready = true;
}

void freeVM()
{
    vm.ready = false;
    vm.running = false;
    freeTable(&vm.globals);
    freeTable(&vm.strings);
    vm.initString = NULL;
    vm.gc = false;
    freeObjects();
}

void addPath(const char *path)
{
    char *home = getHome();
    char *pathStr;
    if (home == NULL)
        pathStr = (char *)path;
    else
    {
        pathStr = (char *)mp_malloc(sizeof(char) * 256);
        pathStr[0] = '\0';
        strcpy(pathStr, path);
        replaceString(pathStr, "~", home);
    }

    DISABLE_GC;
    writeValueArray(&vm.paths->values, STRING_VAL(pathStr));
    RESTORE_GC;

    if (home == NULL)
        mp_free(pathStr);
}

ObjModule *findModule()
{
    return NULL;
}

static int initArgC = 0;
static int initArgStart = 0;
static const char **initArgV;

void loadArgs(int argc, const char *argv[], int argStart)
{
    initArgC = argc;
    initArgStart = argStart;
    initArgV = argv;
}

void push(Value value)
{
    ThreadFrame *threadFrame = currentThread();
    *threadFrame->ctf->stackTop = value;
    threadFrame->ctf->stackTop++;
}

Value pop()
{
    ThreadFrame *threadFrame = currentThread();
    threadFrame->ctf->stackTop--;
    return *threadFrame->ctf->stackTop;
}

Value peek(int distance)
{
    ThreadFrame *threadFrame = currentThread();
    return threadFrame->ctf->stackTop[-1 - distance];
}

static bool call(ObjClosure *closure, int argCount, ObjInstance *instance, ObjClass *klass)
{

    /*if (argCount != closure->function->arity)
    {
      runtimeError("Expected %d arguments but got %d.",
                   closure->function->arity, argCount);
      return false;
    }*/

    if (argCount < closure->function->arity)
    {
        // runtimeError("Expected at least %d arguments but got %d.",
        //              closure->function->arity, argCount);
        // return false;
        while (argCount < closure->function->arity)
        {
            push(NULL_VAL);
            argCount++;
        }
    }

    ThreadFrame *threadFrame = currentThread();
    if (threadFrame->ctf->frameCount == FRAMES_MAX)
    {
        runtimeError("Stack overflow.");
        return false;
    }

    CallFrame *frame = &(threadFrame->ctf)->frames[threadFrame->ctf->frameCount++];
    frame->closure = closure;
    frame->ip = closure->function->chunk.code;
    frame->type = CALL_FRAME_TYPE_FUNCTION;
    frame->module = closure->module ? closure->module : (threadFrame->frame ? threadFrame->frame->module : NULL);
    frame->nextModule = NULL;
    frame->require = false;
    frame->instance = instance;
    frame->klass = klass;
    if (frame->instance == NULL && threadFrame->frame && threadFrame->frame->instance != NULL)
    {
        frame->instance = threadFrame->frame->instance;
    }
    if (frame->klass == NULL && frame->instance != NULL)
    {
        frame->klass = frame->instance->klass;
    }
    if (frame->klass == NULL && threadFrame->frame && threadFrame->frame->klass != NULL)
    {
        frame->klass = threadFrame->frame->klass;
    }
    if (threadFrame->frame && threadFrame->frame->nextModule != NULL)
    {
        frame->module = threadFrame->frame->nextModule;
        threadFrame->frame->nextModule = NULL;
    }

    frame->slots = threadFrame->ctf->stackTop - argCount - 1;

    ObjList *args = initList();
    for (int i = argCount - 1; i >= 0; i--)
    {
        writeValueArray(&args->values, peek(i));
    }
    threadFrame->ctf->currentArgs = OBJ_VAL(args);

    if (argCount > closure->function->arity)
    {
        while (argCount > closure->function->arity)
        {
            pop();
            argCount--;
        }
    }

    return true;
}

static bool findField(ObjInstance *instance, ObjString *name, Value *field)
{
    if (!tableGet(&instance->fields, name, field))
    {
        if (!tableGet(&instance->klass->staticFields, name, field) || IS_CLOSURE(*field))
        {
            return false;
        }
    }

    return true;
}

static bool findMethod(ObjClass *klass, ObjString *name, Value *method, ObjClass **selected)
{
    if (!tableGet(&klass->methods, name, method))
    {
        if (!tableGet(&klass->staticFields, name, method) || !IS_CLOSURE(*method))
        {
            if (klass->super != NULL)
                return findMethod(klass->super, name, method, selected);

            return false;
        }
    }

    *selected = klass;
    return true;
}

static bool callValue(Value callee, int argCount, ObjInstance *instance, ObjClass *klass)
{
    ThreadFrame *threadFrame = currentThread();
    if (IS_OBJ(callee))
    {
        switch (OBJ_TYPE(callee))
        {
            case OBJ_BOUND_METHOD: {
                ObjBoundMethod *bound = AS_BOUND_METHOD(callee);

                // Replace the bound method with the receiver so it's in the
                // right slot when the method is called.
                threadFrame->ctf->stackTop[-argCount - 1] = bound->receiver;
                threadFrame->frame->nextModule = threadFrame->frame->module;
                return call(bound->method, argCount, instance, klass);
            }

            case OBJ_CLASS: {
                ObjClass *klass = AS_CLASS(callee);

                // Create the instance.
                instance = newInstance(klass);
                threadFrame->ctf->stackTop[-argCount - 1] = OBJ_VAL(instance);
                // Call the initializer, if there is one.
                Value initializer;
                ObjClass *selected;
                if (findMethod(klass, vm.initString, &initializer, &selected))
                {
                    threadFrame->frame->nextModule = klass->module ? klass->module : threadFrame->frame->module;
                    return call(AS_CLOSURE(initializer), argCount, instance, instance->klass);
                }
                else if (argCount != 0)
                {
                    runtimeError("Expected 0 arguments but got %d.", argCount);
                    return false;
                }

                return true;
            }
            case OBJ_CLOSURE:
                return call(AS_CLOSURE(callee), argCount, instance, klass);

            case OBJ_NATIVE: {
                NativeFn native = AS_NATIVE(callee);
                Value result = native(argCount, threadFrame->ctf->stackTop - argCount);
                if (IS_REQUEST(result))
                {
                    ObjRequest *request = AS_REQUEST(result);
                    // threadFrame->ctf->stackTop[-argCount - 1] = value;
                    ObjInstance *ins = AS_INSTANCE(*(threadFrame->ctf->stackTop - argCount));
                    if (ins != NULL)
                        instance = ins;
                    return callValue(request->fn, argCount - request->pops, instance, klass);
                }
                else
                {
                    threadFrame->ctf->stackTop -= argCount + 1;
                    push(result);
                    return true;
                }
            }

            case OBJ_NATIVE_FUNC: {
                ObjNativeFunc *func = AS_NATIVE_FUNC(callee);
                Value result = callNative(func, argCount, threadFrame->ctf->stackTop - argCount);
                threadFrame->ctf->stackTop -= argCount + 1;
                push(result);
                return true;
            }

            case OBJ_NATIVE_STRUCT: {
                ObjNativeStruct *str = AS_NATIVE_STRUCT(callee);
                Value result = createNativeStruct(str, argCount, threadFrame->ctf->stackTop - argCount);
                threadFrame->ctf->stackTop -= argCount + 1;
                push(result);
                return true;
            }

            default:
                // Non-callable object type.
                break;
        }
    }

    runtimeError("Can only call functions and classes.");
    return false;
}

static bool hasExtension(Value receiver, ObjString *name)
{
    char *typeStr = valueType(receiver);
    ObjString *type = AS_STRING(STRING_VAL(typeStr));
    mp_free(typeStr);

    Value value;
    if (!tableGet(&vm.extensions, type, &value))
    {
        return false;
    }

    Value result;
    if (!dictContains(value, OBJ_VAL(name), &result))
        return false;
    return AS_BOOL(result);
}

static void defineExtension(ObjString *name, ObjString *type)
{
    Value value;
    if (!tableGet(&vm.extensions, type, &value))
    {
        ObjDict *dict = initDict();
        value = OBJ_VAL(dict);
        tableSet(&vm.extensions, type, value);
    }

    ObjDict *extensions = AS_DICT(value);

    value = peek(0);
    insertDict(extensions, name->chars, value);
    pop();
}

static bool callExtension(Value receiver, ObjString *name, int argCount)
{
    char *typeStr = valueType(receiver);
    ObjString *type = AS_STRING(STRING_VAL(typeStr));
    mp_free(typeStr);

    Value value;
    if (!tableGet(&vm.extensions, type, &value))
    {
        return false;
    }

    Value fn = searchDict(AS_DICT(value), name->chars);
    return call(AS_CLOSURE(fn), argCount, NULL, NULL);
}

static bool invokeFromClass(ObjClass *klass, ObjString *name, int argCount, ObjInstance *instance)
{
    // Look for the method.
    Value method;
    ObjClass *selected;
    if (!findMethod(klass, name, &method, &selected))
    {
        runtimeError("Undefined property '%s'.", name->chars);
        return false;
    }

    ThreadFrame *threadFrame = currentThread();
    threadFrame->frame->nextModule = klass->module ? klass->module : threadFrame->frame->module;
    return call(AS_CLOSURE(method), argCount, instance, selected);
}

static bool invoke(ObjString *name, int argCount)
{
    Value receiver = peek(argCount);

    if (IS_CLASS(receiver))
    {
        ObjClass *klass = AS_CLASS(receiver);
        Value method;
        if (!tableGet(&klass->methods, name, &method))
        {
            if (hasExtension(receiver, name))
            {
                return callExtension(receiver, name, argCount);
            }
            runtimeError("Undefined property '%s'.", name->chars);
            return false;
        }

        if (!AS_CLOSURE(method)->function->staticMethod)
        {
            runtimeError("'%s' is not static. Only static methods can be "
                         "invoked directly from a class.",
                         name->chars);
            return false;
        }

        ThreadFrame *threadFrame = currentThread();
        threadFrame->frame->nextModule = klass->module ? klass->module : threadFrame->frame->module;
        return callValue(method, argCount, NULL, klass);
    }

    else if (IS_MODULE(receiver))
    {
        ObjModule *module = AS_MODULE(receiver);
        Value func;
        if (!tableGet(&module->symbols, name, &func))
        {
            runtimeError("Undefined field '%s' in '%s'.", name->chars, module->name->chars);
            return false;
        }

        ThreadFrame *threadFrame = currentThread();
        threadFrame->frame->nextModule = module;
        return callValue(func, argCount, NULL, NULL);
    }
    else if (hasExtension(receiver, name))
        return callExtension(receiver, name, argCount);
    else if (IS_LIST(receiver))
        return listMethods(name->chars, argCount + 1);
    else if (IS_DICT(receiver))
        return dictMethods(name->chars, argCount + 1);
    else if (IS_STRING(receiver))
        return stringMethods(name->chars, argCount + 1);
    else if (IS_BYTES(receiver))
        return bytesMethods(name->chars, argCount + 1);
    else if (IS_FILE(receiver))
        return fileMethods(name->chars, argCount + 1);
    else if (IS_PROCESS(receiver))
        return processesMethods(name->chars, argCount + 1);
    else if (IS_ENUM(receiver))
        return enumMethods(name->chars, argCount + 1);
    else if (IS_ENUM_VALUE(receiver))
        return enumValueMethods(name->chars, argCount + 1);
    else if (IS_NATIVE_LIB(receiver))
        return nativeLibMethods(name->chars, argCount + 1);
    else if (IS_TASK(receiver))
        return taskMethods(name->chars, argCount + 1);

    if (!IS_INSTANCE(receiver))
    {
        runtimeError("Only instances have methods.");
        return false;
    }

    ObjInstance *instance = AS_INSTANCE(receiver);

    // First look for a field which may shadow a method.
    Value value;
    if (tableGet(&instance->fields, name, &value))
    {
        ThreadFrame *threadFrame = currentThread();
        threadFrame->ctf->stackTop[-argCount - 1] = value;
        threadFrame->frame->nextModule = instance->klass->module ? instance->klass->module : threadFrame->frame->module;
        return callValue(value, argCount, instance, instance->klass);
    }

    return invokeFromClass(instance->klass, name, argCount, instance);
}

static bool bindMethod(ObjClass *klass, ObjString *name)
{
    Value method;
    if (!tableGet(&klass->methods, name, &method))
    {
        runtimeError("Undefined property '%s'.", name->chars);
        return false;
    }

    ObjBoundMethod *bound = newBoundMethod(peek(0), AS_CLOSURE(method));
    pop(); // Instance.
    push(OBJ_VAL(bound));
    return true;
}

static bool enumContains(Value container, Value item, Value *result)
{
    if (!IS_STRING(item))
    {
        return false;
    }

    ObjEnum *enume = AS_ENUM(container);
    ObjString *name = AS_STRING(item);

    Value value;
    if (tableGet(&enume->members, name, &value))
    {
        *result = TRUE_VAL;
        return true;
    }

    *result = FALSE_VAL;
    return true;
}

static bool classContains(Value container, Value item, Value *result)
{
    if (!IS_STRING(item))
    {
        return false;
    }

    ObjClass *klass = AS_CLASS(container);
    ObjString *name = AS_STRING(item);

    Value value;
    if (tableGet(&klass->fields, name, &value))
    {
        *result = TRUE_VAL;
        return true;
    }

    if (tableGet(&klass->methods, name, &value))
    {
        *result = TRUE_VAL;
        return true;
    }

    if (tableGet(&klass->staticFields, name, &value))
    {
        *result = TRUE_VAL;
        return true;
    }

    *result = FALSE_VAL;
    return true;
}

static bool instanceContains(Value container, Value item, Value *result)
{
    if (!IS_STRING(item))
    {
        return false;
    }

    ObjInstance *instance = AS_INSTANCE(container);
    ObjString *name = AS_STRING(item);

    Value value;
    if (tableGet(&instance->fields, name, &value))
    {
        *result = TRUE_VAL;
        return true;
    }

    return classContains(OBJ_VAL(instance->klass), item, result);
}

static ObjUpvalue *captureUpvalue(Value *local)
{
    ThreadFrame *threadFrame = currentThread();
    ObjUpvalue *prevUpvalue = NULL;
    ObjUpvalue *upvalue = threadFrame->ctf->openUpvalues;

    while (upvalue != NULL && upvalue->location > local)
    {
        prevUpvalue = upvalue;
        upvalue = upvalue->next;
    }

    if (upvalue != NULL && upvalue->location == local)
        return upvalue;

    ObjUpvalue *createdUpvalue = newUpvalue(local);
    createdUpvalue->next = upvalue;

    if (prevUpvalue == NULL)
    {
        threadFrame->ctf->openUpvalues = createdUpvalue;
    }
    else
    {
        prevUpvalue->next = createdUpvalue;
    }

    return createdUpvalue;
}

static void closeUpvalues(Value *last)
{
    ThreadFrame *threadFrame = currentThread();
    while (threadFrame->ctf->openUpvalues != NULL && threadFrame->ctf->openUpvalues->location >= last)
    {
        ObjUpvalue *upvalue = threadFrame->ctf->openUpvalues;
        upvalue->closed = *upvalue->location;
        upvalue->location = &upvalue->closed;
        threadFrame->ctf->openUpvalues = upvalue->next;
    }
}

static bool checkTry(CallFrame *frame)
{
    ThreadFrame *threadFrame = currentThread();
    if (threadFrame->ctf->tryFrame != NULL)
    {
        uint8_t *ip = threadFrame->ctf->tryFrame->ip;
        CallFrame *savedFrame = threadFrame->ctf->tryFrame->frame;
        popTry();

        while (savedFrame != frame)
        {
            // CallFrame *frame = &threadFrame->ctf->frames[threadFrame->ctf->frameCount - 1];
            // threadFrame->frame = frame;
            closeUpvalues(frame->slots);
            threadFrame->ctf->frameCount--;

            if (threadFrame->ctf->frameCount == threadFrame->ctf->currentFrameCount)
            {
                threadFrame->ctf->currentScriptName = vm.scriptName;
                threadFrame->ctf->currentFrameCount = -1;
            }

            threadFrame->ctf->stackTop = frame->slots;

            frame = &threadFrame->ctf->frames[threadFrame->ctf->frameCount - 1];
        }

        frame->ip = ip;

        bool hasCatch = READ_BYTE() == OP_TRUE;

        if (hasCatch)
        {
            SKIP_BYTE();
            ObjFunction *function = AS_FUNCTION(READ_CONSTANT());
            ObjClosure *closure = newClosure(function);
            push(OBJ_VAL(closure));
            for (int i = 0; i < closure->upvalueCount; i++)
            {
                uint8_t isLocal = READ_BYTE();
                uint16_t index = READ_SHORT();
                if (isLocal)
                {
                    closure->upvalues[i] = captureUpvalue(frame->slots + index);
                }
                else
                {
                    closure->upvalues[i] = frame->closure->upvalues[index];
                }
            }

            push(STRING_VAL(threadFrame->ctf->error));
        }

        mp_free(threadFrame->ctf->error);
        threadFrame->ctf->error = NULL;

        return true;
    }
    else
    {
        resetStack();
        fputs(threadFrame->ctf->error, stderr);
        fputs("\n", stderr);
        mp_free(threadFrame->ctf->error);
        threadFrame->ctf->error = NULL;
        vm.repl = NULL_VAL;
        vm.print = true;
    }

    return false;
}

static void defineMethod(ObjString *name)
{
    Value method = peek(0);
    if (IS_CLASS(peek(1)))
    {
        ObjClass *klass = AS_CLASS(peek(1));
        tableSet(&klass->methods, name, method);
    }
    pop();
    pop();
}

static void defineProperty(ObjString *name)
{
    bool isStatic = AS_BOOL(pop());

    Value value = peek(0);
    if (IS_CLASS(peek(1)))
    {
        ObjClass *klass = AS_CLASS(peek(1));
        if (isStatic)
            tableSet(&klass->staticFields, name, value);
        else
            tableSet(&klass->fields, name, value);
    }
    else if (IS_ENUM(peek(1)))
    {
        ObjEnum *enume = AS_ENUM(peek(1));
        if (IS_NULL(value))
        {
            if (IS_NUMBER(enume->last))
            {
                value = NUMBER_VAL(AS_NUMBER(enume->last) + 1);
                enume->last = value;
            }
            else
            {
                value = NUMBER_VAL(enume->members.count);
                enume->last = NULL_VAL;
            }
        }
        else
        {
            if (IS_NUMBER(value))
                enume->last = value;
            else
                enume->last = NULL_VAL;
        }
        ObjEnumValue *enumValue = newEnumValue(enume, name, value);
        tableSet(&enume->members, name, OBJ_VAL(enumValue));
    }
    pop();
    pop();
}

bool isFalsey(Value value)
{
    return IS_NULL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

static bool compareStrings(const char *op)
{
    ObjString *b = AS_STRING(peek(0));
    ObjString *a = AS_STRING(peek(1));
    bool valid = false;
    Value res = FALSE_VAL;
    int rc = strcmp(a->chars, b->chars);

    if (strcmp(op, "<") == 0)
    {
        if (rc < 0)
            res = TRUE_VAL;
        valid = true;
    }
    else if (strcmp(op, ">") == 0)
    {
        if (rc > 0)
            res = TRUE_VAL;
        valid = true;
    }
    else if (strcmp(op, "<=") == 0)
    {
        if (rc <= 0)
            res = TRUE_VAL;
        valid = true;
    }
    else if (strcmp(op, ">=") == 0)
    {
        if (rc >= 0)
            res = TRUE_VAL;
        valid = true;
    }

    if (valid)
    {
        pop();
        pop();
        push(res);
    }
    return valid;
}

static void concatenate()
{
    Value second = peek(0);
    Value first = peek(1);

    ObjString *a;
    if (IS_STRING(first))
        a = AS_STRING(first);
    else
        a = AS_STRING(toString(first));

    ObjString *b;
    if (IS_STRING(second))
        b = AS_STRING(second);
    else
        b = AS_STRING(toString(second));

    int length = a->length + b->length;
    char *chars = ALLOCATE(char, length + 1);
    memcpy(chars, a->chars, a->length);
    memcpy(chars + a->length, b->chars, b->length);
    chars[length] = '\0';

    ObjString *result = takeString(chars, length);
    pop();
    pop();
    push(OBJ_VAL(result));
}

static Value concatenateInplace(Value first, Value second)
{
    ObjString *a;
    if (IS_STRING(first))
        a = AS_STRING(first);
    else
        a = AS_STRING(toString(first));

    ObjString *b;
    if (IS_STRING(second))
        b = AS_STRING(second);
    else
        b = AS_STRING(toString(second));

    int length = a->length + b->length;
    char *chars = ALLOCATE(char, length + 1);
    memcpy(chars, a->chars, a->length);
    memcpy(chars + a->length, b->chars, b->length);
    chars[length] = '\0';

    ObjString *result = takeString(chars, length);
    return OBJ_VAL(result);
}

static double valueAsDouble(Value v)
{
    double d;
    if (IS_STRING(v))
        d = AS_CSTRING(v)[0];
    else
        d = AS_NUMBER(v);
    return d;
}

static bool operateValues(Value a, Value b, Value *r, const char *op)
{
    if (strcmp(op, "+") == 0)
    {
        if (IS_STRING(a) || IS_STRING(b))
        {
            *r = concatenateInplace(a, b);
        }
        else if (IS_LIST(a) && IS_LIST(b))
        {
            ObjList *listToAdd = AS_LIST(b);
            ObjList *result = initList(copyList(AS_LIST(a), true));
            stretchList(result, listToAdd);
            *r = OBJ_VAL(result);
        }
        else if (IS_LIST(a))
        {
            ObjList *list = copyList(AS_LIST(a), true);
            addList(list, b);
            *r = OBJ_VAL(list);
        }
        else if (IS_NUMBER(a) && IS_NUMBER(b))
        {
            *r = NUMBER_VAL(AS_NUMBER(a) + AS_NUMBER(b));
        }
        else
        {
            runtimeError("Operands must be numbers or strings.");
            return false;
        }
    }
    else if (strcmp(op, "-") == 0)
    {
        if (IS_LIST(a))
        {
            ObjList *list = copyList(AS_LIST(a), true);
            rmList(list, b);
            *r = OBJ_VAL(list);
        }
        else if (IS_NUMBER(a) && IS_NUMBER(b))
        {
            *r = NUMBER_VAL(AS_NUMBER(a) - AS_NUMBER(b));
        }
        else
        {
            runtimeError("Operands must be numbers or strings.");
            return false;
        }
    }
    else if (strcmp(op, "*") == 0)
    {
        if (IS_LIST(a) && IS_NUMBER(b))
        {
            int factor = AS_NUMBER(b);

            ObjList *list = NULL;
            if (factor == 0)
                list = initList();
            else
            {
                list = copyList(AS_LIST(a), true);
                ObjList *clone = copyList(AS_LIST(a), true);
                for (int i = 1; i < factor; i++)
                {
                    stretchList(list, clone);
                }
            }

            *r = OBJ_VAL(list);
        }
        else if (IS_NUMBER(a) && IS_NUMBER(b))
        {
            *r = NUMBER_VAL(AS_NUMBER(a) * AS_NUMBER(b));
        }
        else
        {
            runtimeError("Operands must be numbers or strings.");
            return false;
        }
    }
    else if (strcmp(op, "/") == 0)
    {
        if (IS_LIST(a) && IS_NUMBER(b))
        {
            int factor = AS_NUMBER(b);

            ObjList *list = copyList(AS_LIST(a), true);
            int num = list->values.count - (list->values.count / factor);
            shrinkList(list, num);

            *r = OBJ_VAL(list);
        }
        else if (IS_NUMBER(a) && IS_NUMBER(b))
        {
            *r = NUMBER_VAL(AS_NUMBER(a) / AS_NUMBER(b));
        }
        else
        {
            runtimeError("Operands must be numbers or strings.");
            return false;
        }
    }
    else if (strcmp(op, "%") == 0)
    {
        if (IS_NUMBER(a) && IS_NUMBER(b))
        {
            *r = NUMBER_VAL(fmod(AS_NUMBER(a), AS_NUMBER(b)));
        }
        else
        {
            runtimeError("Operands must be numbers or strings.");
            return false;
        }
    }
    else if (strcmp(op, "^") == 0)
    {
        if (IS_NUMBER(a) && IS_NUMBER(b))
        {
            *r = NUMBER_VAL(pow(AS_NUMBER(a), AS_NUMBER(b)));
        }
        else
        {
            runtimeError("Operands must be numbers or strings.");
            return false;
        }
    }
    else
    {
        runtimeError("Invalid operator: %s.", op);
        return false;
    }
    return true;
}

static Value getIncrement(Value a, Value b)
{
    if (!IS_INCREMENTAL(a) || !IS_INCREMENTAL(b))
    {
        return NULL_VAL;
    }

    double a_ = valueAsDouble(a);
    double b_ = valueAsDouble(b);

    double inc = 1;
    if (a_ > b_)
        inc = -1;
    return NUMBER_VAL(inc);
}

static Value increment(Value start, Value step, Value stop, bool ex)
{
    double inc = valueAsDouble(step);

    Value res = NULL_VAL;
    if (IS_STRING(start))
    {
        char strS[2];
        char *str = AS_CSTRING(start);
        strS[0] = str[0] + (char)((int)inc);
        strS[1] = '\0';
        res = STRING_VAL(strS);
    }
    else
    {
        res = NUMBER_VAL(AS_NUMBER(start) + inc);
    }

    double res_ = valueAsDouble(res);
    double stop_ = valueAsDouble(stop);
    if (inc > 0)
    {
        if (ex)
        {
            if (res_ >= stop_)
                res = NULL_VAL;
        }
        else
        {
            if (res_ > stop_)
                res = NULL_VAL;
        }
    }
    else
    {
        if (ex)
        {
            if (res_ <= stop_)
                res = NULL_VAL;
        }
        else
        {
            if (res_ < stop_)
                res = NULL_VAL;
        }
    }

    return res;
}

bool envVariable(ObjString *name, Value *value)
{
    ThreadFrame *threadFrame = currentThread();
    if (strcmp(name->chars, "__name__") == 0)
    {
        *value = STRING_VAL(threadFrame->ctf->currentScriptName);
        return true;
    }
    else if (strcmp(name->chars, vm.argsString) == 0)
    {
        *value = threadFrame->ctf->currentArgs;
        return true;
    }
    else if (strcmp(name->chars, "__ans__") == 0)
    {
        *value = vm.repl;
        return true;
    }
    else if (strcmp(name->chars, "__std__") == 0)
    {
        *value = OBJ_VAL(vm.stdModule);
        return true;
    }
    else if (strcmp(name->chars, "__root__") == 0)
    {
        *value = vm.rootPath;
        return true;
    }
    return false;
}

bool subscriptString(Value stringValue, Value indexValue, Value *result)
{
    if (!(IS_NUMBER(indexValue) || IS_LIST(indexValue)))
    {
        runtimeError("String index must be a number or a list.");
        return false;
    }

    ObjString *string = AS_STRING(stringValue);
    if (IS_NUMBER(indexValue))
    {
        int index = AS_NUMBER(indexValue);

        if (index < 0)
            index = string->length + index;
        if (index >= 0 && index < string->length)
        {
            char str[2];
            str[0] = string->chars[index];
            str[1] = '\0';
            *result = STRING_VAL(str);
            return true;
        }

        runtimeError("String index out of bounds.");
        return false;
    }
    else
    {
        ObjList *indexes = AS_LIST(indexValue);
        char *str = ALLOCATE(char, indexes->values.count + 1);
        int strI = 0;
        for (int i = 0; i < indexes->values.count; i++)
        {
            Value innerIndexValue = indexes->values.values[i];
            if (!IS_NUMBER(innerIndexValue))
            {
                FREE_ARRAY(char, str, indexes->values.count + 1);
                runtimeError("String index must be a number.");
                return false;
            }

            int index = AS_NUMBER(innerIndexValue);

            if (index < 0)
                index = string->length + index;
            if (index >= 0 && index < string->length)
            {
                str[strI] = string->chars[index];
                strI++;
            }
            else
            {
                FREE_ARRAY(char, str, indexes->values.count + 1);
                runtimeError("String index out of bounds.");
                return false;
            }
        }
        str[strI] = '\0';

        *result = STRING_VAL(str);
        return true;
    }
    return false;
}

bool subscriptBytes(Value bytesValue, Value indexValue, Value *result)
{
    if (!(IS_NUMBER(indexValue) || IS_LIST(indexValue)))
    {
        runtimeError("Bytes index must be a number or a list.");
        return false;
    }

    ObjBytes *bytes = AS_BYTES(bytesValue);
    if (IS_NUMBER(indexValue))
    {
        int index = AS_NUMBER(indexValue);

        if (index < 0)
        {
            if (bytes->length >= 0)
                index = bytes->length + index;
            else
            {
                runtimeError("Bytes index out of bounds.");
                return false;
            }
        }
        if (index >= 0 && (bytes->length < 0 || index < bytes->length))
        {
            uint8_t bs[1];
            bs[0] = bytes->bytes[index];
            *result = BYTES_VAL(bs, 1);
            return true;
        }

        runtimeError("Bytes index out of bounds.");
        return false;
    }
    else
    {
        ObjList *indexes = AS_LIST(indexValue);
        uint8_t *bs = ALLOCATE(uint8_t, indexes->values.count);
        int bI = 0;
        for (int i = 0; i < indexes->values.count; i++)
        {
            Value innerIndexValue = indexes->values.values[i];
            if (!IS_NUMBER(innerIndexValue))
            {
                FREE_ARRAY(uint8_t, bs, indexes->values.count);
                runtimeError("Bytes index must be a number.");
                return false;
            }

            int index = AS_NUMBER(innerIndexValue);

            if (index < 0)
            {
                if (bytes->length >= 0)
                    index = bytes->length + index;
                else
                {
                    runtimeError("Bytes index out of bounds.");
                    return false;
                }
            }
            if (index >= 0 && (bytes->length < 0 || index < bytes->length))
            {
                bs[bI] = bytes->bytes[index];
                bI++;
            }
            else
            {
                FREE_ARRAY(uint8_t, bs, indexes->values.count);
                runtimeError("Bytes index out of bounds.");
                return false;
            }
        }

        *result = BYTES_VAL(bs, bI);
        return true;
    }
    return false;
}

bool subscriptList(Value listValue, Value indexValue, Value *result)
{
    if (!(IS_NUMBER(indexValue) || IS_LIST(indexValue)))
    {
        runtimeError("Array index must be a number or a list.");
        return false;
    }

    ObjList *list = AS_LIST(listValue);
    if (IS_NUMBER(indexValue))
    {
        int index = AS_NUMBER(indexValue);

        if (index < 0)
            index = list->values.count + index;
        if (index >= 0 && index < list->values.count)
        {
            *result = list->values.values[index];
            return true;
        }

        runtimeError("Array index out of bounds.");
        return false;
    }
    else
    {
        ObjList *indexes = AS_LIST(indexValue);
        ObjList *res = initList();
        for (int i = 0; i < indexes->values.count; i++)
        {
            Value innerIndexValue = indexes->values.values[i];
            if (!IS_NUMBER(innerIndexValue))
            {
                runtimeError("Array index must be a number.");
                return false;
            }

            int index = AS_NUMBER(innerIndexValue);

            if (index < 0)
                index = list->values.count + index;
            if (index >= 0 && index < list->values.count)
            {
                writeValueArray(&res->values, list->values.values[index]);
            }
            else
            {
                runtimeError("Array index out of bounds.");
                return false;
            }
        }

        *result = OBJ_VAL(res);
        return true;
    }
    return false;
}

bool subscriptDict(Value dictValue, Value indexValue, Value *result)
{
    if (!IS_STRING(indexValue) && !IS_NUMBER(indexValue))
    {
        runtimeError("Dictionary key must be a string or a number.");
        return false;
    }

    ObjDict *dict = AS_DICT(dictValue);
    char *key;
    if (IS_STRING(indexValue))
        key = AS_CSTRING(indexValue);
    else
    {
        int index = AS_NUMBER(indexValue);
        key = searchDictKey(dict, index);
        if (key == NULL)
        {
            runtimeError("Invalid index for dictionary.");
            return false;
        }
        *result = STRING_VAL(key);
        return true;
    }

    *result = searchDict(dict, key);
    return true;
}

bool subscriptEnum(Value enumValue, Value indexValue, Value *result)
{
    if (!IS_STRING(indexValue) && !IS_NUMBER(indexValue))
    {
        runtimeError("Enum key must be a string or a number.");
        return false;
    }

    ObjEnum *enume = AS_ENUM(enumValue);
    char *key = NULL;
    int i = 0, _index = 0;
    Entry entry;
    Table *table = &enume->members;

    if (IS_STRING(indexValue))
        key = AS_CSTRING(indexValue);
    else
    {
        int index = AS_NUMBER(indexValue);
        _index = 0;
        while (iterateTable(table, &entry, &i))
        {
            if (entry.key == NULL)
                continue;

            if (_index == index)
            {
                key = entry.key->chars;
                break;
            }
            _index++;
        }
        if (key == NULL)
        {
            runtimeError("Invalid index for enum.");
            return false;
        }
        *result = STRING_VAL(key);
        return true;
    }

    i = 0;
    while (iterateTable(table, &entry, &i))
    {
        if (entry.key == NULL)
            continue;

        if (strcmp(entry.key->chars, key) == 0)
        {
            *result = AS_ENUM_VALUE(entry.value)->value;
            return true;
        }
    }

    *result = NULL_VAL;
    return true;
}

bool subscript(Value container, Value indexValue, Value *result)
{
    if (IS_STRING(container))
    {
        return subscriptString(container, indexValue, result);
    }
    else if (IS_BYTES(container))
    {
        return subscriptBytes(container, indexValue, result);
    }
    else if (IS_LIST(container))
    {
        return subscriptList(container, indexValue, result);
    }
    else if (IS_DICT(container))
    {
        return subscriptDict(container, indexValue, result);
    }
    else if (IS_ENUM(container))
    {
        return subscriptEnum(container, indexValue, result);
    }

    runtimeError("Only lists, dicts and string have indexes.");
    return false;
}

bool subscriptBytesAssign(Value container, Value indexValue, Value assignValue)
{
    if (!(IS_NUMBER(indexValue) || IS_LIST(indexValue)))
    {
        runtimeError("Bytes index must be a number or a list.");
        return false;
    }

    ObjBytes *value = AS_BYTES(toBytes(assignValue));

    ObjBytes *bytes = AS_BYTES(container);

    if (IS_NUMBER(indexValue))
    {
        int index = AS_NUMBER(indexValue);

        if (bytes->length >= 0)
        {
            if (index < 0)
                index = bytes->length + index;
            if (index >= 0 && index < bytes->length)
            {
                bytes->bytes =
                    GROW_ARRAY(bytes->bytes, unsigned char, bytes->length, bytes->length + value->length - 1);
                memcpy(bytes->bytes + index + value->length, bytes->bytes + index + 1, bytes->length - index - 1);
                memcpy(bytes->bytes + index, value->bytes, value->length);
                bytes->length = bytes->length + value->length - 1;
                return true;
            }
        }
        else
        {
            if (index < 0)
            {
                runtimeError("Array index out of bounds.");
                return false;
            }
            if (index >= 0)
            {
                memcpy(bytes->bytes + index, value->bytes, value->length);
                return true;
            }
        }

        runtimeError("Array index out of bounds.");
        return false;
    }
    else
    {
        ObjList *indexes = AS_LIST(indexValue);
        if (IS_BYTES(assignValue) && AS_BYTES(assignValue)->length == indexes->values.count)
        {
            ObjBytes *assignValues = AS_BYTES(assignValue);
            for (int i = 0; i < indexes->values.count; i++)
            {
                Value innerIndexValue = indexes->values.values[i];
                if (!IS_NUMBER(innerIndexValue))
                {
                    runtimeError("Array index must be a number.");
                    return false;
                }

                int index = AS_NUMBER(innerIndexValue);

                if (bytes->length >= 0)
                {
                    if (index < 0)
                        index = bytes->length + index;
                    if (index >= 0 && index < bytes->length)
                    {
                        bytes->bytes[index] = assignValues->bytes[i];
                    }
                    else
                    {
                        runtimeError("Array index out of bounds.");
                        return false;
                    }
                }
                else
                {
                    if (index >= 0)
                    {
                        bytes->bytes[index] = assignValues->bytes[i];
                    }
                    else
                    {
                        runtimeError("Array index out of bounds.");
                        return false;
                    }
                }
            }

            return true;
        }
        else
        {
            for (int i = 0; i < indexes->values.count; i++)
            {
                Value innerIndexValue = indexes->values.values[i];
                if (!IS_NUMBER(innerIndexValue))
                {
                    runtimeError("List index must be a number.");
                    return false;
                }

                int index = AS_NUMBER(innerIndexValue);

                if (bytes->length >= 0)
                {
                    if (index < 0)
                        index = bytes->length + index;
                    if (index >= 0 && index < bytes->length)
                    {
                        bytes->bytes =
                            GROW_ARRAY(bytes->bytes, unsigned char, bytes->length, bytes->length + value->length - 1);
                        memcpy(bytes->bytes + index + value->length, bytes->bytes + index + 1,
                               bytes->length - index - 1);
                        memcpy(bytes->bytes + index, value->bytes, value->length);
                        bytes->length = bytes->length + value->length - 1;
                    }
                    else
                    {
                        runtimeError("Bytes index out of bounds.");
                        return false;
                    }
                }
                else
                {
                    if (index >= 0)
                    {
                        memcpy(bytes->bytes + index, value->bytes, value->length);
                    }
                    else
                    {
                        runtimeError("Bytes index out of bounds.");
                        return false;
                    }
                }
            }

            return true;
        }
    }
    return true;
}

bool subscriptListAssign(Value container, Value indexValue, Value assignValue)
{
    if (!(IS_NUMBER(indexValue) || IS_LIST(indexValue)))
    {
        runtimeError("List index must be a number or a list.");
        return false;
    }

    ObjList *list = AS_LIST(container);

    if (IS_NUMBER(indexValue))
    {
        int index = AS_NUMBER(indexValue);

        if (index < 0)
            index = list->values.count + index;
        if (index >= 0 && index < list->values.count)
        {
            list->values.values[index] = copyValue(assignValue);
            return true;
        }

        runtimeError("Array index out of bounds.");
        return false;
    }
    else
    {
        ObjList *indexes = AS_LIST(indexValue);
        if (IS_LIST(assignValue) && AS_LIST(assignValue)->values.count == indexes->values.count)
        {
            ObjList *assignValues = AS_LIST(assignValue);
            for (int i = 0; i < indexes->values.count; i++)
            {
                Value innerIndexValue = indexes->values.values[i];
                if (!IS_NUMBER(innerIndexValue))
                {
                    runtimeError("Array index must be a number.");
                    return false;
                }

                int index = AS_NUMBER(innerIndexValue);

                if (index < 0)
                    index = list->values.count + index;
                if (index >= 0 && index < list->values.count)
                {
                    list->values.values[index] = copyValue(assignValues->values.values[i]);
                }
                else
                {
                    runtimeError("Array index out of bounds.");
                    return false;
                }
            }

            return true;
        }
        else
        {
            for (int i = 0; i < indexes->values.count; i++)
            {
                Value innerIndexValue = indexes->values.values[i];
                if (!IS_NUMBER(innerIndexValue))
                {
                    runtimeError("List index must be a number.");
                    return false;
                }

                int index = AS_NUMBER(innerIndexValue);

                if (index < 0)
                    index = list->values.count + index;
                if (index >= 0 && index < list->values.count)
                {
                    list->values.values[index] = copyValue(assignValue);
                }
                else
                {
                    runtimeError("List index out of bounds.");
                    return false;
                }
            }

            return true;
        }
    }
    return true;
}

bool subscriptDictAssign(Value dictValue, Value key, Value value)
{
    if (!IS_STRING(key) && !IS_NUMBER(key))
    {
        runtimeError("Dictionary key must be a string.");
        return false;
    }

    ObjDict *dict = AS_DICT(dictValue);
    char *keyString;

    if (IS_STRING(key))
        keyString = AS_CSTRING(key);
    else
    {
        int index = AS_NUMBER(key);
        keyString = searchDictKey(dict, index);
        if (keyString == NULL)
        {
            runtimeError("Invalid index for dictionary.");
            return false;
        }
    }

    insertDict(dict, keyString, value);

    return true;
}

bool subscriptAssign(Value container, Value indexValue, Value assignValue)
{
    if (IS_STRING(container))
    {
        runtimeError("Substring assign not implemented yet.");
        return false;
    }
    else if (IS_BYTES(container))
    {
        return subscriptBytesAssign(container, indexValue, assignValue);
        return false;
    }
    else if (IS_LIST(container))
    {
        return subscriptListAssign(container, indexValue, assignValue);
    }
    else if (IS_DICT(container))
    {
        return subscriptDictAssign(container, indexValue, assignValue);
    }
    runtimeError("Only lists, dicts and string have indexes.");
    return false;
}

bool instanceOperation(const char *op)
{
    if (!IS_INSTANCE(peek(0)) || !IS_INSTANCE(peek(1)))
    {
        if (!IS_INSTANCE(peek(1)) || (strcmp(op, "[]") != 0 && strcmp(op, "[]=") != 0))
        {
            Value value;
            if (!IS_INSTANCE(peek(1)) ||
                !findField(AS_INSTANCE(peek(1)), AS_STRING(STRING_VAL("__allow_insecure_operations")), &value))
                return false;
        }
    }

    ObjString *name = AS_STRING(STRING_VAL(op));

    ObjInstance *instance = AS_INSTANCE(peek(1));

    Value method;
    ObjClass *selected;
    if (!findMethod(instance->klass, name, &method, &selected))
    {
        return false;
    }

    return invokeFromClass(selected, name, 1, instance);
}

bool instanceOperationUnary(const char *op)
{
    if (!IS_INSTANCE(peek(0)))
    {
        return false;
    }

    ObjString *name = AS_STRING(STRING_VAL(op));

    ObjInstance *instance = AS_INSTANCE(peek(0));

    Value method;
    ObjClass *selected;
    if (!findMethod(instance->klass, name, &method, &selected))
    {
        return false;
    }

    return invokeFromClass(selected, name, 0, instance);
}

bool instanceOperationGet(const char *op)
{
    if (!IS_INSTANCE(peek(1)))
    {
        return false;
    }

    ObjString *name = AS_STRING(STRING_VAL(op));

    ObjInstance *instance = AS_INSTANCE(peek(1));

    Value method;
    ObjClass *selected;
    if (!findMethod(instance->klass, name, &method, &selected))
    {
        return false;
    }

    return invokeFromClass(selected, name, 1, instance);
}

bool instanceOperationSet(const char *op)
{
    if (!IS_INSTANCE(peek(2)))
    {
        return false;
    }

    ObjString *name = AS_STRING(STRING_VAL(op));

    ObjInstance *instance = AS_INSTANCE(peek(2));

    Value method;
    ObjClass *selected;
    if (!findMethod(instance->klass, name, &method, &selected))
    {
        return false;
    }

    return invokeFromClass(selected, name, 2, instance);
}

static bool inherits(ObjClass *klass, ObjString *name)
{
    ObjClass *current = klass;
    while (current != NULL)
    {
        if (strcmp(current->name->chars, name->chars) == 0)
            return true;
        current = current->super;
    }
    return false;
}

bool hasTask(ThreadFrame *threadFrame)
{
    TaskFrame *tf = threadFrame->taskFrame;
    if (tf == NULL)
        return false;

    while (tf != NULL)
    {
        if (!tf->finished)
            return true;
        tf = tf->next;
    }
    return false;
}

static bool nextTask()
{
    ThreadFrame *threadFrame = currentThread();
    TaskFrame *ctf = threadFrame->ctf;

    if (vm.skipWaitingTasks)
        goto analyse;

next:
    if (threadFrame->ctf->aborted)
    {
        TaskFrame *task = threadFrame->ctf;
        threadFrame->ctf = threadFrame->ctf->next;
        mp_free(task->name);
        mp_free(task);
    }
    else
    {
        if (!ctf->secure)
            threadFrame->ctf = threadFrame->ctf->next;
    }

    if (threadFrame->ctf == NULL)
        threadFrame->ctf = threadFrame->taskFrame;
    if (threadFrame->ctf->finished)
    {
        if (!hasTask(threadFrame))
        {
            return false;
        }
        else
            goto next;
    }
    else if (threadFrame->ctf->busy)
    {
        goto next;
    }
    else if (vm.skipWaitingTasks && threadFrame->ctf->waiting)
    {
        goto next;
    }

analyse:
    if (threadFrame->ctf->endTime > 0)
    {
        uint64_t t = cube_clock();
        if (t > threadFrame->ctf->endTime)
        {
            pop();
            push(NUMBER_VAL((t - threadFrame->ctf->startTime) * 1e-6));
            threadFrame->ctf->startTime = threadFrame->ctf->endTime = 0;
        }
        else
        {
            goto next;
        }
    }
    else if (threadFrame->ctf->busy)
    {
        goto next;
    }
    else if (threadFrame->ctf->finished)
    {
        goto next;
    }
    else if (vm.skipWaitingTasks && threadFrame->ctf->waiting)
    {
        goto next;
    }
    else if (threadFrame->ctf->aborted)
    {
        goto next;
    }

    return true;
}

InterpretResult checkContinue(ThreadFrame **threadFrame, CallFrame **frame)
{
    if (!vm.running)
    {
        return INTERPRET_OK;
    }

    if (!nextTask())
    {
        return INTERPRET_OK;
    }

    *threadFrame = currentThread();

    *frame = &(*threadFrame)->ctf->frames[(*threadFrame)->ctf->frameCount - 1];
    (*threadFrame)->frame = *frame;

    if ((*threadFrame)->ctf->error != NULL)
    {
        if (!checkTry(*frame))
            return INTERPRET_RUNTIME_ERROR;
    }

    if (vm.debug)
    {
        ObjFunction *function = (*frame)->closure->function;
        size_t instruction = (*frame)->ip - function->chunk.code;
        int line = getLine(&function->chunk, instruction);
        if (line != vm.debugInfo.line || strcmp(function->path, vm.debugInfo.path) != 0)
        {
            vm.debugInfo.line = line;
            vm.debugInfo.path = function->path;
            while (!vm.continueDebug)
            {
                if (vm.waitingDebug == false)
                    vm.waitingDebug = true;
                cube_wait(100);
            }
            vm.continueDebug = false;
            vm.waitingDebug = false;
        }
    }

#ifdef DEBUG_TRACE_EXECUTION
    {
        printf("Thread: %d, Task: %s\n", (*threadFrame)->id, (*threadFrame)->ctf->name);
        int st = 0;
        for (Value *slot = (*threadFrame)->ctf->stack; slot < (*threadFrame)->ctf->stackTop; slot++, st++)
        {
            printf("[ (%d) ", st);
            printValue(*slot);
            printf(" ]");
        }
        printf("\n");
        disassembleInstruction(&(*frame)->closure->function->chunk,
                               (int)((*frame)->ip - (*frame)->closure->function->chunk.code));
    }
#endif

    return INTERPRET_CONTINUE;
}

InterpretResult run()
{
    ThreadFrame *threadFrame;
    CallFrame *frame;
    InterpretResult loopResult;
    uint8_t instruction;

#ifdef USE_COMPUTED_GOTO

    static void *dispatchTable[] = {
#define OPCODE(name) &&code_##name,
#include "opcodes.h"
#undef OPCODE
    };

#define OPCASE(name) code_##name
#define DISPATCH()                                                                                                     \
    do                                                                                                                 \
    {                                                                                                                  \
        loopResult = checkContinue(&threadFrame, &frame);                                                              \
        if (loopResult != INTERPRET_CONTINUE)                                                                          \
            return loopResult;                                                                                         \
        goto *dispatchTable[instruction = READ_BYTE()];                                                                \
    } while (false)
#define INTERPRET_LOOP DISPATCH();

#else

#define OPCASE(name) case OP_##name
#define DISPATCH() goto loop
#define INTERPRET_LOOP                                                                                                 \
    loop:                                                                                                              \
    loopResult = checkContinue(&threadFrame, &frame);                                                                  \
    if (loopResult != INTERPRET_CONTINUE)                                                                              \
        return loopResult;                                                                                             \
    switch (instruction = READ_BYTE())

#endif

    while (true)
    {
        bool istrue = false;
        INTERPRET_LOOP
        {
            OPCASE(CONSTANT) :
            {
                Value constant = READ_CONSTANT();
                push(constant);
                DISPATCH();
            }

            OPCASE(NULL) : push(NULL_VAL);
            DISPATCH();
            OPCASE(TRUE) : push(BOOL_VAL(true));
            DISPATCH();
            OPCASE(FALSE) : push(BOOL_VAL(false));
            DISPATCH();

            OPCASE(STRING) :
            {
                Value constant = READ_CONSTANT();
                if (!IS_STRING(constant))
                {
                    runtimeError("Only strings allowed");
                    if (!checkTry(frame))
                        return INTERPRET_RUNTIME_ERROR;
                    else
                        DISPATCH();
                }

                int num = AS_NUMBER(pop());
                char find[64];

                ObjString *strObj = AS_STRING(constant);
                int len = strObj->length + 1024;
                int l, L, i, I, M;
                char *str = mp_malloc(sizeof(char) * len);
                strcpy(str, strObj->chars);
                L = strlen(str);
                sprintf(find, "${__%d__}", num - 1);
                char *ptr = cube_strrstr(str, find);
                while (ptr != NULL && num > 0)
                {
                    ObjString *paste = AS_STRING(toString(pop()));
                    if (strlen(str) + paste->length > len)
                    {
                        len += 1024 + paste->length;
                        str = mp_realloc(str, len);
                    }
                    i = ptr - str;
                    I = i + strlen(find);
                    l = L - I;
                    M = L - strlen(find) + paste->length;
                    memcpy(str + i + paste->length, str + I, l);
                    memcpy(str + i, paste->chars, paste->length);
                    str[M] = '\0';

                    num--;
                    sprintf(find, "${__%d__}", num - 1);
                    ptr = cube_strrstr(str, find);
                    L = strlen(str);
                }

                push(STRING_VAL(str));
                mp_free(str);
                DISPATCH();
            }

            OPCASE(EXPAND) :
            {
                Value ex = pop();
                Value stop = pop();
                Value step = pop();
                Value start = pop();
                if (IS_NULL(stop))
                {
                    stop = step;
                    step = NULL_VAL;
                }

                if (IS_NULL(step))
                {
                    step = getIncrement(start, stop);
                }

                if (!IS_INCREMENTAL(start) || !IS_INCREMENTAL(stop) || !IS_INCREMENTAL(step))
                {
                    runtimeError("Can only expand numbers and characters.");
                    if (!checkTry(frame))
                        return INTERPRET_RUNTIME_ERROR;
                    else
                        DISPATCH();
                }

                ObjList *list = initList();
                push(OBJ_VAL(list));

                while (!IS_NULL(start))
                {
                    writeValueArray(&list->values, start);
                    start = increment(start, step, stop, AS_BOOL(ex));
                }

                DISPATCH();
            }
            OPCASE(POP) : pop();
            DISPATCH();

            OPCASE(REPL_POP) :
            {
                if (frame->module == NULL)
                    vm.repl = pop();
                else
                    pop();
            }
            DISPATCH();

            OPCASE(GET_LOCAL) :
            {
                uint16_t slot = READ_SHORT();
                push(frame->slots[slot]);
                DISPATCH();
            }

            OPCASE(SET_LOCAL) :
            {
                uint16_t slot = READ_SHORT();
                frame->slots[slot] = peek(0);
                DISPATCH();
            }

            OPCASE(GET_GLOBAL) :
            {
                ObjString *name = READ_STRING();
                Value value;

                if (frame->instance != NULL)
                {
                    if (findField(frame->instance, name, &value))
                    {
                        push(value);
                        DISPATCH();
                    }
                }

                Table *table = &vm.globals;
                if (frame->module != NULL)
                {
                    ObjModule *module = frame->module;
                    while (module != NULL)
                    {
                        table = &module->symbols;
                        if (tableGet(table, name, &value))
                        {
                            push(value);
                            break;
                        }
                        module = module->parent;
                    }
                    if (module != NULL)
                        DISPATCH();
                    table = &vm.globals;
                }

                if (!tableGet(table, name, &value))
                {
                    if (!envVariable(name, &value))
                    {
                        if (frame->instance != NULL)
                        {
                            ObjClass *selected;
                            if (findMethod(frame->instance->klass, name, &value, &selected))
                            {
                                if (IS_CLOSURE(value))
                                {
                                    ObjClosure *closure = AS_CLOSURE(value);
                                    closure->instance = frame->instance;
                                }
                                push(value);
                                DISPATCH();
                            }
                        }
                        if (frame->klass != NULL)
                        {
                            ObjClass *selected;
                            if (findMethod(frame->klass, name, &value, &selected))
                            {
                                push(value);
                                DISPATCH();
                            }
                        }
                        runtimeError("Undefined variable '%s'.", name->chars);
                        if (!checkTry(frame))
                            return INTERPRET_RUNTIME_ERROR;
                        else
                            DISPATCH();
                    }
                }
                push(value);
                DISPATCH();
            }

            OPCASE(DEFINE_GLOBAL) :
            {
                ObjString *name = READ_STRING();
                Table *table = &vm.globals;
                if (frame->module != NULL)
                {
                    table = &frame->module->symbols;
                    linenoise_add_keyword_2(frame->module->name->chars, name->chars);
                }
                else
                    linenoise_add_keyword(name->chars);

                tableSet(table, name, peek(0));
                if (frame->module == NULL)
                    vm.repl = peek(0);
                pop();
                DISPATCH();
            }

            OPCASE(DEFINE_GLOBAL_FORCED) :
            {
                ObjString *name = READ_STRING();

                linenoise_add_keyword(name->chars);

                tableSet(&vm.globals, name, peek(0));
                if (frame->module == NULL)
                    vm.repl = peek(0);
                pop();
                DISPATCH();
            }

            OPCASE(SET_GLOBAL) :
            {
                ObjString *name = READ_STRING();

                if (frame->instance != NULL)
                {
                    Value value;
                    if (tableGet(&frame->instance->fields, name, &value))
                    {
                        tableSet(&frame->instance->fields, name, peek(0));
                        DISPATCH();
                    }
                    else if (tableGet(&frame->instance->klass->staticFields, name, &value))
                    {
                        tableSet(&frame->instance->klass->staticFields, name, peek(0));
                        DISPATCH();
                    }
                }

                Table *table = &vm.globals;
                if (frame->module != NULL)
                    table = &frame->module->symbols;

                if (tableSet(table, name, peek(0)))
                {
                    tableDelete(table, name); // [delete]
                    if (table != &vm.globals)
                    {
                        table = &vm.globals;
                        if (tableSet(table, name, peek(0)))
                        {
                            tableDelete(table, name); // [delete]

                            runtimeError("Undefined variable '%s'.", name->chars);
                            if (!checkTry(frame))
                                return INTERPRET_RUNTIME_ERROR;
                            else
                                DISPATCH();
                        }
                    }
                    else
                    {
                        runtimeError("Undefined variable '%s'.", name->chars);
                        if (!checkTry(frame))
                            return INTERPRET_RUNTIME_ERROR;
                        else
                            DISPATCH();
                    }
                }
                DISPATCH();
            }

            OPCASE(GET_UPVALUE) :
            {
                uint16_t slot = READ_SHORT();
                push(*frame->closure->upvalues[slot]->location);
                DISPATCH();
            }

            OPCASE(SET_UPVALUE) :
            {
                uint16_t slot = READ_SHORT();
                *frame->closure->upvalues[slot]->location = peek(0);
                DISPATCH();
            }

            OPCASE(GET_PROPERTY) :
            {
                if (!IS_INSTANCE(peek(0)) && !IS_CLASS(peek(0)) && !IS_MODULE(peek(0)) && !IS_ENUM(peek(0)) &&
                    !IS_DICT(peek(0)))
                {
                    runtimeError("Only instances, classes, enums, dicts and modules have properties.");
                    if (!checkTry(frame))
                        return INTERPRET_RUNTIME_ERROR;
                    else
                        DISPATCH();
                }

                if (IS_ENUM(peek(0)))
                {
                    ObjEnum *enume = AS_ENUM(peek(0));
                    ObjString *name = READ_STRING();
                    Value value;
                    if (tableGet(&enume->members, name, &value))
                    {
                        pop(); // enum
                        push(value);
                        DISPATCH();
                    }
                    else
                    {
                        runtimeError("Member not found on enum.");
                        if (!checkTry(frame))
                            return INTERPRET_RUNTIME_ERROR;
                        else
                            DISPATCH();
                    }
                }
                else if (IS_CLASS(peek(0)))
                {
                    ObjClass *klass = AS_CLASS(peek(0));
                    ObjString *name = READ_STRING();
                    Value value;
                    if (tableGet(&klass->staticFields, name, &value))
                    {
                        pop(); // Instance.
                        push(value);
                        DISPATCH();
                    }
                    else
                    {
                        runtimeError("Only static properties can be called without an instance.");
                        if (!checkTry(frame))
                            return INTERPRET_RUNTIME_ERROR;
                        else
                            DISPATCH();
                    }
                }
                else if (IS_MODULE(peek(0)))
                {
                    ObjModule *module = AS_MODULE(peek(0));
                    ObjString *name = READ_STRING();
                    Value value;
                    if (tableGet(&module->symbols, name, &value))
                    {
                        pop(); // Instance.
                        push(value);
                        DISPATCH();
                    }
                    else
                    {
                        runtimeError("Field not found on module.");
                        if (!checkTry(frame))
                            return INTERPRET_RUNTIME_ERROR;
                        else
                            DISPATCH();
                    }
                }
                else if (IS_DICT(peek(0)))
                {
                    Value value;
                    if (!subscript(peek(0), READ_CONSTANT(), &value))
                    {
                        runtimeError("Could not find this key in dict.");
                        if (!checkTry(frame))
                            return INTERPRET_RUNTIME_ERROR;
                        else
                            DISPATCH();
                    }

                    pop();
                    push(value);
                }
                else
                {
                    ObjInstance *instance = AS_INSTANCE(peek(0));
                    ObjString *name = READ_STRING();
                    Value value;
                    if (tableGet(&instance->fields, name, &value))
                    {
                        pop(); // Instance.
                        push(value);
                        DISPATCH();
                    }

                    if (!bindMethod(instance->klass, name))
                    {
                        if (!checkTry(frame))
                            return INTERPRET_RUNTIME_ERROR;
                        else
                            DISPATCH();
                    }
                }

                DISPATCH();
            }

            OPCASE(GET_PROPERTY_NO_POP) :
            {
                if (!IS_INSTANCE(peek(0)) && !IS_CLASS(peek(0)) && !IS_DICT(peek(0)))
                {
                    runtimeError("Only dicts, instances and classes have properties.");
                    if (!checkTry(frame))
                        return INTERPRET_RUNTIME_ERROR;
                    else
                        DISPATCH();
                }

                if (IS_CLASS(peek(0)))
                {
                    ObjClass *klass = AS_CLASS(peek(0));
                    ObjString *name = READ_STRING();
                    Value value;
                    if (tableGet(&klass->staticFields, name, &value))
                    {
                        push(value);
                        DISPATCH();
                    }
                    else
                    {
                        runtimeError("Only static properties can be called without an instance.");
                        if (!checkTry(frame))
                            return INTERPRET_RUNTIME_ERROR;
                        else
                            DISPATCH();
                    }
                }
                else if (IS_DICT(peek(0)))
                {
                    Value value;
                    if (!subscript(peek(0), READ_CONSTANT(), &value))
                    {
                        runtimeError("Could not set this key in dict.");
                        if (!checkTry(frame))
                            return INTERPRET_RUNTIME_ERROR;
                        else
                            DISPATCH();
                    }

                    push(value);
                }
                else
                {
                    ObjInstance *instance = AS_INSTANCE(peek(0));
                    ObjString *name = READ_STRING();
                    Value value;
                    if (tableGet(&instance->fields, name, &value))
                    {
                        push(value);
                        DISPATCH();
                    }

                    if (!bindMethod(instance->klass, name))
                        if (!checkTry(frame))
                            return INTERPRET_RUNTIME_ERROR;
                        else
                            DISPATCH();
                }

                DISPATCH();
            }

            OPCASE(SET_PROPERTY) :
            {
                if (!IS_INSTANCE(peek(1)) && !IS_CLASS(peek(1)) && !IS_DICT(peek(1)) && !IS_MODULE(peek(1)))
                {
                    runtimeError("Only modules, dicts, instances and classes have properties.");
                    if (!checkTry(frame))
                        return INTERPRET_RUNTIME_ERROR;
                    else
                        DISPATCH();
                }

                if (IS_CLASS(peek(1)))
                {
                    ObjClass *klass = AS_CLASS(peek(1));
                    ObjString *name = READ_STRING();
                    Value value = peek(0);
                    Value holder;
                    if (!tableGet(&klass->staticFields, name, &holder))
                    {
                        runtimeError("Only static properties can be set without an instance.");
                        if (!checkTry(frame))
                            return INTERPRET_RUNTIME_ERROR;
                        else
                            DISPATCH();
                    }
                    else
                        tableSet(&klass->staticFields, name, value);
                }
                else if (IS_MODULE(peek(1)))
                {
                    ObjModule *module = AS_MODULE(peek(1));
                    ObjString *name = READ_STRING();
                    Value value = peek(0);
                    Value holder;
                    if (!tableGet(&module->symbols, name, &holder))
                    {
                        runtimeError("Field not found on module.");
                        if (!checkTry(frame))
                            return INTERPRET_RUNTIME_ERROR;
                        else
                            DISPATCH();
                    }
                    else
                        tableSet(&module->symbols, name, value);
                }
                else if (IS_DICT(peek(1)))
                {
                    if (!subscriptAssign(peek(1), READ_CONSTANT(), peek(0)))
                    {
                        runtimeError("Could not set this key in dict.");
                        if (!checkTry(frame))
                            return INTERPRET_RUNTIME_ERROR;
                        else
                            DISPATCH();
                    }
                }
                else
                {
                    ObjInstance *instance = AS_INSTANCE(peek(1));
                    tableSet(&instance->fields, READ_STRING(), peek(0));
                }

                Value value = pop();
                pop();
                push(value);

                DISPATCH();
            }

            OPCASE(GET_SUPER) :
            {
                ObjString *name = READ_STRING();
                ObjClass *superclass = AS_CLASS(pop());

                if (!bindMethod(superclass, name))
                {
                    if (!checkTry(frame))
                        return INTERPRET_RUNTIME_ERROR;
                    else
                        DISPATCH();
                }
                DISPATCH();
            }

            OPCASE(EQUAL) :
            {
                if (instanceOperation("=="))
                {
                    frame = &threadFrame->ctf->frames[threadFrame->ctf->frameCount - 1];
                }
                else
                {

                    Value b = pop();
                    Value a = pop();
                    push(BOOL_VAL(valuesEqual(a, b)));
                }
                DISPATCH();
            }

            OPCASE(GREATER) :
            {
                BINARY_OP(BOOL_VAL, >);
                DISPATCH();
            }
            OPCASE(LESS) :
            {
                BINARY_OP(BOOL_VAL, <);
                DISPATCH();
            }
            OPCASE(ADD) :
            {
                istrue = READ_BYTE() == OP_TRUE;
                if (IS_STRING(peek(1)) || IS_STRING(peek(0)))
                {
                    concatenate();
                }
                else if (IS_LIST(peek(1)) && istrue)
                {
                    Value value = peek(0);
                    ObjList *list = copyList(AS_LIST(peek(1)), true);
                    if (!IS_LIST(value))
                    {
                        for (int i = 0; i < list->values.count; i++)
                        {
                            if (!operateValues(list->values.values[i], value, &list->values.values[i], "+"))
                            {
                                if (!checkTry(frame))
                                    return INTERPRET_RUNTIME_ERROR;
                                else
                                    break;
                            }
                        }
                    }
                    else
                    {
                        ObjList *list2 = AS_LIST(value);
                        if (list2->values.count != list->values.count)
                        {
                            runtimeError("Lists size doesn't match.");
                            if (!checkTry(frame))
                                return INTERPRET_RUNTIME_ERROR;
                            else
                                DISPATCH();
                        }
                        else
                        {
                            for (int i = 0; i < list2->values.count; i++)
                            {
                                value = list2->values.values[i];
                                if (!operateValues(list->values.values[i], value, &list->values.values[i], "+"))
                                {
                                    if (!checkTry(frame))
                                        return INTERPRET_RUNTIME_ERROR;
                                    else
                                        break;
                                }
                            }
                        }
                    }
                    pop();
                    pop();
                    push(OBJ_VAL(list));
                }
                else if (IS_LIST(peek(0)) && IS_LIST(peek(1)))
                {
                    Value listToAddValue = peek(0);
                    Value listValue = peek(1);

                    ObjList *listToAdd = AS_LIST(listToAddValue);

                    ObjList *result = initList(copyList(AS_LIST(listValue), true));
                    stretchList(result, listToAdd);

                    pop();
                    pop();

                    push(OBJ_VAL(result));
                }
                else if (IS_LIST(peek(1)))
                {
                    Value add = peek(0);
                    Value listValue = peek(1);

                    ObjList *list = copyList(AS_LIST(listValue), true);
                    addList(list, add);

                    pop();
                    pop();

                    push(OBJ_VAL(list));
                }
                else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1)))
                {
                    double b = AS_NUMBER(pop());
                    double a = AS_NUMBER(pop());
                    push(NUMBER_VAL(a + b));
                }
                else
                {
                    if (istrue && instanceOperation(".+"))
                    {
                        frame = &threadFrame->ctf->frames[threadFrame->ctf->frameCount - 1];
                    }
                    else if (instanceOperation("+"))
                    {
                        frame = &threadFrame->ctf->frames[threadFrame->ctf->frameCount - 1];
                    }
                    else
                    {
                        runtimeError("Operands must be numbers or strings.");
                        if (!checkTry(frame))
                            return INTERPRET_RUNTIME_ERROR;
                        else
                            DISPATCH();
                    }
                }
                DISPATCH();
            }

            OPCASE(SUBTRACT) :
            {
                istrue = READ_BYTE() == OP_TRUE;
                if (istrue && instanceOperation(".-"))
                {
                    frame = &threadFrame->ctf->frames[threadFrame->ctf->frameCount - 1];
                }
                else if (instanceOperation("-"))
                {
                    frame = &threadFrame->ctf->frames[threadFrame->ctf->frameCount - 1];
                }
                else if (IS_LIST(peek(1)) && istrue)
                {
                    Value value = peek(0);
                    ObjList *list = copyList(AS_LIST(peek(1)), true);
                    if (!IS_LIST(value))
                    {
                        for (int i = 0; i < list->values.count; i++)
                        {
                            if (!operateValues(list->values.values[i], value, &list->values.values[i], "-"))
                            {
                                if (!checkTry(frame))
                                    return INTERPRET_RUNTIME_ERROR;
                                else
                                    break;
                            }
                        }
                    }
                    else
                    {
                        ObjList *list2 = AS_LIST(value);
                        if (list2->values.count != list->values.count)
                        {
                            runtimeError("Lists size doesn't match.");
                            if (!checkTry(frame))
                                return INTERPRET_RUNTIME_ERROR;
                            else
                                DISPATCH();
                        }
                        else
                        {
                            for (int i = 0; i < list2->values.count; i++)
                            {
                                value = list2->values.values[i];
                                if (!operateValues(list->values.values[i], value, &list->values.values[i], "-"))
                                {
                                    if (!checkTry(frame))
                                        return INTERPRET_RUNTIME_ERROR;
                                    else
                                        break;
                                }
                            }
                        }
                    }
                    pop();
                    pop();
                    push(OBJ_VAL(list));
                }
                else if (IS_LIST(peek(1)))
                {
                    Value rm = peek(0);
                    Value listValue = peek(1);

                    ObjList *list = copyList(AS_LIST(listValue), true);
                    rmList(list, rm);

                    pop();
                    pop();

                    push(OBJ_VAL(list));
                }
                else
                {
                    BINARY_OP(NUMBER_VAL, -);
                }
                DISPATCH();
            }
            OPCASE(INC) :
            {
                if (instanceOperationUnary("++"))
                {
                    frame = &threadFrame->ctf->frames[threadFrame->ctf->frameCount - 1];
                }
                else
                {
                    if (!IS_NUMBER(peek(0)))
                    {
                        runtimeError("Operand must be a number.");
                        if (!checkTry(frame))
                            return INTERPRET_RUNTIME_ERROR;
                        else
                            DISPATCH();
                    }

                    push(NUMBER_VAL(AS_NUMBER(pop()) + 1));
                }
                DISPATCH();
            }
            OPCASE(DEC) :
            {
                if (instanceOperationUnary("--"))
                {
                    frame = &threadFrame->ctf->frames[threadFrame->ctf->frameCount - 1];
                }
                else
                {
                    if (!IS_NUMBER(peek(0)))
                    {
                        runtimeError("Operand must be a number.");
                        if (!checkTry(frame))
                            return INTERPRET_RUNTIME_ERROR;
                        else
                            DISPATCH();
                    }

                    push(NUMBER_VAL(AS_NUMBER(pop()) - 1));
                }
                DISPATCH();
            }
            OPCASE(MULTIPLY) : istrue = READ_BYTE() == OP_TRUE;
            if (istrue && instanceOperation(".*"))
            {
                frame = &threadFrame->ctf->frames[threadFrame->ctf->frameCount - 1];
            }
            else if (instanceOperation("*"))
            {
                frame = &threadFrame->ctf->frames[threadFrame->ctf->frameCount - 1];
            }
            else if (IS_LIST(peek(1)) && istrue)
            {
                Value value = peek(0);
                ObjList *list = copyList(AS_LIST(peek(1)), true);
                if (!IS_LIST(value))
                {
                    for (int i = 0; i < list->values.count; i++)
                    {
                        if (!operateValues(list->values.values[i], value, &list->values.values[i], "*"))
                        {
                            if (!checkTry(frame))
                                return INTERPRET_RUNTIME_ERROR;
                            else
                                break;
                        }
                    }
                }
                else
                {
                    ObjList *list2 = AS_LIST(value);
                    if (list2->values.count != list->values.count)
                    {
                        runtimeError("Lists size doesn't match.");
                        if (!checkTry(frame))
                            return INTERPRET_RUNTIME_ERROR;
                        else
                            DISPATCH();
                    }
                    else
                    {
                        for (int i = 0; i < list2->values.count; i++)
                        {
                            value = list2->values.values[i];
                            if (!operateValues(list->values.values[i], value, &list->values.values[i], "*"))
                            {
                                if (!checkTry(frame))
                                    return INTERPRET_RUNTIME_ERROR;
                                else
                                    break;
                            }
                        }
                    }
                }
                pop();
                pop();
                push(OBJ_VAL(list));
            }
            else if (IS_LIST(peek(1)) && IS_NUMBER(peek(0)))
            {
                int factor = AS_NUMBER(peek(0));
                Value listValue = peek(1);

                ObjList *list = NULL;
                if (factor == 0)
                    list = initList();
                else
                {
                    list = copyList(AS_LIST(listValue), true);
                    ObjList *clone = copyList(AS_LIST(listValue), true);
                    for (int i = 1; i < factor; i++)
                    {
                        stretchList(list, clone);
                    }
                }

                pop();
                pop();

                push(OBJ_VAL(list));
            }
            else
            {
                BINARY_OP(NUMBER_VAL, *);
            }
            DISPATCH();
            OPCASE(DIVIDE) :
            {
                istrue = READ_BYTE() == OP_TRUE;
                if (istrue && instanceOperation("./"))
                {
                    frame = &threadFrame->ctf->frames[threadFrame->ctf->frameCount - 1];
                }
                else if (instanceOperation("/"))
                {
                    frame = &threadFrame->ctf->frames[threadFrame->ctf->frameCount - 1];
                }
                else if (IS_LIST(peek(1)) && istrue)
                {
                    Value value = peek(0);
                    ObjList *list = copyList(AS_LIST(peek(1)), true);
                    if (!IS_LIST(value))
                    {
                        for (int i = 0; i < list->values.count; i++)
                        {
                            if (!operateValues(list->values.values[i], value, &list->values.values[i], "/"))
                            {
                                if (!checkTry(frame))
                                    return INTERPRET_RUNTIME_ERROR;
                                else
                                    break;
                            }
                        }
                    }
                    else
                    {
                        ObjList *list2 = AS_LIST(value);
                        if (list2->values.count != list->values.count)
                        {
                            runtimeError("Lists size doesn't match.");
                            if (!checkTry(frame))
                                return INTERPRET_RUNTIME_ERROR;
                            else
                                DISPATCH();
                        }
                        else
                        {
                            for (int i = 0; i < list2->values.count; i++)
                            {
                                value = list2->values.values[i];
                                if (!operateValues(list->values.values[i], value, &list->values.values[i], "/"))
                                {
                                    if (!checkTry(frame))
                                        return INTERPRET_RUNTIME_ERROR;
                                    else
                                        break;
                                }
                            }
                        }
                    }
                    pop();
                    pop();
                    push(OBJ_VAL(list));
                }
                else if (IS_LIST(peek(1)) && IS_NUMBER(peek(0)))
                {
                    int factor = AS_NUMBER(peek(0));
                    Value listValue = peek(1);

                    ObjList *list = copyList(AS_LIST(listValue), true);
                    int num = list->values.count - (list->values.count / factor);
                    shrinkList(list, num);

                    pop();
                    pop();

                    push(OBJ_VAL(list));
                }
                else
                {
                    BINARY_OP(NUMBER_VAL, /);
                }
                DISPATCH();
            }

            OPCASE(NULLABLE) :
            {
                Value def = pop();
                Value cur = pop();
                if (IS_NULL(cur))
                    push(def);
                else
                    push(cur);
                DISPATCH();
            }

            OPCASE(QUESTION) :
            {
                bool hasFalse = AS_BOOL(pop());
                if (hasFalse)
                {
                    Value false_val = pop();
                    Value true_val = pop();
                    Value cond = toBool(pop());
                    if (AS_BOOL(cond) == true)
                        push(true_val);
                    else
                        push(false_val);
                }
                else
                {
                    Value val = pop();
                    Value cond = pop();
                    if (IS_NULL(cond))
                        push(val);
                    else
                        push(cond);
                }

                DISPATCH();
            }

            OPCASE(SWAP) :
            {
                Value b = peek(0);
                Value a = peek(1);
                BINARY_OP(BOOL_VAL, >);
                bool swap = AS_BOOL(pop());
                if (swap)
                {
                    push(a);
                    push(b);
                }
                else
                {
                    push(b);
                    push(a);
                }
                DISPATCH();
            }

            OPCASE(MOD) :
            {
                istrue = READ_BYTE() == OP_TRUE;
                if (istrue && instanceOperation(".%"))
                {
                    frame = &threadFrame->ctf->frames[threadFrame->ctf->frameCount - 1];
                }
                else if (instanceOperation("%"))
                {
                    frame = &threadFrame->ctf->frames[threadFrame->ctf->frameCount - 1];
                }
                else if (IS_LIST(peek(1)) && istrue)
                {
                    Value value = peek(0);
                    ObjList *list = copyList(AS_LIST(peek(1)), true);
                    if (!IS_LIST(value))
                    {
                        for (int i = 0; i < list->values.count; i++)
                        {
                            if (!operateValues(list->values.values[i], value, &list->values.values[i], "%"))
                            {
                                if (!checkTry(frame))
                                    return INTERPRET_RUNTIME_ERROR;
                                else
                                    break;
                            }
                        }
                    }
                    else
                    {
                        ObjList *list2 = AS_LIST(value);
                        if (list2->values.count != list->values.count)
                        {
                            runtimeError("Lists size doesn't match.");
                            if (!checkTry(frame))
                                return INTERPRET_RUNTIME_ERROR;
                            else
                                DISPATCH();
                        }
                        else
                        {
                            for (int i = 0; i < list2->values.count; i++)
                            {
                                value = list2->values.values[i];
                                if (!operateValues(list->values.values[i], value, &list->values.values[i], "%"))
                                {
                                    if (!checkTry(frame))
                                        return INTERPRET_RUNTIME_ERROR;
                                    else
                                        break;
                                }
                            }
                        }
                    }
                    pop();
                    pop();
                    push(OBJ_VAL(list));
                }
                else
                {
                    double b = AS_NUMBER(pop());
                    double a = AS_NUMBER(pop());

                    push(NUMBER_VAL(fmod(a, b)));
                }
                DISPATCH();
            }

            OPCASE(POW) :
            {
                istrue = READ_BYTE() == OP_TRUE;
                if (istrue && instanceOperation(".^"))
                {
                    frame = &threadFrame->ctf->frames[threadFrame->ctf->frameCount - 1];
                }
                else if (instanceOperation("^"))
                {
                    frame = &threadFrame->ctf->frames[threadFrame->ctf->frameCount - 1];
                }
                else if (IS_LIST(peek(1)) && istrue)
                {
                    Value value = peek(0);
                    ObjList *list = copyList(AS_LIST(peek(1)), true);
                    if (!IS_LIST(value))
                    {
                        for (int i = 0; i < list->values.count; i++)
                        {
                            if (!operateValues(list->values.values[i], value, &list->values.values[i], "^"))
                            {
                                if (!checkTry(frame))
                                    return INTERPRET_RUNTIME_ERROR;
                                else
                                    break;
                            }
                        }
                    }
                    else
                    {
                        ObjList *list2 = AS_LIST(value);
                        if (list2->values.count != list->values.count)
                        {
                            runtimeError("Lists size doesn't match.");
                            if (!checkTry(frame))
                                return INTERPRET_RUNTIME_ERROR;
                            else
                                DISPATCH();
                        }
                        else
                        {
                            for (int i = 0; i < list2->values.count; i++)
                            {
                                value = list2->values.values[i];
                                if (!operateValues(list->values.values[i], value, &list->values.values[i], "^"))
                                {
                                    if (!checkTry(frame))
                                        return INTERPRET_RUNTIME_ERROR;
                                    else
                                        break;
                                }
                            }
                        }
                    }
                    pop();
                    pop();
                    push(OBJ_VAL(list));
                }
                else
                {
                    double b = AS_NUMBER(pop());
                    double a = AS_NUMBER(pop());

                    push(NUMBER_VAL(pow(a, b)));
                }
                DISPATCH();
            }

            OPCASE(SHIFT_LEFT) :
            {
                if (instanceOperation("<<"))
                {
                    frame = &threadFrame->ctf->frames[threadFrame->ctf->frameCount - 1];
                }
                else
                {
                    int b = AS_NUMBER(toNumber(pop()));
                    int a = AS_NUMBER(toNumber(pop()));
                    push(NUMBER_VAL((a << b)));
                }
                DISPATCH();
            }

            OPCASE(SHIFT_RIGHT) :
            {
                if (instanceOperation(">>"))
                {
                    frame = &threadFrame->ctf->frames[threadFrame->ctf->frameCount - 1];
                }
                else
                {
                    int b = AS_NUMBER(toNumber(pop()));
                    int a = AS_NUMBER(toNumber(pop()));
                    push(NUMBER_VAL((a >> b)));
                }
                DISPATCH();
            }

            OPCASE(BINARY_AND) :
            {
                if (instanceOperation("&"))
                {
                    frame = &threadFrame->ctf->frames[threadFrame->ctf->frameCount - 1];
                }
                else
                {
                    int b = 0;
                    int a = 0;
                    if (IS_ENUM_VALUE(peek(0)))
                        b = AS_NUMBER(AS_ENUM_VALUE(pop())->value);
                    else
                        b = AS_NUMBER(toNumber(pop()));

                    if (IS_ENUM_VALUE(peek(0)))
                        a = AS_NUMBER(AS_ENUM_VALUE(pop())->value);
                    else
                        a = AS_NUMBER(toNumber(pop()));
                    push(NUMBER_VAL((a & b)));
                }
                DISPATCH();
            }

            OPCASE(BINARY_OR) :
            {
                if (instanceOperation("|"))
                {
                    frame = &threadFrame->ctf->frames[threadFrame->ctf->frameCount - 1];
                }
                else
                {
                    int b = 0;
                    int a = 0;
                    if (IS_ENUM_VALUE(peek(0)))
                        b = AS_NUMBER(AS_ENUM_VALUE(pop())->value);
                    else
                        b = AS_NUMBER(toNumber(pop()));

                    if (IS_ENUM_VALUE(peek(0)))
                        a = AS_NUMBER(AS_ENUM_VALUE(pop())->value);
                    else
                        a = AS_NUMBER(toNumber(pop()));
                    push(NUMBER_VAL((a | b)));
                }
                DISPATCH();
            }

            OPCASE(IN) :
            {
                Value container = peek(0);
                Value item = peek(1);
                Value result;
                if (IS_STRING(container) && IS_STRING(item))
                {
                    if (!stringContains(container, item, &result))
                    {
                        runtimeError("Could not search on string");
                        if (!checkTry(frame))
                            return INTERPRET_RUNTIME_ERROR;
                        else
                            DISPATCH();
                    }
                }
                else if (IS_BYTES(container) && IS_BYTES(item))
                {
                    if (!bytesContains(container, item, &result))
                    {
                        runtimeError("Could not search on bytes");
                        if (!checkTry(frame))
                            return INTERPRET_RUNTIME_ERROR;
                        else
                            DISPATCH();
                    }
                }
                else if (IS_LIST(container))
                {
                    if (!listContains(container, item, &result))
                    {
                        runtimeError("Could not search on list");
                        if (!checkTry(frame))
                            return INTERPRET_RUNTIME_ERROR;
                        else
                            DISPATCH();
                    }
                }
                else if (IS_DICT(container))
                {
                    if (!dictContains(container, item, &result))
                    {
                        runtimeError("Key in dict must be a string");
                        if (!checkTry(frame))
                            return INTERPRET_RUNTIME_ERROR;
                        else
                            DISPATCH();
                    }
                }
                else if (IS_INSTANCE(container))
                {
                    if (!instanceContains(container, item, &result))
                    {
                        runtimeError("Could not search on instance");
                        if (!checkTry(frame))
                            return INTERPRET_RUNTIME_ERROR;
                        else
                            DISPATCH();
                    }
                }
                else if (IS_CLASS(container))
                {
                    if (!classContains(container, item, &result))
                    {
                        runtimeError("Could not search on class");
                        if (!checkTry(frame))
                            return INTERPRET_RUNTIME_ERROR;
                        else
                            DISPATCH();
                    }
                }
                else if (IS_ENUM(container))
                {
                    if (!enumContains(container, item, &result))
                    {
                        runtimeError("Could not search on enum");
                        if (!checkTry(frame))
                            return INTERPRET_RUNTIME_ERROR;
                        else
                            DISPATCH();
                    }
                }
                else
                {
                    result = BOOL_VAL(valuesEqual(container, item));
                }

                pop();
                pop();
                push(result);
                DISPATCH();
            }

            OPCASE(IS) :
            {
                ObjString *type = AS_STRING(peek(0));
                bool not = AS_BOOL(peek(1));
                Value obj = peek(2);
                char *objType = valueType(obj);
                Value ret;
                if (strcmp(objType, type->chars) == 0)
                {
                    ret = not? FALSE_VAL : TRUE_VAL;
                }
                else
                {
                    if (IS_INSTANCE(obj))
                    {
                        ObjInstance *instance = AS_INSTANCE(obj);
                        if (inherits(instance->klass, type))
                            ret = not? FALSE_VAL : TRUE_VAL;
                        else
                            ret = not? TRUE_VAL : FALSE_VAL;
                    }
                    else if (IS_ENUM_VALUE(obj))
                    {
                        ObjEnumValue *enumValue = AS_ENUM_VALUE(obj);
                        if (strcmp(enumValue->enume->name->chars, type->chars) == 0)
                        {
                            ret = not? FALSE_VAL : TRUE_VAL;
                        }
                    }
                    else
                        ret = not? TRUE_VAL : FALSE_VAL;
                }

                pop();
                pop();
                pop();
                push(ret);

                DISPATCH();
            }

            OPCASE(NOT) : if (instanceOperationUnary("!"))
            {
                frame = &threadFrame->ctf->frames[threadFrame->ctf->frameCount - 1];
            }
            else push(BOOL_VAL(isFalsey(pop())));
            DISPATCH();

            OPCASE(NEGATE) :
            {
                if (instanceOperationUnary("-"))
                {
                    frame = &threadFrame->ctf->frames[threadFrame->ctf->frameCount - 1];
                }
                else
                {
                    if (!IS_NUMBER(peek(0)))
                    {
                        runtimeError("Operand must be a number.");
                        if (!checkTry(frame))
                            return INTERPRET_RUNTIME_ERROR;
                        else
                            DISPATCH();
                    }

                    push(NUMBER_VAL(-AS_NUMBER(pop())));
                }
                DISPATCH();
            }
            DISPATCH();

            OPCASE(JUMP) :
            {
                uint16_t offset = READ_SHORT();
                frame->ip += offset;
                DISPATCH();
            }

            OPCASE(JUMP_IF_FALSE) :
            {
                uint16_t offset = READ_SHORT();
                if (isFalsey(peek(0)))
                    frame->ip += offset;
                DISPATCH();
            }

            OPCASE(LOOP) :
            {
                uint16_t offset = READ_SHORT();
                frame->ip -= offset;
                DISPATCH();
            }

            OPCASE(BREAK) :
            {
                uint16_t offset = READ_SHORT();
                frame->ip += offset;
            }
            DISPATCH();

            OPCASE(DUP) : push(peek(0));
            DISPATCH();

            OPCASE(IMPORT) :
            {
                Value asValue = peek(0);
                ObjString *as = NULL;
                if (IS_STRING(asValue))
                {
                    as = AS_STRING(asValue);
                }

                ObjString *fileName = AS_STRING(peek(1));
                char *s = NULL;
                char *strPath;
                if (stringEndsWith(fileName->chars, "\\*") || stringEndsWith(fileName->chars, "/*"))
                {
                    char *strFileName = mp_malloc(sizeof(char) * (fileName->length + 1));
                    strcpy(strFileName, fileName->chars);
                    strFileName[fileName->length - 2] = '\0';
                    fileName = AS_STRING(STRING_VAL(strFileName));
                    mp_free(strFileName);
                    strPath = findFile(fileName->chars);
                    if (strPath == NULL)
                    {
                        runtimeError("Could not load the folder \"%s\".", fileName->chars);
                        if (!checkTry(frame))
                            return INTERPRET_RUNTIME_ERROR;
                        else
                            DISPATCH();
                    }

                    int size;
                    char **files = listFiles(strPath, &size);
                    if (size == 0 || files == NULL)
                    {
                        mp_free(strPath);
                        runtimeError("Could not load the folder \"%s\".", fileName->chars);
                        if (!checkTry(frame))
                            return INTERPRET_RUNTIME_ERROR;
                        else
                            DISPATCH();
                    }
                    int sLen = 1024;
                    s = mp_malloc(sizeof(char) * sLen);
                    s[0] = '\0';
                    char *line = mp_malloc(sizeof(char) * 1024);
                    for (int i = 0; i < size; i++)
                    {
                        if (strcmp(files[i], ".") != 0 && strcmp(files[i], "..") != 0 &&
                            stringEndsWith(files[i], ".cube"))
                        {
                            snprintf(line, 1023, "import '%s/%s';\n", strPath, files[i]);
                            if (strlen(s) + strlen(line) >= sLen)
                            {
                                sLen += 1024;
                                s = mp_realloc(s, sizeof(char) * sLen);
                            }
                            strcat(s, line);
                        }
                        mp_free(files[i]);
                    }
                    mp_free(files);
                    mp_free(line);
                }
                else if (!findAndReadFile(fileName->chars, &strPath, &s))
                {
                    mp_free(strPath);
                    runtimeError("Could not load the file \"%s\".", fileName->chars);
                    if (!checkTry(frame))
                        return INTERPRET_RUNTIME_ERROR;
                    else
                        DISPATCH();
                }
                threadFrame->ctf->currentScriptName = fileName->chars;

                char *name = NULL;

                if (as != NULL)
                {
                    if (strcmp(as->chars, "default") != 0)
                    {
                        name = (char *)mp_malloc(sizeof(char) * (as->length + 1));
                        strcpy(name, as->chars);
                    }
                }
                else
                {
                    name = getFileDisplayName(fileName->chars);
                }

                ObjFunction *function = compile(s, strPath);
                if (function == NULL)
                {
                    mp_free(s);
                    return INTERPRET_COMPILE_ERROR;
                }
                mp_free(s);
                pop();
                pop();

                ObjModule *module = frame->module;
                if (name != NULL)
                {
                    ObjString *nameStr = AS_STRING(STRING_VAL(name));
                    module = newModule(nameStr);
                    if (frame->module != NULL)
                    {
                        module->parent = frame->module;
                    }
                    linenoise_add_keyword(name);
                    if (frame->module == NULL)
                        tableSet(&vm.globals, nameStr, OBJ_VAL(module));
                    else
                        tableSet(&frame->module->symbols, nameStr, OBJ_VAL(module));
                    mp_free(name);
                }

                push(OBJ_VAL(function));
                ObjClosure *closure = newClosure(function);
                pop();
                push(OBJ_VAL(closure));

                threadFrame->ctf->currentFrameCount = threadFrame->ctf->frameCount;

                frame = &threadFrame->ctf->frames[threadFrame->ctf->frameCount++];
                frame->ip = closure->function->chunk.code;
                frame->closure = closure;
                frame->slots = threadFrame->ctf->stackTop - 1;
                frame->module = module;
                frame->type = CALL_FRAME_TYPE_MODULE;
                frame->nextModule = NULL;
                frame->require = false;

                vm.repl = OBJ_VAL(module);

                DISPATCH();
            }

            OPCASE(INCLUDE) :
            {
                ObjFunction *function = AS_FUNCTION(READ_CONSTANT());

                Value asValue = pop();
                ObjString *as = NULL;
                if (IS_STRING(asValue))
                {
                    as = AS_STRING(asValue);
                }

                ObjString *fileName = AS_STRING(pop());

                char *name = NULL;

                if (as != NULL)
                {
                    if (strcmp(as->chars, "default") != 0)
                    {
                        name = (char *)mp_malloc(sizeof(char) * (as->length + 1));
                        strcpy(name, as->chars);
                    }
                }
                else
                {
                    name = getFileDisplayName(fileName->chars);
                }

                threadFrame->ctf->currentScriptName = fileName->chars;

                ObjModule *module = frame->module;
                if (name != NULL)
                {
                    ObjString *nameStr = AS_STRING(STRING_VAL(name));
                    module = newModule(nameStr);
                    if (frame->module != NULL)
                    {
                        module->parent = frame->module;
                    }
                    linenoise_add_keyword(name);
                    if (frame->module == NULL)
                        tableSet(&vm.globals, nameStr, OBJ_VAL(module));
                    else
                        tableSet(&frame->module->symbols, nameStr, OBJ_VAL(module));
                    mp_free(name);
                }

                push(OBJ_VAL(function));
                ObjClosure *closure = newClosure(function);
                pop();
                push(OBJ_VAL(closure));

                threadFrame->ctf->currentFrameCount = threadFrame->ctf->frameCount;

                frame = &threadFrame->ctf->frames[threadFrame->ctf->frameCount++];
                frame->ip = closure->function->chunk.code;
                frame->closure = closure;
                frame->slots = threadFrame->ctf->stackTop - 1;
                frame->module = module;
                frame->type = CALL_FRAME_TYPE_MODULE;
                frame->nextModule = NULL;
                frame->require = false;

                vm.repl = OBJ_VAL(module);

                DISPATCH();
            }

            OPCASE(FROM_MODULE) :
            {
                if (!IS_STRING(peek(0)))
                {
                    runtimeError("Variable name required to import from module");
                    if (!checkTry(frame))
                        return INTERPRET_RUNTIME_ERROR;
                    else
                        DISPATCH();
                }
                if (!IS_MODULE(peek(1)))
                {
                    runtimeError("Module name required to import variables");
                    if (!checkTry(frame))
                        return INTERPRET_RUNTIME_ERROR;
                    else
                        DISPATCH();
                }

                ObjString *name = AS_STRING(peek(0));
                ObjModule *module = AS_MODULE(peek(1));

                if (strcmp(name->chars, "*") == 0)
                {
                    Entry entry;
                    int i = 0;
                    while (iterateTable(&module->symbols, &entry, &i))
                    {
                        if (entry.key == NULL)
                            continue;

                        if (frame->module == NULL)
                            tableSet(&vm.globals, entry.key, entry.value);
                        else
                            tableSet(&frame->module->symbols, entry.key, entry.value);
                    }
                }
                else
                {
                    Value value;
                    if (!tableGet(&module->symbols, name, &value))
                    {
                        runtimeError("'%s' does not exists.", name->chars);
                        if (!checkTry(frame))
                            return INTERPRET_RUNTIME_ERROR;
                        else
                            DISPATCH();
                    }

                    if (frame->module == NULL)
                        tableSet(&vm.globals, name, value);
                    else
                        tableSet(&frame->module->symbols, name, value);
                }

                pop();
                pop();
                DISPATCH();
            }

            OPCASE(REMOVE_VAR) :
            {
                if (!IS_STRING(peek(0)))
                {
                    runtimeError("Variable name must be a string");
                    if (!checkTry(frame))
                        return INTERPRET_RUNTIME_ERROR;
                    else
                        DISPATCH();
                }

                ObjString *name = AS_STRING(peek(0));
                bool success = false;

                if (frame->module == NULL)
                    success = tableDelete(&vm.globals, name);
                else
                    success = tableDelete(&frame->module->symbols, name);

                // linenoise_remove_keyword(name);

                pop();
                DISPATCH();
            }

            OPCASE(REQUIRE) :
            {
                if (!IS_STRING(peek(0)))
                {
                    runtimeError("Require argument must be an string variable.");
                    if (!checkTry(frame))
                        return INTERPRET_RUNTIME_ERROR;
                    else
                        DISPATCH();
                }

                ObjString *fileNameStr = AS_STRING(peek(0));

                char *fileName = (char *)mp_malloc(sizeof(char) * (fileNameStr->length + strlen(vm.extension) + 2));
                strcpy(fileName, fileNameStr->chars);
                if (strstr(fileName, vm.extension) == NULL &&
                    (!stringEndsWith(fileName, "\\*") && !stringEndsWith(fileName, "/*")))
                {
                    strcat(fileName, vm.extension);
                }

                int len = strlen(fileName);
                char *s = NULL;
                char *strPath;
                if (stringEndsWith(fileNameStr->chars, "\\*") || stringEndsWith(fileNameStr->chars, "/*"))
                {
                    char *strFileName = mp_malloc(sizeof(char) * (fileNameStr->length + 1));
                    strcpy(strFileName, fileNameStr->chars);
                    strFileName[fileNameStr->length - 2] = '\0';
                    fileNameStr = AS_STRING(STRING_VAL(strFileName));
                    mp_free(strFileName);
                    strPath = findFile(fileNameStr->chars);
                    mp_free(fileName);

                    if (strPath == NULL)
                    {
                        runtimeError("Could not load the folder \"%s\".", fileNameStr->chars);
                        if (!checkTry(frame))
                            return INTERPRET_RUNTIME_ERROR;
                        else
                            DISPATCH();
                    }

                    fileName = (char *)mp_malloc(sizeof(char) * (strlen(strPath) + 1));
                    strcpy(fileName, strPath);

                    int size;
                    char **files = listFiles(strPath, &size);
                    if (size == 0 || files == NULL)
                    {
                        mp_free(fileName);
                        mp_free(strPath);
                        runtimeError("Could not load the folder \"%s\".", fileNameStr->chars);
                        if (!checkTry(frame))
                            return INTERPRET_RUNTIME_ERROR;
                        else
                            DISPATCH();
                    }
                    int sLen = 1024;
                    s = mp_malloc(sizeof(char) * sLen);
                    s[0] = '\0';
                    char *line = mp_malloc(sizeof(char) * 1024);
                    for (int i = 0; i < size; i++)
                    {
                        if (strcmp(files[i], ".") != 0 && strcmp(files[i], "..") != 0 &&
                            stringEndsWith(files[i], ".cube"))
                        {
                            snprintf(line, 1023, "import '%s/%s';\n", strPath, files[i]);
                            if (strlen(s) + strlen(line) >= sLen)
                            {
                                sLen += 1024;
                                s = mp_realloc(s, sizeof(char) * sLen);
                            }
                            strcat(s, line);
                        }
                        mp_free(files[i]);
                    }
                    mp_free(files);
                    mp_free(line);
                }
                else if (!findAndReadFile(fileName, &strPath, &s))
                {
                    runtimeError("Could not load the file \"%s\".", fileName);
                    mp_free(strPath);
                    mp_free(fileName);
                    if (!checkTry(frame))
                        return INTERPRET_RUNTIME_ERROR;
                    else
                        DISPATCH();
                }
                threadFrame->ctf->currentScriptName = fileName;

                char *name = getFileDisplayName(fileName);

                ObjFunction *function = compile(s, strPath);
                if (function == NULL)
                {
                    mp_free(s);
                    mp_free(name);
                    if (!checkTry(frame))
                        return INTERPRET_COMPILE_ERROR;
                    else
                        DISPATCH();
                }
                mp_free(s);
                pop();
                pop();

                ObjModule *module = NULL;
                ObjString *nameStr = AS_STRING(STRING_VAL(name));
                module = newModule(nameStr);
                if (frame->module != NULL)
                {
                    module->parent = frame->module;
                }
                // linenoise_add_keyword(name);
                // tableSet(&vm.globals, nameStr, OBJ_VAL(module));
                if (name != NULL)
                    mp_free(name);

                push(OBJ_VAL(function));
                ObjClosure *closure = newClosure(function);
                pop();
                push(OBJ_VAL(closure));

                threadFrame->ctf->currentFrameCount = threadFrame->ctf->frameCount;

                frame = &threadFrame->ctf->frames[threadFrame->ctf->frameCount++];
                frame->ip = closure->function->chunk.code;
                frame->closure = closure;
                frame->slots = threadFrame->ctf->stackTop - 1;
                frame->type = CALL_FRAME_TYPE_MODULE;
                frame->module = module;
                frame->nextModule = NULL;
                frame->require = true;

                vm.repl = OBJ_VAL(module);

                DISPATCH();
            }

            OPCASE(NEW_LIST) :
            {
                ObjList *list = initList();
                push(OBJ_VAL(list));
                DISPATCH();
            }
            OPCASE(ADD_LIST) :
            {
                Value addValue = peek(0);
                Value listValue = peek(1);

                ObjList *list = AS_LIST(listValue);
                writeValueArray(&list->values, addValue);

                pop();
                pop();
                push(OBJ_VAL(list));
                DISPATCH();
            }

            OPCASE(NEW_DICT) :
            {
                ObjDict *dict = initDict();
                push(OBJ_VAL(dict));
                DISPATCH();
            }
            OPCASE(ADD_DICT) :
            {
                Value value = peek(0);
                Value key = peek(1);
                Value dictValue = peek(2);

                if (!IS_STRING(key))
                {
                    runtimeError("Dictionary key must be a string.");
                    if (!checkTry(frame))
                        return INTERPRET_RUNTIME_ERROR;
                    else
                        DISPATCH();
                }

                ObjDict *dict = AS_DICT(dictValue);
                char *keyString = AS_CSTRING(key);

                insertDict(dict, keyString, value);

                pop();
                pop();
                pop();
                push(OBJ_VAL(dict));
                DISPATCH();
            }
            OPCASE(SUBSCRIPT) :
            {
                Value indexValue = peek(0);
                Value listValue = peek(1);
                Value result;

                if (instanceOperationGet("[]"))
                {
                    frame = &threadFrame->ctf->frames[threadFrame->ctf->frameCount - 1];
                }
                else
                {
                    if (!subscript(listValue, indexValue, &result))
                    {
                        if (!checkTry(frame))
                            return INTERPRET_RUNTIME_ERROR;
                        else
                            DISPATCH();
                    }

                    pop();
                    pop();
                    push(result);
                }
                DISPATCH();
            }
            OPCASE(SUBSCRIPT_ASSIGN) :
            {
                Value assignValue = peek(0);
                Value indexValue = peek(1);
                Value listValue = peek(2);

                if (instanceOperationSet("[=]"))
                {
                    frame = &threadFrame->ctf->frames[threadFrame->ctf->frameCount - 1];
                }
                else
                {
                    if (!subscriptAssign(listValue, indexValue, assignValue))
                    {
                        pop();
                        pop();
                        pop();
                        push(NULL_VAL);
                        if (!checkTry(frame))
                            return INTERPRET_RUNTIME_ERROR;
                        else
                            DISPATCH();
                    }

                    pop();
                    pop();
                    pop();
                    push(NULL_VAL);
                }
                DISPATCH();
            }

            OPCASE(CALL) :
            {
                int argCount = READ_BYTE();
                Value callee = peek(argCount);
                if (IS_CLOSURE(callee) && AS_CLOSURE(callee)->instance != NULL)
                {
                    ObjClosure *closure = AS_CLOSURE(callee);
                    ObjInstance *instance = closure->instance;
                    ObjString *name = closure->function->name;
                    closure->instance = NULL;

                    ObjList *args = NULL;
                    Value arg;

                    if (argCount > 0)
                    {
                        args = initList();
                        for (int i = argCount - 1; i >= 0; i--)
                        {
                            arg = pop();
                            writeValueArray(&args->values, arg);
                        }
                    }

                    pop();

                    push(OBJ_VAL(instance));

                    if (args != NULL)
                    {
                        for (int i = args->values.count - 1; i >= 0; i--)
                        {
                            arg = args->values.values[i];
                            push(arg);
                        }
                    }

                    if (!invokeFromClass(instance->klass, closure->function->name, argCount, instance))
                    {
                        if (!checkTry(frame))
                            return INTERPRET_RUNTIME_ERROR;
                        else
                            DISPATCH();
                    }
                    frame = &threadFrame->ctf->frames[threadFrame->ctf->frameCount - 1];
                }
                else
                {
                    if (!callValue(peek(argCount), argCount, NULL, NULL))
                    {
                        if (!checkTry(frame))
                            return INTERPRET_RUNTIME_ERROR;
                        else
                            DISPATCH();
                    }

                    if (threadFrame->ctf->eval && IS_FUNCTION(peek(0)))
                    {
                        argCount = 0;

                        ObjFunction *function = AS_FUNCTION(peek(0));
                        ObjClosure *closure = newClosure(function);
                        pop();
                        push(OBJ_VAL(closure));

                        if (!callValue(peek(argCount), argCount, NULL, NULL))
                        {
                            if (!checkTry(frame))
                                return INTERPRET_RUNTIME_ERROR;
                            else
                                DISPATCH();
                        }
                    }
                    frame = &threadFrame->ctf->frames[threadFrame->ctf->frameCount - 1];
                    threadFrame->ctf->eval = false;
                }
                DISPATCH();
            }

            OPCASE(INVOKE) :
            {
                int argCount = READ_BYTE();
                ObjString *method = READ_STRING();
                if (!invoke(method, argCount))
                {
                    if (!checkTry(frame))
                        return INTERPRET_RUNTIME_ERROR;
                    else
                        DISPATCH();
                }
                frame = &threadFrame->ctf->frames[threadFrame->ctf->frameCount - 1];
                DISPATCH();
            }

            OPCASE(SUPER) :
            {
                int argCount = READ_BYTE();
                ObjString *method = READ_STRING();
                ObjClass *superclass = AS_CLASS(pop());
                if (!invokeFromClass(superclass, method, argCount, NULL))
                {
                    if (!checkTry(frame))
                        return INTERPRET_RUNTIME_ERROR;
                    else
                        DISPATCH();
                }
                frame = &threadFrame->ctf->frames[threadFrame->ctf->frameCount - 1];
                DISPATCH();
            }

            OPCASE(CLOSURE) :
            {
                ObjFunction *function = AS_FUNCTION(READ_CONSTANT());
                ObjClosure *closure = newClosure(function);
                closure->module = frame->module;
                push(OBJ_VAL(closure));
                for (int i = 0; i < closure->upvalueCount; i++)
                {
                    uint8_t isLocal = READ_BYTE();
                    uint16_t index = READ_SHORT();
                    if (isLocal)
                    {
                        closure->upvalues[i] = captureUpvalue(frame->slots + index);
                    }
                    else
                    {
                        closure->upvalues[i] = frame->closure->upvalues[index];
                    }
                }
                DISPATCH();
            }

            OPCASE(CLOSE_UPVALUE) : closeUpvalues(threadFrame->ctf->stackTop - 1);
            pop();
            DISPATCH();

            OPCASE(RETURN) :
            {
                Value result = pop();

                ObjModule *module = frame->module;
                if (frame->module != NULL)
                {
                    frame->module = NULL;
                }

                CallFrameType type = frame->type;

                closeUpvalues(frame->slots);
                threadFrame->ctf->frameCount--;

                if (threadFrame->ctf->frameCount == threadFrame->ctf->currentFrameCount)
                {
                    threadFrame->ctf->currentScriptName = vm.scriptName;
                    threadFrame->ctf->currentFrameCount = -1;
                }

                if (threadFrame->ctf->frameCount == 0)
                {
                    threadFrame->ctf->result = result;
                    threadFrame->ctf->finished = true;
                    if (threadFrame->ctf->parent != NULL)
                        threadFrame->ctf->parent->waiting = false;
                    // return INTERPRET_OK;
                    DISPATCH();
                }

                /*
                if (threadFrame->frameCount == 0)
                {
                  pop();
                  return INTERPRET_OK;
                }
                */

                threadFrame->ctf->stackTop = frame->slots;
                if (type != CALL_FRAME_TYPE_MODULE)
                {
                    push(result);
                }
                else if (frame->require && module != NULL)
                    push(OBJ_VAL(module));

                frame = &threadFrame->ctf->frames[threadFrame->ctf->frameCount - 1];
                DISPATCH();
            }

            OPCASE(CLASS) :
            {
                ObjClass *klass = newClass(READ_STRING());
                klass->module = frame->module;
                vm.repl = OBJ_VAL(klass);
                push(OBJ_VAL(klass));
            }
            DISPATCH();

            OPCASE(INHERIT) :
            {
                Value superclass = peek(1);
                if (!IS_CLASS(superclass))
                {
                    runtimeError("Superclass must be a class.");
                    if (!checkTry(frame))
                        return INTERPRET_RUNTIME_ERROR;
                    else
                        DISPATCH();
                }

                ObjClass *subclass = AS_CLASS(peek(0));
                subclass->super = AS_CLASS(superclass);
                // tableAddAll(&AS_CLASS(superclass)->methods, &subclass->methods);
                tableAddAll(&AS_CLASS(superclass)->fields, &subclass->fields);
                pop(); // Subclass.
                DISPATCH();
            }

            OPCASE(METHOD) : defineMethod(READ_STRING());
            DISPATCH();
            OPCASE(PROPERTY) : defineProperty(READ_STRING());
            DISPATCH();
            OPCASE(EXTENSION) : defineExtension(READ_STRING(), READ_STRING());
            DISPATCH();

            OPCASE(ENUM) :
            {
                ObjEnum *enume = newEnum(READ_STRING());
                vm.repl = OBJ_VAL(enume);
                push(OBJ_VAL(enume));
            }
            DISPATCH();

            OPCASE(FILE) :
            {
                Value openType = pop();
                Value fileName = pop();

                if (!IS_STRING(openType))
                {
                    runtimeError("File open type must be a string");
                    if (!checkTry(frame))
                        return INTERPRET_RUNTIME_ERROR;
                    else
                        DISPATCH();
                }

                if (!IS_STRING(fileName))
                {
                    runtimeError("Filename must be a string");
                    if (!checkTry(frame))
                        return INTERPRET_RUNTIME_ERROR;
                    else
                        DISPATCH();
                }

                ObjString *openTypeString = AS_STRING(openType);
                ObjString *fileNameString = AS_STRING(fileName);

                ObjFile *file = openFile(fileNameString->chars, openTypeString->chars);

                if (file == NULL)
                {
                    runtimeError("Unable to open file [%s]", fileNameString->chars);
                    if (!checkTry(frame))
                        return INTERPRET_RUNTIME_ERROR;
                    else
                        DISPATCH();
                }

                push(OBJ_VAL(file));
                DISPATCH();
            }
            OPCASE(NATIVE_FUNC) :
            {
                ObjNativeFunc *func = initNativeFunc();

                int arity = AS_NUMBER(pop());

                for (int i = (arity * 3) - 1; i >= 0; i -= 3)
                {
                    writeValueArray(&func->hasDefaults, peek(i - 2));
                    writeValueArray(&func->defaults, peek(i - 1));
                    writeValueArray(&func->params, peek(i));
                }

                arity *= 3;
                while (arity > 0)
                {
                    pop();
                    arity--;
                }

                func->name = AS_STRING(pop());
                func->returnType = AS_STRING(pop());

                push(OBJ_VAL(func));
                DISPATCH();
            }
            OPCASE(NATIVE_STRUCT) :
            {
                ObjNativeStruct *str = initNativeStruct();

                int members = AS_NUMBER(pop());
                for (int i = (2 * members) - 1; i >= 0; i--)
                {
                    writeValueArray(&str->types, peek(i));
                    i--;
                    writeValueArray(&str->names, peek(i));
                }

                while (members > 0)
                {
                    pop();
                    pop();
                    members--;
                }

                str->name = AS_STRING(pop());

                push(OBJ_VAL(str));
                DISPATCH();
            }
            OPCASE(NATIVE) :
            {
                ObjNativeLib *lib = initNativeLib();

                int count = AS_NUMBER(pop());
                lib->functions = 0;
                lib->structs = 0;

                while (count > 0)
                {
                    writeValueArray(&lib->objs, peek(0));
                    if (IS_NATIVE_FUNC(peek(0)))
                    {
                        ObjNativeFunc *func = AS_NATIVE_FUNC(pop());
                        func->lib = lib;
                        lib->functions++;
                    }
                    else
                    {
                        ObjNativeStruct *str = AS_NATIVE_STRUCT(pop());
                        str->lib = lib;
                        lib->structs++;
                    }
                    count--;
                }

                Value name = pop();

                lib->name = AS_STRING(pop());

                Value libVal = OBJ_VAL(lib);

                push(libVal);

                ObjString *nameStr;

                if (IS_STRING(name))
                    nameStr = AS_STRING(name);
                else
                    nameStr = AS_STRING(STRING_VAL(getFileDisplayName(lib->name->chars)));

                if (frame->module == NULL)
                    tableSet(&vm.globals, nameStr, libVal);
                else
                    tableSet(&frame->module->symbols, nameStr, libVal);

                DISPATCH();
            }
            OPCASE(ASYNC) :
            {
                char *name = (char *)mp_malloc(sizeof(char) * 32);
                name[0] = '\0';
                sprintf(name, "Task[%d-%d]", thread_id(), threadFrame->tasksCount);
                threadFrame->tasksCount++;

                ObjClosure *closure = AS_CLOSURE(pop());
                TaskFrame *tf = createTaskFrame(name);

                ObjString *strTaskName = AS_STRING(STRING_VAL(name));
                mp_free(name);

                ObjTask *task = newTask(strTaskName);

                push(OBJ_VAL(task));

                // Push the context
                *tf->stackTop = OBJ_VAL(closure);
                tf->stackTop++;

                // Add the context to the frames stack

                CallFrame *frame = &tf->frames[tf->frameCount++];
                frame->closure = closure;
                frame->ip = closure->function->chunk.code;
                frame->module = threadFrame->frame->module;
                frame->type = CALL_FRAME_TYPE_FUNCTION;
                frame->nextModule = NULL;
                frame->require = false;

                frame->slots = tf->stackTop - 1;

                ObjList *args = initList();
                tf->currentArgs = OBJ_VAL(args);

                ((ThreadFrame *)tf->threadFrame)->running = true;
                // while(true)
                // {
                //   cube_wait(100);
                // }

                DISPATCH();
            }

            OPCASE(AWAIT) :
            {
                if (!IS_TASK(peek(0)))
                {
                    runtimeError("A task is required in await.");
                    if (!checkTry(frame))
                        return INTERPRET_RUNTIME_ERROR;
                    else
                        DISPATCH();
                }

                threadFrame->ctf->waiting = true;

                ObjTask *task = AS_TASK(pop());
                TaskFrame *tf = findTaskFrame(task->name->chars);
                ThreadFrame *thf = findThreadFrame(task->name->chars);
                if (tf == NULL)
                {
                    push(NULL_VAL);
                    threadFrame->ctf->waiting = false;
                    destroyTaskFrame(task->name->chars);
                    // destroyThreadFrame(thf);
                }
                else
                {
                    if (tf->finished)
                    {
                        tf->parent = NULL;
                        push(tf->result);
                        threadFrame->ctf->waiting = false;
                        destroyTaskFrame(task->name->chars);
                        // destroyThreadFrame(thf);
                    }
                    else
                    {
                        if (threadFrame->ctf->parent == tf)
                        {
                            runtimeError("A task cannot wait for another task already waiting it.");
                            if (!checkTry(frame))
                                return INTERPRET_RUNTIME_ERROR;
                            else
                                DISPATCH();
                        }
                        else
                            tf->parent = threadFrame->ctf;
                        push(OBJ_VAL(task));
                        frame->ip--;
                    }
                }

                DISPATCH();
            }

            OPCASE(ABORT) :
            {
                if (!IS_TASK(peek(0)))
                {
                    runtimeError("A task is required in abort.");
                    if (!checkTry(frame))
                        return INTERPRET_RUNTIME_ERROR;
                    else
                        DISPATCH();
                }
                ObjTask *task = AS_TASK(pop());
                destroyTaskFrame(task->name->chars);
                DISPATCH();
            }

            OPCASE(TRY) :
            {
                uint16_t catch = READ_SHORT();
                pushTry(frame, catch);
                DISPATCH();
            }

            OPCASE(CLOSE_TRY) :
            {
                uint16_t end = READ_SHORT();
                frame->ip += end;
                popTry();
                DISPATCH();
            }

            OPCASE(SECURE_START) :
            {
                threadFrame->ctf->secure = true;
                DISPATCH();
            }

            OPCASE(SECURE_END) :
            {
                threadFrame->ctf->secure = false;
                DISPATCH();
            }

            OPCASE(PASS) : DISPATCH();
        }
        // thread_yield();
    }

    // #undef READ_BYTE
    // #undef READ_SHORT
    // #undef READ_CONSTANT
    // #undef READ_STRING
    // #undef BINARY_OP
}

InterpretResult interpret(const char *source, const char *path)
{
    ObjFunction *function = compile(source, path);
    if (function == NULL)
        return INTERPRET_COMPILE_ERROR;

    push(OBJ_VAL(function));

    ObjClosure *closure = newClosure(function);
    pop();
    push(OBJ_VAL(closure));

    if (initArgC > 0)
    {
        for (int i = initArgStart; i < initArgC; i++)
        {
            push(STRING_VAL(initArgV[i]));
        }
        callValue(OBJ_VAL(closure), initArgC - initArgStart, NULL, NULL);
        // initArgC = 0;
        // initArgStart = 0;
        // initArgV = NULL;
    }
    else
        callValue(OBJ_VAL(closure), 0, NULL, NULL);

    ThreadFrame *threadFrame = currentThread();
    threadFrame->taskFrame->finished = false;

    InterpretResult ret = run();

    pop();

    int i = 1;
loopWait:
    for (i = 1; i < MAX_THREADS; i++)
    {
        ThreadFrame *tfr = &vm.threadFrames[i];
        if (vm.threadFrames[i].taskFrame != NULL && vm.threadFrames[i].result == INTERPRET_WAIT)
        {
            break;
        }
    }
    if (i < MAX_THREADS)
    {
        goto loopWait;
        printf("Waiting: %d\n", i);
    }

    return ret;
}

InterpretResult compileCode(const char *source, const char *path, const char *output)
{
    ObjFunction *fn = compile(source, path);
    if (fn == NULL)
        return INTERPRET_COMPILE_ERROR;

    char *bcPath = NULL;
    if (output == NULL)
    {
        ALLOCATE(char, strlen(path) * 2);
        strcpy(bcPath, path);
        if (strstr(bcPath, ".cube") != NULL)
            replaceStringN(bcPath, ".cube", ".cubec", 1);
        else
            strcat(bcPath + strlen(bcPath), ".cubec");
    }
    else
        bcPath = (char *)output;

    FILE *file = fopen(bcPath, "wb");
    if (file == NULL)
    {
        printf("Could not write the byte code");
        return INTERPRET_COMPILE_ERROR;
    }

    if (!initByteCode(file))
    {
        fclose(file);
        printf("Could not write the byte code");
        return INTERPRET_COMPILE_ERROR;
    }

    if (!writeByteCode(file, OBJ_VAL(fn)))
    {
        fclose(file);
        printf("Could not write the byte code");
        return INTERPRET_COMPILE_ERROR;
    }

    if (!finishByteCode(file))
    {
        fclose(file);
        printf("Could not write the byte code");
        return INTERPRET_COMPILE_ERROR;
    }

    fclose(file);

    return INTERPRET_OK;
}

void *threadFn(void *data)
{
    ThreadFrame *threadFrame = (ThreadFrame *)data;
    threadFrame->result = INTERPRET_WAIT;
    // printf("Thread start: %d\n", thread_id());
    while (!threadFrame->destroy)
    {
        if (!threadFrame->running)
        {
            // printf("Thread wait: %d\n", thread_id());
            cube_wait(1);
        }
        else
        {
            // printf("Thread run: %d\n", thread_id());
            threadFrame->result = INTERPRET_WAIT;
            // cube_wait(1000000);
            // printf("Start\n");
            threadFrame->taskFrame->finished = false;
            threadFrame->result = run();
            pop();
            // vm.running = false;
            threadFrame->running = false;
            // printf("End\n");
        }
    }
    return NULL;
}