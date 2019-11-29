#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

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

VM vm; // [one]

static void resetStack()
{
  vm.stackTop = vm.stack;
  vm.frameCount = 0;
  vm.openUpvalues = NULL;
  vm.currentFrameCount = 0;
}

/*
void runtimeError(const char *format, ...)
{
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  fputs("\n", stderr);

  for (int i = vm.frameCount - 1; i >= 0; i--)
  {
    CallFrame *frame = &vm.frames[i];
    ObjFunction *function = frame->closure->function;

    size_t instruction = frame->ip - function->chunk.code - 1;
    fprintf(stderr, "[line %d] in ",
            function->chunk.lines[instruction]);
    if (function->name == NULL)
    {
      fprintf(stderr, "%s: ", vm.currentScriptName);
    }
    else
    {
      fprintf(stderr, "%s()\n", function->name->chars);
    }
  }

  resetStack();
}
*/

void runtimeError(const char *format, ...)
{
  for (int i = vm.frameCount - 1; i >= 0; i--)
  {
    CallFrame *frame = &vm.frames[i];

    ObjFunction *function = frame->closure->function;

    // -1 because the IP is sitting on the next instruction to be
    // executed.
    size_t instruction = frame->ip - function->chunk.code - 1;
    fprintf(stderr, "[line %d] in ", function->chunk.lines[instruction]);

    if (function->name == NULL)
    {
      fprintf(stderr, "%s: ", vm.currentScriptName);
      i = -1;
    }
    else
      fprintf(stderr, "%s(): ", function->name->chars);

    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    fputs("\n", stderr);
    va_end(args);
  }

  resetStack();
}

static void defineNative(const char *name, NativeFn function)
{
  push(OBJ_VAL(copyString(name, (int)strlen(name))));
  push(OBJ_VAL(newNative(function)));
  tableSet(&vm.globals, AS_STRING(vm.stack[0]), vm.stack[1]);
  pop();
  pop();
}

void initVM(const char *path, const char *scriptName)
{
  resetStack();
  vm.objects = NULL;
  vm.listObjects = NULL;
  vm.bytesAllocated = 0;
  vm.nextGC = 1024 * 1024;

  vm.grayCount = 0;
  vm.grayCapacity = 0;
  vm.grayStack = NULL;

  initTable(&vm.globals);
  initTable(&vm.strings);
  vm.paths = initList();
  addPath(path);

  vm.extension = ".cube";
  vm.scriptName = scriptName;
  vm.currentScriptName = scriptName;
  vm.initString = copyString("init", 4);
  vm.newLine = false;

  // STD
  defineNative("clock", clockNative);
  defineNative("time", timeNative);
  defineNative("exit", exitNative);
  defineNative("input", inputNative);
  defineNative("print", printNative);
  defineNative("println", printlnNative);
  defineNative("random", randomNative);
  defineNative("bool", boolNative);
  defineNative("num", numNative);
  defineNative("str", strNative);
  defineNative("ceil", ceilNative);
  defineNative("floor", floorNative);
  defineNative("round", roundNative);
  defineNative("pow", powNative);
  defineNative("exp", expNative);
  defineNative("len", lenNative);
  defineNative("type", typeNative);
  defineNative("env", envNative);
  defineNative("open", openNative);
}

void freeVM()
{
  freeTable(&vm.globals);
  freeTable(&vm.strings);
  vm.initString = NULL;
  freeObjects();
  freeLists();
}

void addPath(const char *path)
{
  writeValueArray(&vm.paths->values, STRING_VAL(path));
}

void push(Value value)
{
  *vm.stackTop = value;
  vm.stackTop++;
}

Value pop()
{
  vm.stackTop--;
  return *vm.stackTop;
}

Value peek(int distance)
{
  return vm.stackTop[-1 - distance];
}

static bool call(ObjClosure *closure, int argCount)
{
  if (argCount != closure->function->arity)
  {
    runtimeError("Expected %d arguments but got %d.",
                 closure->function->arity, argCount);
    return false;
  }

  if (vm.frameCount == FRAMES_MAX)
  {
    runtimeError("Stack overflow.");
    return false;
  }

  CallFrame *frame = &vm.frames[vm.frameCount++];
  frame->closure = closure;
  frame->ip = closure->function->chunk.code;

  frame->slots = vm.stackTop - argCount - 1;
  return true;
}

static bool callValue(Value callee, int argCount)
{
  if (IS_OBJ(callee))
  {
    switch (OBJ_TYPE(callee))
    {
    case OBJ_BOUND_METHOD:
    {
      ObjBoundMethod *bound = AS_BOUND_METHOD(callee);

      // Replace the bound method with the receiver so it's in the
      // right slot when the method is called.
      vm.stackTop[-argCount - 1] = bound->receiver;
      return call(bound->method, argCount);
    }

    case OBJ_CLASS:
    {
      ObjClass *klass = AS_CLASS(callee);

      // Create the instance.
      vm.stackTop[-argCount - 1] = OBJ_VAL(newInstance(klass));
      // Call the initializer, if there is one.
      Value initializer;
      if (tableGet(&klass->methods, vm.initString, &initializer))
      {
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
      Value result = native(argCount, vm.stackTop - argCount);
      vm.stackTop -= argCount + 1;
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

static bool invokeFromClass(ObjClass *klass, ObjString *name,
                            int argCount)
{
  // Look for the method.
  Value method;
  if (!tableGet(&klass->methods, name, &method))
  {
    runtimeError("Undefined property '%s'.", name->chars);
    return false;
  }

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

    return callValue(method, argCount);
  }

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
    vm.stackTop[-argCount] = value;
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

static ObjUpvalue *captureUpvalue(Value *local)
{
  ObjUpvalue *prevUpvalue = NULL;
  ObjUpvalue *upvalue = vm.openUpvalues;

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
    vm.openUpvalues = createdUpvalue;
  }
  else
  {
    prevUpvalue->next = createdUpvalue;
  }

  return createdUpvalue;
}

static void closeUpvalues(Value *last)
{
  while (vm.openUpvalues != NULL &&
         vm.openUpvalues->location >= last)
  {
    ObjUpvalue *upvalue = vm.openUpvalues;
    upvalue->closed = *upvalue->location;
    upvalue->location = &upvalue->closed;
    vm.openUpvalues = upvalue->next;
  }
}

static void defineMethod(ObjString *name)
{
  Value method = peek(0);
  ObjClass *klass = AS_CLASS(peek(1));
  tableSet(&klass->methods, name, method);
  pop();
  pop();
}

static void defineProperty(ObjString *name)
{
  Value value = peek(0);
  ObjClass *klass = AS_CLASS(peek(1));
  tableSet(&klass->fields, name, value);
  pop();
  pop();
}

bool isFalsey(Value value)
{
  return IS_NONE(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

static void concatenate()
{
  ObjString *b = AS_STRING(peek(0));
  ObjString *a = AS_STRING(peek(1));

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

static Value increment(Value start, Value step, Value stop)
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
    if (res_ > stop_)
      res = NONE_VAL;
  }
  else
  {
    if (res_ < stop_)
      res = NONE_VAL;
  }

  return res;
}

Value envVariable(ObjString *name)
{
  if (strcmp(name->chars, "__name__") == 0)
  {
    return STRING_VAL(vm.currentScriptName);
  }
  return NONE_VAL;
}

static InterpretResult run()
{
  CallFrame *frame = &vm.frames[vm.frameCount - 1];

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
      return INTERPRET_RUNTIME_ERROR;               \
    }                                               \
                                                    \
    double b = AS_NUMBER(pop());                    \
    double a = AS_NUMBER(pop());                    \
    push(valueType(a op b));                        \
  } while (false)

  for (;;)
  {
#ifdef DEBUG_TRACE_EXECUTION
    printf("          ");
    for (Value *slot = vm.stack; slot < vm.stackTop; slot++)
    {
      printf("[ ");
      printValue(*slot);
      printf(" ]");
    }
    printf("\n");
    disassembleInstruction(&frame->closure->function->chunk,
                           (int)(frame->ip - frame->closure->function->chunk.code));
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
        return INTERPRET_RUNTIME_ERROR;
      }

      ObjList *list = initList();

      while (!IS_NONE(start))
      {
        writeValueArray(&list->values, start);
        start = increment(start, step, stop);
      }

      push(OBJ_VAL(list));
      break;
    }
    case OP_POP:
      pop();
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
      if (!tableGet(&vm.globals, name, &value))
      {
        value = envVariable(name);
        if (IS_NONE(value))
        {
          runtimeError("Undefined variable '%s'.", name->chars);
          return INTERPRET_RUNTIME_ERROR;
        }
      }
      push(value);
      break;
    }

    case OP_DEFINE_GLOBAL:
    {
      ObjString *name = READ_STRING();
      tableSet(&vm.globals, name, peek(0));
      pop();
      break;
    }

    case OP_SET_GLOBAL:
    {
      ObjString *name = READ_STRING();
      if (tableSet(&vm.globals, name, peek(0)))
      {
        tableDelete(&vm.globals, name); // [delete]
        runtimeError("Undefined variable '%s'.", name->chars);
        return INTERPRET_RUNTIME_ERROR;
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
      if (!IS_INSTANCE(peek(0)))
      {
        runtimeError("Only instances have properties.");
        return INTERPRET_RUNTIME_ERROR;
      }

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
        return INTERPRET_RUNTIME_ERROR;
      }
      break;
    }

    OP_GET_PROPERTY_NO_POP:
    {
      if (!IS_INSTANCE(peek(0)))
      {
        runtimeError("Only instances have properties.");
        return INTERPRET_RUNTIME_ERROR;
      }

      ObjInstance *instance = AS_INSTANCE(peek(0));
      ObjString *name = READ_STRING();
      Value value;
      if (tableGet(&instance->fields, name, &value))
      {
        push(value);
        break;
      }

      if (!bindMethod(instance->klass, name))
        return INTERPRET_RUNTIME_ERROR;

      break;
    }

    case OP_SET_PROPERTY:
    {
      if (!IS_INSTANCE(peek(1)))
      {
        runtimeError("Only instances have fields.");
        return INTERPRET_RUNTIME_ERROR;
      }

      ObjInstance *instance = AS_INSTANCE(peek(1));
      tableSet(&instance->fields, READ_STRING(), peek(0));
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
        return INTERPRET_RUNTIME_ERROR;
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
      BINARY_OP(BOOL_VAL, <);
      break;
    case OP_ADD:
    {
      if (IS_STRING(peek(0)) && IS_STRING(peek(1)))
      {
        concatenate();
      }
      else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1)))
      {
        double b = AS_NUMBER(pop());
        double a = AS_NUMBER(pop());
        push(NUMBER_VAL(a + b));
      }
      else if (IS_LIST(peek(0)) && IS_LIST(peek(1)))
      {
        Value listToAddValue = pop();
        Value listValue = pop();

        ObjList *list = AS_LIST(listValue);
        ObjList *listToAdd = AS_LIST(listToAddValue);

        ObjList *result = initList();
        for (int i = 0; i < list->values.count; ++i)
          writeValueArray(&result->values, list->values.values[i]);
        for (int i = 0; i < listToAdd->values.count; ++i)
          writeValueArray(&result->values, listToAdd->values.values[i]);

        push(OBJ_VAL(result));
      }
      else
      {
        runtimeError("Operands must be two numbers or two strings.");
        return INTERPRET_RUNTIME_ERROR;
      }
      break;
    }

    case OP_SUBTRACT:
      BINARY_OP(NUMBER_VAL, -);
      break;
    case OP_INC:
    {
      if (!IS_NUMBER(peek(0)))
      {
        runtimeError("Operand must be a number.");
        return INTERPRET_RUNTIME_ERROR;
      }

      push(NUMBER_VAL(AS_NUMBER(pop()) + 1));
      break;
    }
    case OP_DEC:
    {
      if (!IS_NUMBER(peek(0)))
      {
        runtimeError("Operand must be a number.");
        return INTERPRET_RUNTIME_ERROR;
      }

      push(NUMBER_VAL(AS_NUMBER(pop()) - 1));
      break;
    }
    case OP_MULTIPLY:
      BINARY_OP(NUMBER_VAL, *);
      break;
    case OP_DIVIDE:
      BINARY_OP(NUMBER_VAL, /);
      break;

    case OP_MOD:
    {
      double b = AS_NUMBER(pop());
      double a = AS_NUMBER(pop());

      push(NUMBER_VAL(fmod(a, b)));
      break;
    }

    case OP_POW:
    {
      double b = AS_NUMBER(pop());
      double a = AS_NUMBER(pop());

      push(NUMBER_VAL(pow(a, b)));
      break;
    }

    case OP_IN:
    {
      Value item = pop();
      Value container = pop();
      Value result;
      if (IS_STRING(container) && IS_STRING(item))
      {
        if (!stringContains(container, item, &result))
        {
          runtimeError("Could not search on string");
          return INTERPRET_RUNTIME_ERROR;
        }
      }
      else if (IS_LIST(container))
      {
        if (!listContains(container, item, &result))
        {
          runtimeError("Could not search on list");
          return INTERPRET_RUNTIME_ERROR;
        }
      }
      else if (IS_DICT(container))
      {
        if (!dictContains(container, item, &result))
        {
          runtimeError("Key in dict must be a string");
          return INTERPRET_RUNTIME_ERROR;
        }
      }

      push(result);
      break;
    }

    case OP_NOT:
      push(BOOL_VAL(isFalsey(pop())));
      break;

    case OP_NEGATE:
      if (!IS_NUMBER(peek(0)))
      {
        runtimeError("Operand must be a number.");
        return INTERPRET_RUNTIME_ERROR;
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

    case OP_IMPORT:
    {
      ObjString *fileName = AS_STRING(pop());
      char *s = readFile(fileName->chars, false);
      int index = 0;
      while (s == NULL && index < vm.paths->values.count)
      {
        ObjString *folder = AS_STRING(vm.paths->values.values[index]);
        char *strPath = malloc(sizeof(char) * (folder->length + fileName->length + 2));
        strcpy(strPath, folder->chars);
        strcat(strPath, fileName->chars);
        s = readFile(strPath, false);
        free(strPath);
        index++;
      }
      if (s == NULL)
      {
        runtimeError("Could not loaded the file \"%s\".", fileName->chars);
        return INTERPRET_RUNTIME_ERROR;
      }
      vm.currentScriptName = fileName->chars;

      ObjFunction *function = compile(s);
      if (function == NULL)
      {
        free(s);
        return INTERPRET_COMPILE_ERROR;
      }
      push(OBJ_VAL(function));
      ObjClosure *closure = newClosure(function);
      pop();

      vm.currentFrameCount = vm.frameCount;

      frame = &vm.frames[vm.frameCount++];
      frame->ip = closure->function->chunk.code;
      frame->closure = closure;
      frame->slots = vm.stackTop - 1;
      free(s);
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
      Value addValue = pop();
      Value listValue = pop();

      ObjList *list = AS_LIST(listValue);
      writeValueArray(&list->values, addValue);

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
      Value value = pop();
      Value key = pop();
      Value dictValue = pop();

      if (!IS_STRING(key))
      {
        runtimeError("Dictionary key must be a string.");
        return INTERPRET_RUNTIME_ERROR;
      }

      ObjDict *dict = AS_DICT(dictValue);
      char *keyString = AS_CSTRING(key);

      insertDict(dict, keyString, value);

      push(OBJ_VAL(dict));
      break;
    }
    case OP_SUBSCRIPT:
    {
      Value indexValue = pop();
      Value listValue = pop();

      if (!IS_NUMBER(indexValue))
      {
        runtimeError("Array index must be a number.");
        return INTERPRET_RUNTIME_ERROR;
      }

      ObjList *list = AS_LIST(listValue);
      int index = AS_NUMBER(indexValue);

      if (index < 0)
        index = list->values.count + index;
      if (index >= 0 && index < list->values.count)
      {
        push(list->values.values[index]);
        break;
      }

      runtimeError("Array index out of bounds.");
      return INTERPRET_RUNTIME_ERROR;
    }
    case OP_SUBSCRIPT_ASSIGN:
    {
      Value assignValue = pop();
      Value indexValue = pop();
      Value listValue = pop();

      if (!IS_NUMBER(indexValue))
      {
        runtimeError("Array index must be a number.");
        return INTERPRET_RUNTIME_ERROR;
      }

      ObjList *list = AS_LIST(listValue);
      int index = AS_NUMBER(indexValue);

      if (index < 0)
        index = list->values.count + index;
      if (index >= 0 && index < list->values.count)
      {
        list->values.values[index] = assignValue;
        push(NONE_VAL);
        break;
      }

      push(NONE_VAL);

      runtimeError("Array index out of bounds.");
      return INTERPRET_RUNTIME_ERROR;
    }
    case OP_SUBSCRIPT_DICT:
    {
      Value indexValue = pop();
      Value dictValue = pop();

      if (!IS_STRING(indexValue))
      {
        runtimeError("Dictionary key must be a string.");
        return INTERPRET_RUNTIME_ERROR;
      }

      ObjDict *dict = AS_DICT(dictValue);
      char *key = AS_CSTRING(indexValue);

      push(searchDict(dict, key));

      break;
    }
    case OP_SUBSCRIPT_DICT_ASSIGN:
    {
      Value value = pop();
      Value key = pop();
      Value dictValue = pop();

      if (!IS_STRING(key))
      {
        runtimeError("Dictionary key must be a string.");
        return INTERPRET_RUNTIME_ERROR;
      }

      ObjDict *dict = AS_DICT(dictValue);
      char *keyString = AS_CSTRING(key);

      insertDict(dict, keyString, value);

      push(NONE_VAL);
      break;
    }

    case OP_CALL:
    {
      int argCount = READ_BYTE();
      if (!callValue(peek(argCount), argCount))
      {
        return INTERPRET_RUNTIME_ERROR;
      }

      frame = &vm.frames[vm.frameCount - 1];
      break;
    }

    case OP_INVOKE:
    {
      int argCount = READ_BYTE();
      ObjString *method = READ_STRING();
      if (!invoke(method, argCount))
      {
        return INTERPRET_RUNTIME_ERROR;
      }
      frame = &vm.frames[vm.frameCount - 1];
      break;
    }

    case OP_SUPER:
    {
      int argCount = READ_BYTE();
      ObjString *method = READ_STRING();
      ObjClass *superclass = AS_CLASS(pop());
      if (!invokeFromClass(superclass, method, argCount))
      {
        return INTERPRET_RUNTIME_ERROR;
      }
      frame = &vm.frames[vm.frameCount - 1];
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
      closeUpvalues(vm.stackTop - 1);
      pop();
      break;

    case OP_RETURN:
    {
      Value result = pop();
      closeUpvalues(frame->slots);
      vm.frameCount--;

      if (vm.frameCount == vm.currentFrameCount)
      {
        vm.currentScriptName = vm.scriptName;
        vm.currentFrameCount = -1;
      }

      if (vm.frameCount == 0)
        return INTERPRET_OK;

      /*
      if (vm.frameCount == 0)
      {
        pop();
        return INTERPRET_OK;
      }
      */

      vm.stackTop = frame->slots;
      push(result);

      frame = &vm.frames[vm.frameCount - 1];
      break;
    }

    case OP_CLASS:
      push(OBJ_VAL(newClass(READ_STRING())));
      break;

    case OP_INHERIT:
    {
      Value superclass = peek(1);
      if (!IS_CLASS(superclass))
      {
        runtimeError("Superclass must be a class.");
        return INTERPRET_RUNTIME_ERROR;
      }

      ObjClass *subclass = AS_CLASS(peek(0));
      tableAddAll(&AS_CLASS(superclass)->methods, &subclass->methods);
      pop(); // Subclass.
      break;
    }

    case OP_METHOD:
      defineMethod(READ_STRING());
      break;
    case OP_PROPERTY:
      defineProperty(READ_STRING());
      break;
    case OP_FILE:
    {
      Value openType = pop();
      Value fileName = pop();

      if (!IS_STRING(openType))
      {
        runtimeError("File open type must be a string");
        return INTERPRET_RUNTIME_ERROR;
      }

      if (!IS_STRING(fileName))
      {
        runtimeError("Filename must be a string");
        return INTERPRET_RUNTIME_ERROR;
      }

      ObjString *openTypeString = AS_STRING(openType);
      ObjString *fileNameString = AS_STRING(fileName);

      ObjFile *file = initFile();
      file->file = fopen(fileNameString->chars, openTypeString->chars);
      file->path = fileNameString->chars;
      file->openType = openTypeString->chars;

      if (file->file == NULL)
      {
        runtimeError("Unable to open file");
        return INTERPRET_RUNTIME_ERROR;
      }

      file->isOpen = true;

      push(OBJ_VAL(file));
      break;
    }
    }
  }

#undef READ_BYTE
#undef READ_SHORT
#undef READ_CONSTANT
#undef READ_STRING
#undef BINARY_OP
}

void hack(bool b)
{
  run();
  if (b)
    hack(false);
}

InterpretResult interpret(const char *source)
{
  ObjFunction *function = compile(source);
  if (function == NULL)
    return INTERPRET_COMPILE_ERROR;

  push(OBJ_VAL(function));

  ObjClosure *closure = newClosure(function);
  pop();
  push(OBJ_VAL(closure));
  callValue(OBJ_VAL(closure), 0);
  return run();
}
