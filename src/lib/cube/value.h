#ifndef _CUBE_VALUE_H_
#define _CUBE_VALUE_H_

#include "common.h"

typedef struct sObj Obj;
typedef struct sObjString ObjString;
typedef struct sObjList ObjList;
typedef struct sObjDict ObjDict;
typedef struct dictItem dictItem;

typedef enum
{
    VAL_BOOL,
    VAL_NONE,
    VAL_NUMBER,
    VAL_OBJ
} ValueType;

typedef struct
{
    ValueType type;
    union
    {
        bool boolean;
        double number;
        Obj* obj;
    } as;
} Value;

#define IS_BOOL(value)    ((value).type == VAL_BOOL)
#define IS_NONE(value)     ((value).type == VAL_NONE)
#define IS_NUMBER(value)  ((value).type == VAL_NUMBER)
#define IS_OBJ(value)     ((value).type == VAL_OBJ)

#define AS_BOOL(value)    ((value).as.boolean)
#define AS_NUMBER(value)  ((value).as.number)
#define AS_OBJ(value)     ((value).as.obj)

#define BOOL_VAL(value)   ((Value){ VAL_BOOL, { .boolean = value } })
#define NONE_VAL           ((Value){ VAL_NONE, { .number = 0 } })
#define NUMBER_VAL(value) ((Value){ VAL_NUMBER, { .number = value } })
#define OBJ_VAL(object)   ((Value){ VAL_OBJ, { .obj = (Obj*)object } })

typedef struct
{
    int count;
    int capacity;
    Value* values;
} ValueArray;


bool valuesEqual(Value a, Value b);
void initValueArray(ValueArray* array);
void freeValueArray(ValueArray* array);

ObjDict *initDictValues(uint32_t capacity);
void insertDict(ObjDict *dict, char *key, Value value);
void resizeDict(ObjDict *dict, bool grow);
Value searchDict(ObjDict *dict, char *key);
void freeDict(ObjDict *dict);

void writeValueArray(ValueArray* array, Value value);
void printValue(Value value);
char *valueToString(Value value);

Value toBool(Value value);
Value toNumber(Value value);
Value toString(Value value);

#endif
