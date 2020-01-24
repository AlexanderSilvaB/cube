#ifndef CLOX_object_h
#define CLOX_object_h

#include <stdlib.h>
#include <stdio.h>

#include "common.h"
#include "chunk.h"
#include "table.h"
#include "value.h"

#define OBJ_TYPE(value) (AS_OBJ(value)->type)

#define IS_BOUND_METHOD(value) isObjType(value, OBJ_BOUND_METHOD)
#define IS_CLASS(value) isObjType(value, OBJ_CLASS)
#define IS_PACKAGE(value) isObjType(value, OBJ_PACKAGE)
#define IS_CLOSURE(value) isObjType(value, OBJ_CLOSURE)
#define IS_FUNCTION(value) isObjType(value, OBJ_FUNCTION)
#define IS_INSTANCE(value) isObjType(value, OBJ_INSTANCE)
#define IS_NATIVE(value) isObjType(value, OBJ_NATIVE)
#define IS_STRING(value) isObjType(value, OBJ_STRING)
#define IS_LIST(value) isObjType(value, OBJ_LIST)
#define IS_DICT(value) isObjType(value, OBJ_DICT)
#define IS_FILE(value) isObjType(value, OBJ_FILE)
#define IS_BYTES(value) isObjType(value, OBJ_BYTES)
#define IS_NATIVE_FUNC(value) isObjType(value, OBJ_NATIVE_FUNC)
#define IS_NATIVE_LIB(value) isObjType(value, OBJ_NATIVE_LIB)
#define IS_TASK(value) isObjType(value, OBJ_TASK)

#define AS_BOUND_METHOD(value) ((ObjBoundMethod *)AS_OBJ(value))
#define AS_CLASS(value) ((ObjClass *)AS_OBJ(value))
#define AS_PACKAGE(value) ((ObjPackage *)AS_OBJ(value))
#define AS_CLOSURE(value) ((ObjClosure *)AS_OBJ(value))
#define AS_FUNCTION(value) ((ObjFunction *)AS_OBJ(value))
#define AS_INSTANCE(value) ((ObjInstance *)AS_OBJ(value))
#define AS_NATIVE(value) (((ObjNative *)AS_OBJ(value))->function)
#define AS_STRING(value) ((ObjString *)AS_OBJ(value))
#define AS_CSTRING(value) (((ObjString *)AS_OBJ(value))->chars)
#define AS_LIST(value) ((ObjList *)AS_OBJ(value))
#define AS_DICT(value) ((ObjDict *)AS_OBJ(value))
#define AS_FILE(value)          ((ObjFile*)AS_OBJ(value))
#define AS_BYTES(value)          ((ObjBytes*)AS_OBJ(value))
#define AS_CBYTES(value) (((ObjBytes *)AS_OBJ(value))->bytes)
#define AS_NATIVE_FUNC(value)          ((ObjNativeFunc*)AS_OBJ(value))
#define AS_NATIVE_LIB(value)          ((ObjNativeLib*)AS_OBJ(value))
#define AS_TASK(value)          ((ObjTask*)AS_OBJ(value))

#define STRING_VAL(str) (OBJ_VAL(copyString(str, strlen(str))))
#define BYTES_VAL(data, len) (OBJ_VAL(copyBytes(data, len)))

#define IS_INCREMENTAL(value) (IS_NUMBER(value) || (IS_STRING(value) && AS_STRING(value)->length == 1))

typedef enum
{
  OBJ_BOUND_METHOD,
  OBJ_CLASS,
  OBJ_PACKAGE,
  OBJ_CLOSURE,
  OBJ_FUNCTION,
  OBJ_INSTANCE,
  OBJ_NATIVE,
  OBJ_STRING,
  OBJ_LIST,
  OBJ_DICT,
  OBJ_FILE,
  OBJ_BYTES,
  OBJ_NATIVE_FUNC,
  OBJ_NATIVE_LIB,
  OBJ_TASK,
  OBJ_UPVALUE
} ObjType;

struct sObj
{
  ObjType type;
  bool isMarked;
  struct sObj *next;
};

typedef struct
{
  Obj obj;
  int arity;
  int upvalueCount;
  Chunk chunk;
  ObjString *name;
  bool staticMethod;
  const char *path;
} ObjFunction;

typedef Value (*NativeFn)(int argCount, Value *args);

typedef struct
{
  Obj obj;
  NativeFn function;
} ObjNative;

struct sObjString
{
  Obj obj;
  int length;
  char *chars;
  uint32_t hash;
};

struct sObjList
{
  Obj obj;
  ValueArray values;
};

struct dictItem
{
  char *key;
  Value item;
  bool deleted;
  uint32_t hash;
};

struct sObjDict
{
  Obj obj;
  int capacity;
  int count;
  dictItem **items;
};

#define FILE_MODE_READ 0x1
#define FILE_MODE_WRITE 0x2
#define FILE_MODE_BINARY 0x4

#define FILE_CAN_READ(file) ((file->mode & FILE_MODE_READ) != 0)
#define FILE_CAN_WRITE(file) ((file->mode & FILE_MODE_WRITE) != 0)
#define FILE_IS_BINARY(file) ((file->mode & FILE_MODE_BINARY) != 0)

struct sObjFile {
    Obj obj;
    FILE *file;
    char *path;
    bool isOpen;
    int mode;
};

struct sObjBytes
{
  Obj obj;
  int length;
  unsigned char *bytes;
};

typedef struct sUpvalue
{
  Obj obj;
  Value *location;
  Value closed;
  struct sUpvalue *next;
} ObjUpvalue;

typedef struct
{
  Obj obj;
  ObjFunction *function;
  ObjUpvalue **upvalues;
  int upvalueCount;
} ObjClosure;

typedef struct sObjPackage
{
  Obj obj;
  ObjString *name;
  Table symbols;
  struct sObjPackage *parent;
} ObjPackage;

typedef struct sObjClass
{
  Obj obj;
  ObjString *name;
  ObjPackage *package;
  Table methods;
  Table fields;
  Table staticFields;
} ObjClass;

typedef struct
{
  Obj obj;
  ObjClass *klass;
  Table fields;
} ObjInstance;

typedef struct
{
  Obj obj;
  Value receiver;
  ObjClosure *method;
} ObjBoundMethod;

typedef struct
{
  Obj obj;
  ObjString *name;
  int functions;
  void *handle;
} ObjNativeLib;

typedef struct
{
  Obj obj;
  ObjString *name;
  ObjString *returnType;
  ObjNativeLib *lib;
  ValueArray params;
} ObjNativeFunc;

typedef struct
{
  Obj obj;
  ObjString *name;
} ObjTask;


ObjBoundMethod *newBoundMethod(Value receiver, ObjClosure *method);
ObjClass *newClass(ObjString *name);
ObjTask *newTask(ObjString *name);
ObjClosure *newClosure(ObjFunction *function);
ObjFunction *newFunction(bool isStatic);
ObjInstance *newInstance(ObjClass *klass);
ObjNative *newNative(NativeFn function);
ObjPackage *newPackage(ObjString *name);
ObjNativeFunc *initNativeFunc();
ObjNativeLib *initNativeLib();
ObjString *takeString(char *chars, int length);
ObjString *copyString(const char *chars, int length);
ObjList *initList();
ObjDict *initDict();
ObjFile *initFile();
ObjBytes *initBytes();
ObjUpvalue *newUpvalue(Value *slot);

ObjBytes *copyBytes(const void *bytes, int length);
void appendBytes(ObjBytes *dest, ObjBytes *src);

char *objectToString(Value value, bool literal);
char *objectType(Value value);
ObjBytes* objectToBytes(Value value);

bool dictComparison(Value a, Value b);
bool listComparison(Value a, Value b);
bool objectComparison(Value a, Value b);

Value copyObject(Value value);

static inline bool isObjType(Value value, ObjType type)
{
  return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#endif
