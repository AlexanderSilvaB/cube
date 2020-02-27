#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include <linenoise/linenoise.h>

#include "common.h"
#include "compiler.h"
#include "debug.h"
#include "object.h"
#include "memory.h"
#include "vm.h"
#include "std.h"
#include "util.h"
#include "strings.h"
#include "collections.h"
#include "files.h"
#include "scanner.h"
#include "native.h"
#include "gc.h"
#include "threads.h"
#include "mempool.h"

VM vm; // [one]

void *threadFn(void *data);

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
    vm.threadFrames[i].running = true;
    vm.threadFrames[i].id = thread_id();
    vm.threadFrames[i].result = INTERPRET_OK;
  }

  return &vm.threadFrames[i];
}

static TaskFrame *createTaskFrame(const char *name)
{
  ThreadFrame *threadFrame = createThreadFrame();

  TaskFrame *taskFrame = (TaskFrame *)mp_malloc(sizeof(TaskFrame));
  taskFrame->name = (char *)mp_malloc(sizeof(char) * (strlen(name) + 1));
  strcpy(taskFrame->name, name);
  taskFrame->next = NULL;
  taskFrame->finished = false;
  taskFrame->result = NONE_VAL;
  taskFrame->eval = false;
  taskFrame->stackTop = taskFrame->stack;
  taskFrame->frameCount = 0;
  taskFrame->openUpvalues = NULL;
  taskFrame->currentFrameCount = 0;
  taskFrame->startTime = 0;
  taskFrame->endTime = 0;
  taskFrame->tryFrame = NULL;
  taskFrame->error = NULL;
  taskFrame->currentArgs = NONE_VAL;
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

static TaskFrame *findTaskFrame(const char *name)
{
  for(int i = 0; i < MAX_THREADS; i++)
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

static void destroyTaskFrame(const char *name)
{
  for(int i = 0; i < MAX_THREADS; i++)
  {
    ThreadFrame *threadFrame = &vm.threadFrames[i];
    TaskFrame *tf = threadFrame->taskFrame;
    if (tf == NULL)
      continue;

    if (strcmp(name, tf->name) == 0)
    {
      threadFrame->taskFrame = tf->next;
      mp_free(tf->name);
      mp_free(tf);
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
        parent->next = tf->next;
        mp_free(tf->name);
        mp_free(tf);
        break;
      }
    }
  }
}

static void pushTry(CallFrame *frame, uint16_t offset)
{
  ThreadFrame *threadFrame = currentThread();
  TryFrame *try
    = (TryFrame *)mp_malloc(sizeof(TryFrame));
  try
    ->next = threadFrame->ctf->tryFrame;
  try
    ->ip = frame->ip + offset;
  try
    ->frame = frame;
  threadFrame->ctf->tryFrame = try
    ;
}

static void popTry()
{
  ThreadFrame *threadFrame = currentThread();
  TryFrame *try
    = threadFrame->ctf->tryFrame;
  threadFrame->ctf->tryFrame = try
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
      if (frame->package != NULL)
      {
        sprintf(str + strlen(str), "%s.", frame->package->name->chars);
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

static void defineNative(const char *name, NativeFn function)
{
  ThreadFrame *threadFrame = currentThread();
  push(OBJ_VAL(copyString(name, (int)strlen(name))));
  push(OBJ_VAL(newNative(function)));
  tableSet(&vm.globals, AS_STRING(threadFrame->ctf->stack[0]), threadFrame->ctf->stack[1]);
  pop();
  pop();
}

void initVM(const char *path, const char *scriptName)
{
  vm.debug = false;
  vm.ready = false;
  vm.exitCode = 0;
  vm.continueDebug = false;
  vm.waitingDebug = false;
  vm.debugInfo.line = 0;
  vm.debugInfo.path = "";
  // threadFrame->taskFrame = NULL;
  vm.running = true;
  vm.repl = NONE_VAL;
  vm.print = false;

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

  vm.initString = AS_STRING(STRING_VAL("init"));
  vm.newLine = false;

  // STD
  initStd();
  do
  {
    std_fn *stdFn = linked_list_get(stdFnList);
    if (stdFn != NULL)
    {
      defineNative(stdFn->name, stdFn->fn);
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

static bool call(ObjClosure *closure, int argCount)
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
      push(NONE_VAL);
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
  frame->package = threadFrame->frame ? threadFrame->frame->package : NULL;
  frame->nextPackage = NULL;
  frame->require = false;
  if (threadFrame->frame && threadFrame->frame->nextPackage != NULL)
  {
    frame->package = threadFrame->frame->nextPackage;
    threadFrame->frame->nextPackage = NULL;
  }

  frame->slots = threadFrame->ctf->stackTop - argCount - 1;

  ObjList *args = initList();
  for (int i = argCount - 1; i >= 0; i--)
  {
    writeValueArray(&args->values, peek(i));
  }
  threadFrame->ctf->currentArgs = OBJ_VAL(args);

  return true;
}

static bool callValue(Value callee, int argCount)
{
  ThreadFrame *threadFrame = currentThread();
  if (IS_OBJ(callee))
  {
    switch (OBJ_TYPE(callee))
    {
    case OBJ_BOUND_METHOD:
    {
      ObjBoundMethod *bound = AS_BOUND_METHOD(callee);

      // Replace the bound method with the receiver so it's in the
      // right slot when the method is called.
      threadFrame->ctf->stackTop[-argCount - 1] = bound->receiver;
      threadFrame->frame->nextPackage = threadFrame->frame->package;
      return call(bound->method, argCount);
    }

    case OBJ_CLASS:
    {
      ObjClass *klass = AS_CLASS(callee);

      // Create the instance.
      threadFrame->ctf->stackTop[-argCount - 1] = OBJ_VAL(newInstance(klass));
      // Call the initializer, if there is one.
      Value initializer;
      if (tableGet(&klass->methods, vm.initString, &initializer))
      {
        threadFrame->frame->nextPackage = klass->package ? klass->package : threadFrame->frame->package;
        return call(AS_CLOSURE(initializer), argCount);
      }
      else if (argCount != 0)
      {
        runtimeError("Expected 0 arguments but got %d.", argCount);
        return false;
      }

      return true;
    }
    case OBJ_CLOSURE:
      return call(AS_CLOSURE(callee), argCount);

    case OBJ_NATIVE:
    {
      NativeFn native = AS_NATIVE(callee);
      Value result = native(argCount, threadFrame->ctf->stackTop - argCount);
      if (IS_REQUEST(result))
      {
        ObjRequest *request = AS_REQUEST(result);
        // threadFrame->ctf->stackTop[-argCount - 1] = value;
        return callValue(request->fn, argCount - request->pops);
      }
      else
      {
        threadFrame->ctf->stackTop -= argCount + 1;
        push(result);
        return true;
      }
    }

    case OBJ_NATIVE_FUNC:
    {
      ObjNativeFunc *func = AS_NATIVE_FUNC(callee);
      Value result = callNative(func, argCount, threadFrame->ctf->stackTop - argCount);
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
  return dictContains(value, OBJ_VAL(name), &result);
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
  return call(AS_CLOSURE(fn), argCount);
}

static bool invokeFromClass(ObjClass *klass, ObjString *name,
                            int argCount)
{
  // Look for the method.
  Value method;
  if (!tableGet(&klass->methods, name, &method))
  {
    if (!tableGet(&klass->staticFields, name, &method))
    {
      runtimeError("Undefined property '%s'.", name->chars);
      return false;
    }
  }

  ThreadFrame *threadFrame = currentThread();
  threadFrame->frame->nextPackage = klass->package ? klass->package : threadFrame->frame->package;
  return call(AS_CLOSURE(method), argCount);
}

static bool invoke(ObjString *name, int argCount)
{
  Value receiver = peek(argCount);

  if (IS_CLASS(receiver))
  {
    ObjClass *instance = AS_CLASS(receiver);
    Value method;
    if (!tableGet(&instance->methods, name, &method))
    {
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
    threadFrame->frame->nextPackage = instance->package ? instance->package : threadFrame->frame->package;
    return callValue(method, argCount);
  }

  else if (IS_PACKAGE(receiver))
  {
    ObjPackage *package = AS_PACKAGE(receiver);
    Value func;
    if (!tableGet(&package->symbols, name, &func))
    {
      runtimeError("Undefined field '%s' in '%s'.", name->chars, package->name->chars);
      return false;
    }

    ThreadFrame *threadFrame = currentThread();
    threadFrame->frame->nextPackage = package;
    return callValue(func, argCount);
  }
  else if (hasExtension(receiver, name))
    return callExtension(receiver, name, argCount);
  else if (IS_LIST(receiver))
    return listMethods(name->chars, argCount + 1);
  else if (IS_DICT(receiver))
    return dictMethods(name->chars, argCount + 1);
  else if (IS_STRING(receiver))
    return stringMethods(name->chars, argCount + 1);
  else if (IS_FILE(receiver))
    return fileMethods(name->chars, argCount + 1);

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
    threadFrame->frame->nextPackage = instance->klass->package ? instance->klass->package : threadFrame->frame->package;
    return callValue(value, argCount);
  }

  return invokeFromClass(instance->klass, name, argCount);
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
  while (threadFrame->ctf->openUpvalues != NULL &&
         threadFrame->ctf->openUpvalues->location >= last)
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

    bool hasCatch = (*frame->ip++) == OP_TRUE;

    if (hasCatch)
    {
      frame->ip++;
      ObjFunction *function = AS_FUNCTION((frame->closure->function->chunk.constants.values[*frame->ip++]));
      ObjClosure *closure = newClosure(function);
      push(OBJ_VAL(closure));
      for (int i = 0; i < closure->upvalueCount; i++)
      {
        uint8_t isLocal = *frame->ip++;
        uint8_t index = *frame->ip++;
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
    vm.repl = NONE_VAL;
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
  pop();
  pop();
}

bool isFalsey(Value value)
{
  return IS_NONE(value) || (IS_BOOL(value) && !AS_BOOL(value));
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

static double valueAsDouble(Value v)
{
  double d;
  if (IS_STRING(v))
    d = AS_CSTRING(v)[0];
  else
    d = AS_NUMBER(v);
  return d;
}

static Value getIncrement(Value a, Value b)
{
  if (!IS_INCREMENTAL(a) || !IS_INCREMENTAL(b))
  {
    return NONE_VAL;
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

  Value res = NONE_VAL;
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
        res = NONE_VAL;
    }
    else
    {
      if (res_ > stop_)
        res = NONE_VAL;
    }
  }
  else
  {
    if (ex)
    {
      if (res_ <= stop_)
        res = NONE_VAL;
    }
    else
    {
      if (res_ < stop_)
        res = NONE_VAL;
    }
  }

  return res;
}

Value envVariable(ObjString *name)
{
  ThreadFrame *threadFrame = currentThread();
  if (strcmp(name->chars, "__name__") == 0)
  {
    return STRING_VAL(threadFrame->ctf->currentScriptName);
  }
  else if (strcmp(name->chars, vm.argsString) == 0)
  {
    return threadFrame->ctf->currentArgs;
  }
  return NONE_VAL;
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

bool subscript(Value container, Value indexValue, Value *result)
{
  if (IS_STRING(container))
  {
    return subscriptString(container, indexValue, result);
  }
  else if (IS_LIST(container))
  {
    return subscriptList(container, indexValue, result);
  }
  else if (IS_DICT(container))
  {
    return subscriptDict(container, indexValue, result);
  }
  runtimeError("Only lists, dicts and string have indexes.");
  return false;
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
    runtimeError("String substring not implemented yet.");
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
      return false;
  }

  ObjString *name = AS_STRING(STRING_VAL(op));

  ObjInstance *instance = AS_INSTANCE(peek(1));

  Value method;
  if (!tableGet(&instance->klass->methods, name, &method))
  {
    return false;
  }

  return invokeFromClass(instance->klass, name, 1);
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
  if (!tableGet(&instance->klass->methods, name, &method))
  {
    return false;
  }

  return invokeFromClass(instance->klass, name, 1);
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
  if (!tableGet(&instance->klass->methods, name, &method))
  {
    return false;
  }

  return invokeFromClass(instance->klass, name, 2);
}

static bool nextTask()
{
  ThreadFrame *threadFrame = currentThread();
  TaskFrame *ctf = threadFrame->ctf;

next:
  threadFrame->ctf = threadFrame->ctf->next;
  if (threadFrame->ctf == NULL)
    threadFrame->ctf = threadFrame->taskFrame;
  if (threadFrame->ctf->finished)
  {
    if (threadFrame->ctf == ctf)
      return false;
    else
      goto next;
  }
  return true;
}

#define READ_BYTE() (*frame->ip++)
#define READ_SHORT() \
  (frame->ip += 2, (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))

#define READ_CONSTANT() \
  (frame->closure->function->chunk.constants.values[READ_BYTE()])

#define READ_STRING() AS_STRING(READ_CONSTANT())

#define BINARY_OP(valueType, op)                    \
  do                                                \
  {                                                 \
    if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) \
    {                                               \
      runtimeError("Operands must be numbers.");    \
      if (!checkTry(frame))                         \
        return INTERPRET_RUNTIME_ERROR;             \
    }                                               \
    else                                            \
    {                                               \
                                                    \
      double b = AS_NUMBER(pop());                  \
      double a = AS_NUMBER(pop());                  \
      push(valueType(a op b));                      \
    }                                               \
  } while (false)

InterpretResult run()
{
  for (;;)
  {
    // if(vm.id != vm.threadFrames[0].id)
    // {
    //   printf("Other\n");
    //   continue;
    // }
    if (!vm.running)
    {
      return INTERPRET_OK;
    }
    if (!nextTask())
    {
      return INTERPRET_OK;
    }

    ThreadFrame *threadFrame = currentThread();
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
        continue;
      }
    }

    CallFrame *frame = &threadFrame->ctf->frames[threadFrame->ctf->frameCount - 1];
    threadFrame->frame = frame;

    if (threadFrame->ctf->error != NULL)
    {
      if (!checkTry(frame))
        return INTERPRET_RUNTIME_ERROR;
    }

    if (vm.debug)
    {
      ObjFunction *function = frame->closure->function;
      size_t instruction = frame->ip - function->chunk.code;
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
    printf("Thread: %d, Task: %s\n", threadFrame->id, threadFrame->ctf->name);
    int st = 0;
    for (Value *slot = threadFrame->ctf->stack; slot < threadFrame->ctf->stackTop; slot++, st++)
    {
      printf("[ (%d) ", st);
      printValue(*slot);
      printf(" ]");
    }
    printf("\n");
    disassembleInstruction(&frame->closure->function->chunk,
                           (int)(frame->ip - frame->closure->function->chunk.code));
}
#endif

    uint8_t instruction;
    switch (instruction = READ_BYTE())
    {
    case OP_CONSTANT:
    {
      Value constant = READ_CONSTANT();
      push(constant);
      break;
    }

    case OP_NONE:
      push(NONE_VAL);
      break;
    case OP_TRUE:
      push(BOOL_VAL(true));
      break;
    case OP_FALSE:
      push(BOOL_VAL(false));
      break;
    case OP_EXPAND:
    {
      Value ex = pop();
      Value stop = pop();
      Value step = pop();
      Value start = pop();
      if (IS_NONE(stop))
      {
        stop = step;
        step = NONE_VAL;
      }

      if (IS_NONE(step))
      {
        step = getIncrement(start, stop);
      }

      if (!IS_INCREMENTAL(start) || !IS_INCREMENTAL(stop) || !IS_INCREMENTAL(step))
      {
        runtimeError("Can only expand numbers and characters.");
        if (!checkTry(frame))
          return INTERPRET_RUNTIME_ERROR;
        else
          break;
      }

      ObjList *list = initList();
      push(OBJ_VAL(list));

      while (!IS_NONE(start))
      {
        writeValueArray(&list->values, start);
        start = increment(start, step, stop, AS_BOOL(ex));
      }

      break;
    }
    case OP_POP:
      pop();
      break;

    case OP_REPL_POP:
    {
      if (frame->package == NULL)
        vm.repl = pop();
      else
        pop();
    }
    break;

    case OP_GET_LOCAL:
    {
      uint8_t slot = READ_BYTE();
      push(frame->slots[slot]);
      break;
    }

    case OP_SET_LOCAL:
    {
      uint8_t slot = READ_BYTE();
      frame->slots[slot] = peek(0);
      break;
    }

    case OP_GET_GLOBAL:
    {
      ObjString *name = READ_STRING();
      Value value;

      Table *table = &vm.globals;
      if (frame->package != NULL)
      {
        ObjPackage *package = frame->package;
        while (package != NULL)
        {
          table = &package->symbols;
          if (tableGet(table, name, &value))
          {
            push(value);
            break;
          }
          package = package->parent;
        }
        if (package != NULL)
          break;
        table = &vm.globals;
      }

      if (!tableGet(table, name, &value))
      {
        value = envVariable(name);
        if (IS_NONE(value))
        {
          runtimeError("Undefined variable '%s'.", name->chars);
          if (!checkTry(frame))
            return INTERPRET_RUNTIME_ERROR;
          else
            break;
        }
      }
      push(value);
      break;
    }

    case OP_DEFINE_GLOBAL:
    {
      ObjString *name = READ_STRING();
      Table *table = &vm.globals;
      if (frame->package != NULL)
      {
        table = &frame->package->symbols;
        linenoise_add_keyword_2(frame->package->name->chars, name->chars);
      }
      else
        linenoise_add_keyword(name->chars);

      tableSet(table, name, peek(0));
      if (frame->package == NULL)
        vm.repl = peek(0);
      pop();
      break;
    }

    case OP_DEFINE_GLOBAL_FORCED:
    {
      ObjString *name = READ_STRING();

      linenoise_add_keyword(name->chars);

      tableSet(&vm.globals, name, peek(0));
      if (frame->package == NULL)
        vm.repl = peek(0);
      pop();
      break;
    }

    case OP_SET_GLOBAL:
    {
      ObjString *name = READ_STRING();
      Table *table = &vm.globals;
      if (frame->package != NULL)
        table = &frame->package->symbols;

      if (tableSet(table, name, peek(0)))
      {
        tableDelete(table, name); // [delete]
        runtimeError("Undefined variable '%s'.", name->chars);
        if (!checkTry(frame))
          return INTERPRET_RUNTIME_ERROR;
        else
          break;
      }
      break;
    }

    case OP_GET_UPVALUE:
    {
      uint8_t slot = READ_BYTE();
      push(*frame->closure->upvalues[slot]->location);
      break;
    }

    case OP_SET_UPVALUE:
    {
      uint8_t slot = READ_BYTE();
      *frame->closure->upvalues[slot]->location = peek(0);
      break;
    }

    case OP_GET_PROPERTY:
    {
      if (!IS_INSTANCE(peek(0)) && !IS_CLASS(peek(0)) && !IS_PACKAGE(peek(0)))
      {
        runtimeError("Only instances, classes and packages have properties.");
        if (!checkTry(frame))
          return INTERPRET_RUNTIME_ERROR;
        else
          break;
      }

      if (IS_CLASS(peek(0)))
      {
        ObjClass *klass = AS_CLASS(peek(0));
        ObjString *name = READ_STRING();
        Value value;
        if (tableGet(&klass->staticFields, name, &value))
        {
          pop(); // Instance.
          push(value);
          break;
        }
        else
        {
          runtimeError("Only static properties can be called without an instance.");
          if (!checkTry(frame))
            return INTERPRET_RUNTIME_ERROR;
          else
            break;
        }
      }
      else if (IS_PACKAGE(peek(0)))
      {
        ObjPackage *package = AS_PACKAGE(peek(0));
        ObjString *name = READ_STRING();
        Value value;
        if (tableGet(&package->symbols, name, &value))
        {
          pop(); // Instance.
          push(value);
          break;
        }
        else
        {
          runtimeError("Field not found on package.");
          if (!checkTry(frame))
            return INTERPRET_RUNTIME_ERROR;
          else
            break;
        }
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
          break;
        }

        if (!bindMethod(instance->klass, name))
        {
          if (!checkTry(frame))
            return INTERPRET_RUNTIME_ERROR;
          else
            break;
        }
      }

      break;
    }

    case OP_GET_PROPERTY_NO_POP:
    {
      if (!IS_INSTANCE(peek(0)) && !IS_CLASS(peek(0)))
      {
        runtimeError("Only instances and classes have properties.");
        if (!checkTry(frame))
          return INTERPRET_RUNTIME_ERROR;
        else
          break;
      }

      if (IS_CLASS(peek(0)))
      {
        ObjClass *klass = AS_CLASS(peek(0));
        ObjString *name = READ_STRING();
        Value value;
        if (tableGet(&klass->staticFields, name, &value))
        {
          push(value);
          break;
        }
        else
        {
          runtimeError("Only static properties can be called without an instance.");
          if (!checkTry(frame))
            return INTERPRET_RUNTIME_ERROR;
          else
            break;
        }
      }
      else
      {
        ObjInstance *instance = AS_INSTANCE(peek(0));
        ObjString *name = READ_STRING();
        Value value;
        if (tableGet(&instance->fields, name, &value))
        {
          push(value);
          break;
        }

        if (!bindMethod(instance->klass, name))
          if (!checkTry(frame))
            return INTERPRET_RUNTIME_ERROR;
          else
            break;
      }

      break;
    }

    case OP_SET_PROPERTY:
    {
      if (!IS_INSTANCE(peek(1)) && !IS_CLASS(peek(1)))
      {
        runtimeError("Only instances and classes have properties.");
        if (!checkTry(frame))
          return INTERPRET_RUNTIME_ERROR;
        else
          break;
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
            break;
        }
        else
          tableSet(&klass->staticFields, name, value);
      }
      else
      {
        ObjInstance *instance = AS_INSTANCE(peek(1));
        tableSet(&instance->fields, READ_STRING(), peek(0));
      }

      Value value = pop();
      pop();
      push(value);

      break;
    }

    case OP_GET_SUPER:
    {
      ObjString *name = READ_STRING();
      ObjClass *superclass = AS_CLASS(pop());
      if (!bindMethod(superclass, name))
      {
        if (!checkTry(frame))
          return INTERPRET_RUNTIME_ERROR;
        else
          break;
      }
      break;
    }

    case OP_EQUAL:
    {
      Value b = pop();
      Value a = pop();
      push(BOOL_VAL(valuesEqual(a, b)));
      break;
    }

    case OP_GREATER:
      BINARY_OP(BOOL_VAL, >);
      break;
    case OP_LESS:
    {
      BINARY_OP(BOOL_VAL, <);
      break;
    }
    case OP_ADD:
    {
      if (IS_STRING(peek(1)) || IS_STRING(peek(0)))
      {
        concatenate();
      }
      else if (IS_LIST(peek(0)) && IS_LIST(peek(1)))
      {
        Value listToAddValue = peek(0);
        Value listValue = peek(1);

        ObjList *list = AS_LIST(listValue);
        ObjList *listToAdd = AS_LIST(listToAddValue);

        ObjList *result = initList();
        for (int i = 0; i < list->values.count; ++i)
          writeValueArray(&result->values, list->values.values[i]);
        for (int i = 0; i < listToAdd->values.count; ++i)
          writeValueArray(&result->values, listToAdd->values.values[i]);

        pop();
        pop();

        push(OBJ_VAL(result));
      }
      else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1)))
      {
        double b = AS_NUMBER(pop());
        double a = AS_NUMBER(pop());
        push(NUMBER_VAL(a + b));
      }
      else
      {
        if (instanceOperation("+"))
        {
          frame = &threadFrame->ctf->frames[threadFrame->ctf->frameCount - 1];
        }
        else
        {
          runtimeError("Operands must be two numbers or two strings.");
          if (!checkTry(frame))
            return INTERPRET_RUNTIME_ERROR;
          else
            break;
        }
      }
      break;
    }

    case OP_SUBTRACT:
      if (instanceOperation("-"))
      {
        frame = &threadFrame->ctf->frames[threadFrame->ctf->frameCount - 1];
      }
      else
      {
        BINARY_OP(NUMBER_VAL, -);
      }
      break;
    case OP_INC:
    {
      if (!IS_NUMBER(peek(0)))
      {
        runtimeError("Operand must be a number.");
        if (!checkTry(frame))
          return INTERPRET_RUNTIME_ERROR;
        else
          break;
      }

      push(NUMBER_VAL(AS_NUMBER(pop()) + 1));
      break;
    }
    case OP_DEC:
    {
      if (!IS_NUMBER(peek(0)))
      {
        runtimeError("Operand must be a number.");
        if (!checkTry(frame))
          return INTERPRET_RUNTIME_ERROR;
        else
          break;
      }

      push(NUMBER_VAL(AS_NUMBER(pop()) - 1));
      break;
    }
    case OP_MULTIPLY:
      if (instanceOperation("*"))
      {
        frame = &threadFrame->ctf->frames[threadFrame->ctf->frameCount - 1];
      }
      else
      {
        BINARY_OP(NUMBER_VAL, *);
      }
      break;
    case OP_DIVIDE:
      if (instanceOperation("/"))
      {
        frame = &threadFrame->ctf->frames[threadFrame->ctf->frameCount - 1];
      }
      else
      {
        BINARY_OP(NUMBER_VAL, /);
      }
      break;

    case OP_MOD:
    {
      if (instanceOperation("%"))
      {
        frame = &threadFrame->ctf->frames[threadFrame->ctf->frameCount - 1];
      }
      else
      {
        double b = AS_NUMBER(pop());
        double a = AS_NUMBER(pop());

        push(NUMBER_VAL(fmod(a, b)));
      }
      break;
    }

    case OP_POW:
    {
      if (instanceOperation("^"))
      {
        frame = &threadFrame->ctf->frames[threadFrame->ctf->frameCount - 1];
      }
      else
      {
        double b = AS_NUMBER(pop());
        double a = AS_NUMBER(pop());

        push(NUMBER_VAL(pow(a, b)));
      }
      break;
    }

    case OP_IN:
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
            break;
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
            break;
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
            break;
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
            break;
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
            break;
        }
      }
      else
      {
        result = BOOL_VAL(valuesEqual(container, item));
      }

      pop();
      pop();
      push(result);
      break;
    }

    case OP_IS:
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
          if (strcmp(instance->klass->name->chars, type->chars) == 0)
            ret = not? FALSE_VAL : TRUE_VAL;
          else
            ret = not? TRUE_VAL : FALSE_VAL;
        }
        else
          ret = not? TRUE_VAL : FALSE_VAL;
      }

      pop();
      pop();
      pop();
      push(ret);

      break;
    }

    case OP_NOT:
      push(BOOL_VAL(isFalsey(pop())));
      break;

    case OP_NEGATE:
      if (!IS_NUMBER(peek(0)))
      {
        runtimeError("Operand must be a number.");
        if (!checkTry(frame))
          return INTERPRET_RUNTIME_ERROR;
        else
          break;
      }

      push(NUMBER_VAL(-AS_NUMBER(pop())));
      break;

    case OP_JUMP:
    {
      uint16_t offset = READ_SHORT();
      frame->ip += offset;
      break;
    }

    case OP_JUMP_IF_FALSE:
    {
      uint16_t offset = READ_SHORT();
      if (isFalsey(peek(0)))
        frame->ip += offset;
      break;
    }

    case OP_LOOP:
    {
      uint16_t offset = READ_SHORT();
      frame->ip -= offset;
      break;
    }

    case OP_BREAK:
    {
      uint16_t offset = READ_SHORT();
      frame->ip += offset;
    }
    break;

    case OP_DUP:
      push(peek(0));
      break;

    case OP_IMPORT:
    {
      Value asValue = peek(0);
      ObjString *as = NULL;
      if (IS_STRING(asValue))
      {
        as = AS_STRING(asValue);
      }

      ObjString *fileName = AS_STRING(peek(1));
      char *s = readFile(fileName->chars, false);

      char *strPath = (char *)mp_malloc(sizeof(char) * (fileName->length + 1));
      strcpy(strPath, fileName->chars);

      int index = 0;
      while (s == NULL && index < vm.paths->values.count)
      {
        ObjString *folder = AS_STRING(vm.paths->values.values[index]);
        mp_free(strPath);
        strPath = mp_malloc(sizeof(char) * (folder->length + fileName->length + 2));
        strcpy(strPath, folder->chars);
        strcat(strPath, fileName->chars);
        s = readFile(strPath, false);
        index++;
      }
      if (s == NULL)
      {
        mp_free(strPath);
        runtimeError("Could not load the file \"%s\".", fileName->chars);
        if (!checkTry(frame))
          return INTERPRET_RUNTIME_ERROR;
        else
          break;
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

      ObjPackage *package = NULL;
      if (name != NULL)
      {
        ObjString *nameStr = AS_STRING(STRING_VAL(name));
        package = newPackage(nameStr);
        if (frame->package != NULL)
        {
          package->parent = frame->package;
        }
        linenoise_add_keyword(name);
        tableSet(&vm.globals, nameStr, OBJ_VAL(package));
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
      frame->package = package;
      frame->type = CALL_FRAME_TYPE_PACKAGE;
      frame->nextPackage = NULL;
      frame->require = false;

      vm.repl = OBJ_VAL(package);

      break;
    }

    case OP_REQUIRE:
    {
      if (!IS_STRING(peek(0)))
      {
        runtimeError("Require argument must be an string variable.");
        if (!checkTry(frame))
          return INTERPRET_RUNTIME_ERROR;
        else
          break;
      }

      ObjString *fileNameStr = AS_STRING(peek(0));

      char *fileName = (char *)mp_malloc(sizeof(char) * (fileNameStr->length + strlen(vm.extension) + 2));
      strcpy(fileName, fileNameStr->chars);
      if (strstr(fileName, vm.extension) == NULL)
      {
        strcat(fileName, vm.extension);
      }

      int len = strlen(fileName);
      char *strPath = (char *)mp_malloc(sizeof(char) * (len + 1));
      strcpy(strPath, fileName);

      char *s = readFile(fileName, false);
      int index = 0;
      while (s == NULL && index < vm.paths->values.count)
      {
        ObjString *folder = AS_STRING(vm.paths->values.values[index]);
        mp_free(strPath);
        strPath = mp_malloc(sizeof(char) * (folder->length + len + 2));
        strcpy(strPath, folder->chars);
        strcat(strPath, fileName);

        s = readFile(strPath, false);
        index++;
      }
      if (s == NULL)
      {
        runtimeError("Could not load the file \"%s\".", fileName);
        mp_free(strPath);
        mp_free(fileName);
        if (!checkTry(frame))
          return INTERPRET_RUNTIME_ERROR;
        else
          break;
      }
      threadFrame->ctf->currentScriptName = fileName;

      char *name = getFileDisplayName(fileName);

      ObjFunction *function = compile(s, strPath);
      if (function == NULL)
      {
        mp_free(s);
        mp_free(name);
        return INTERPRET_COMPILE_ERROR;
      }
      mp_free(s);
      pop();
      pop();

      ObjPackage *package = NULL;
      ObjString *nameStr = AS_STRING(STRING_VAL(name));
      package = newPackage(nameStr);
      if (frame->package != NULL)
      {
        package->parent = frame->package;
      }
      // linenoise_add_keyword(name);
      // tableSet(&vm.globals, nameStr, OBJ_VAL(package));
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
      frame->type = CALL_FRAME_TYPE_PACKAGE;
      frame->package = package;
      frame->nextPackage = NULL;
      frame->require = true;

      vm.repl = OBJ_VAL(package);

      break;
    }

    case OP_NEW_LIST:
    {
      ObjList *list = initList();
      push(OBJ_VAL(list));
      break;
    }
    case OP_ADD_LIST:
    {
      Value addValue = peek(0);
      Value listValue = peek(1);

      ObjList *list = AS_LIST(listValue);
      writeValueArray(&list->values, addValue);

      pop();
      pop();
      push(OBJ_VAL(list));
      break;
    }

    case OP_NEW_DICT:
    {
      ObjDict *dict = initDict();
      push(OBJ_VAL(dict));
      break;
    }
    case OP_ADD_DICT:
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
          break;
      }

      ObjDict *dict = AS_DICT(dictValue);
      char *keyString = AS_CSTRING(key);

      insertDict(dict, keyString, value);

      pop();
      pop();
      pop();
      push(OBJ_VAL(dict));
      break;
    }
    case OP_SUBSCRIPT:
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
            break;
        }

        pop();
        pop();
        push(result);
      }
      break;
    }
    case OP_SUBSCRIPT_ASSIGN:
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
          push(NONE_VAL);
          if (!checkTry(frame))
            return INTERPRET_RUNTIME_ERROR;
          else
            break;
        }

        pop();
        pop();
        pop();
        push(NONE_VAL);
      }
      break;
    }

    case OP_CALL:
    {
      int argCount = READ_BYTE();
      if (!callValue(peek(argCount), argCount))
      {
        if (!checkTry(frame))
          return INTERPRET_RUNTIME_ERROR;
        else
          break;
      }

      if (threadFrame->ctf->eval && IS_FUNCTION(peek(0)))
      {
        argCount = 0;

        ObjFunction *function = AS_FUNCTION(peek(0));
        ObjClosure *closure = newClosure(function);
        pop();
        push(OBJ_VAL(closure));

        if (!callValue(peek(argCount), argCount))
        {
          if (!checkTry(frame))
            return INTERPRET_RUNTIME_ERROR;
          else
            break;
        }
      }
      frame = &threadFrame->ctf->frames[threadFrame->ctf->frameCount - 1];
      threadFrame->ctf->eval = false;
      break;
    }

    case OP_INVOKE:
    {
      int argCount = READ_BYTE();
      ObjString *method = READ_STRING();
      if (!invoke(method, argCount))
      {
        if (!checkTry(frame))
          return INTERPRET_RUNTIME_ERROR;
        else
          break;
      }
      frame = &threadFrame->ctf->frames[threadFrame->ctf->frameCount - 1];
      break;
    }

    case OP_SUPER:
    {
      int argCount = READ_BYTE();
      ObjString *method = READ_STRING();
      ObjClass *superclass = AS_CLASS(pop());
      if (!invokeFromClass(superclass, method, argCount))
      {
        if (!checkTry(frame))
          return INTERPRET_RUNTIME_ERROR;
        else
          break;
      }
      frame = &threadFrame->ctf->frames[threadFrame->ctf->frameCount - 1];
      break;
    }

    case OP_CLOSURE:
    {
      ObjFunction *function = AS_FUNCTION(READ_CONSTANT());
      ObjClosure *closure = newClosure(function);
      push(OBJ_VAL(closure));
      for (int i = 0; i < closure->upvalueCount; i++)
      {
        uint8_t isLocal = READ_BYTE();
        uint8_t index = READ_BYTE();
        if (isLocal)
        {
          closure->upvalues[i] = captureUpvalue(frame->slots + index);
        }
        else
        {
          closure->upvalues[i] = frame->closure->upvalues[index];
        }
      }
      break;
    }

    case OP_CLOSE_UPVALUE:
      closeUpvalues(threadFrame->ctf->stackTop - 1);
      pop();
      break;

    case OP_RETURN:
    {
      Value result = pop();

      ObjPackage *package = frame->package;
      if (frame->package != NULL)
      {
        frame->package = NULL;
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
        //return INTERPRET_OK;
        break;
      }

      /*
      if (threadFrame->frameCount == 0)
      {
        pop();
        return INTERPRET_OK;
      }
      */

      threadFrame->ctf->stackTop = frame->slots;
      if (type != CALL_FRAME_TYPE_PACKAGE)
      {
        push(result);
      }
      else if(frame->require && package != NULL)
        push(OBJ_VAL(package));

      frame = &threadFrame->ctf->frames[threadFrame->ctf->frameCount - 1];
      break;
    }

    case OP_CLASS:
    {
      ObjClass *klass = newClass(READ_STRING());
      klass->package = frame->package;
      vm.repl = OBJ_VAL(klass);
      push(OBJ_VAL(klass));
    }
    break;

    case OP_INHERIT:
    {
      Value superclass = peek(1);
      if (!IS_CLASS(superclass))
      {
        runtimeError("Superclass must be a class.");
        if (!checkTry(frame))
          return INTERPRET_RUNTIME_ERROR;
        else
          break;
      }

      ObjClass *subclass = AS_CLASS(peek(0));
      tableAddAll(&AS_CLASS(superclass)->methods, &subclass->methods);
      tableAddAll(&AS_CLASS(superclass)->fields, &subclass->fields);
      pop(); // Subclass.
      break;
    }

    case OP_METHOD:
      defineMethod(READ_STRING());
      break;
    case OP_PROPERTY:
      defineProperty(READ_STRING());
      break;
    case OP_EXTENSION:
      defineExtension(READ_STRING(), READ_STRING());
      break;

    case OP_FILE:
    {
      Value openType = pop();
      Value fileName = pop();

      if (!IS_STRING(openType))
      {
        runtimeError("File open type must be a string");
        if (!checkTry(frame))
          return INTERPRET_RUNTIME_ERROR;
        else
          break;
      }

      if (!IS_STRING(fileName))
      {
        runtimeError("Filename must be a string");
        if (!checkTry(frame))
          return INTERPRET_RUNTIME_ERROR;
        else
          break;
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
          break;
      }

      push(OBJ_VAL(file));
      break;
    }
    case OP_NATIVE_FUNC:
    {
      ObjNativeFunc *func = initNativeFunc();

      int arity = AS_NUMBER(pop());
      for (int i = arity - 1; i >= 0; i--)
      {
        writeValueArray(&func->params, peek(i));
      }

      while (arity > 0)
      {
        pop();
        arity--;
      }

      func->name = AS_STRING(pop());
      func->returnType = AS_STRING(pop());

      push(OBJ_VAL(func));
      break;
    }
    case OP_NATIVE:
    {
      ObjNativeLib *lib = initNativeLib();

      int count = AS_NUMBER(pop());
      lib->functions = count;

      while (count > 0)
      {
        ObjNativeFunc *func = AS_NATIVE_FUNC(pop());
        func->lib = lib;
        count--;
      }

      lib->name = AS_STRING(pop());

      push(OBJ_VAL(lib));
      break;
    }
    case OP_ASYNC:
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
      frame->package = threadFrame->frame->package;
      frame->type = CALL_FRAME_TYPE_FUNCTION;
      frame->nextPackage = NULL;
      frame->require = false;

      frame->slots = tf->stackTop - 1;

      ObjList *args = initList();
      tf->currentArgs = OBJ_VAL(args);

      ((ThreadFrame *)tf->threadFrame)->running = true;
      // while(true)
      // {
      //   cube_wait(100);
      // }

      break;
    }

    case OP_AWAIT:
    {
      if (!IS_TASK(peek(0)))
      {
        runtimeError("A task is required in await.");
        if (!checkTry(frame))
          return INTERPRET_RUNTIME_ERROR;
        else
          break;
      }
      ObjTask *task = AS_TASK(pop());
      TaskFrame *tf = findTaskFrame(task->name->chars);
      if (tf == NULL)
      {
        push(NONE_VAL);
      }
      else
      {
        if (tf->finished)
        {
          push(tf->result);
        }
        else
        {
          push(OBJ_VAL(task));
          frame->ip--;
        }
      }

      break;
    }

    case OP_ABORT:
    {
      if (!IS_TASK(peek(0)))
      {
        runtimeError("A task is required in abort.");
        if (!checkTry(frame))
          return INTERPRET_RUNTIME_ERROR;
        else
          break;
      }
      ObjTask *task = AS_TASK(pop());
      destroyTaskFrame(task->name->chars);
      break;
    }

    case OP_TRY:
    {
      uint16_t catch = READ_SHORT();
      pushTry(frame, catch);
      break;
    }

    case OP_CLOSE_TRY:
    {
      uint16_t end = READ_SHORT();
      frame->ip += end;
      popTry();
      break;
    }

    case OP_TEST:
      break;
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
    callValue(OBJ_VAL(closure), initArgC - initArgStart);
    initArgC = 0;
    initArgStart = 0;
    initArgV = NULL;
  }
  else
    callValue(OBJ_VAL(closure), 0);

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

InterpretResult compileCode(const char *source, const char *path)
{
  ObjFunction *function = compile(source, path);
  if (function == NULL)
    return INTERPRET_COMPILE_ERROR;

  char *bcPath = ALLOCATE(char, strlen(path) * 2);
  strcpy(bcPath, path);
  if (strstr(bcPath, ".cube") != NULL)
    replaceStringN(bcPath, ".cube", ".cubec", 1);
  else
    strcat(bcPath + strlen(bcPath), ".cubec");

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

  if (!writeByteCode(file, OBJ_VAL(function)))
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
  while (true)
  {
    if (!threadFrame->running)
    {
      // printf("Thread wait: %d\n", thread_id());
      cube_wait(1);
    }
    else
    {
      printf("Thread run: %d\n", thread_id());
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
}