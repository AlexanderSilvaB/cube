#ifndef _CUBE_EXT_H_
#define _CUBE_EXT_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#if defined _WIN32 || defined __CYGWIN__
#ifdef WIN_EXPORT
// Exporting...
#ifdef __GNUC__
#define EXPORTED __attribute__((dllexport))
#else
#define EXPORTED __declspec(dllexport) // Note: actually gcc seems to also supports this syntax.
#endif
#else
#ifdef __GNUC__
#define EXPORTED __attribute__((dllimport))
#else
#define EXPORTED __declspec(dllimport) // Note: actually gcc seems to also supports this syntax.
#endif
#endif
#define NOT_EXPORTED
#else
#if __GNUC__ >= 4
#define EXPORTED __attribute__((visibility("default")))
#define NOT_EXPORTED __attribute__((visibility("hidden")))
#else
#define EXPORTED
#define NOT_EXPORTED
#endif
#endif

typedef struct
{
    int line;
    const char *path;
} DebugInfo;

typedef enum
{
    TYPE_VOID,
    TYPE_NULL,
    TYPE_BOOL,
    TYPE_NUMBER,
    TYPE_STRING,
    TYPE_BYTES,
    TYPE_LIST,
    TYPE_DICT,
    TYPE_VAR,
    TYPE_FUNC,
    TYPE_UNKNOWN
} NativeTypes;

typedef struct
{
    unsigned int length;
    unsigned char *bytes;
} cube_native_bytes;

typedef union cube_native_value_t {
    bool _bool;
    double _number;
    char *_string;
    cube_native_bytes _bytes;
} cube_native_value;

typedef struct cube_native_var_t
{
    cube_native_value value;
    NativeTypes type;
    bool is_list;
    bool is_dict;
    char *key;
    int size;
    int capacity;
    struct cube_native_var_t **list;
    struct cube_native_var_t **dict;
} cube_native_var;

#define NATIVE_TYPE(var) (var->type)
#define AS_NATIVE_BOOL(var) var->value._bool
#define AS_NATIVE_NUMBER(var) var->value._number
#define AS_NATIVE_STRING(var) var->value._string
#define AS_NATIVE_BYTES(var) var->value._bytes
#define IS_NATIVE_LIST(var) (var->is_list)
#define IS_NATIVE_DICT(var) (var->is_dict)

static char *COPY_STR(const char *str)
{
    char *s = (char *)malloc(sizeof(char) * (strlen(str) + 1));
    strcpy(s, str);
    return s;
}

static cube_native_var *NATIVE_VAR()
{
    cube_native_var *var = (cube_native_var *)malloc(sizeof(cube_native_var));
    var->is_list = false;
    var->is_dict = false;
    var->type = TYPE_VOID;
    var->list = NULL;
    var->dict = NULL;
    var->key = NULL;
    var->size = 0;
    var->capacity = 0;
    return var;
}

static cube_native_var *NATIVE_NULL()
{
    cube_native_var *var = NATIVE_VAR();
    var->type = TYPE_NULL;
    return var;
}

static cube_native_var *NATIVE_BOOL(bool v)
{
    cube_native_var *var = NATIVE_VAR();
    var->type = TYPE_BOOL;
    var->value._bool = v;
    return var;
}

static cube_native_var *NATIVE_NUMBER(double v)
{
    cube_native_var *var = NATIVE_VAR();
    var->type = TYPE_NUMBER;
    var->value._number = v;
    return var;
}

static cube_native_var *NATIVE_STRING(char *v)
{
    cube_native_var *var = NATIVE_VAR();
    var->type = TYPE_STRING;
    var->value._string = v;
    return var;
}

static cube_native_var *NATIVE_STRING_COPY(const char *v)
{
    cube_native_var *var = NATIVE_VAR();
    var->type = TYPE_STRING;
    var->value._string = (char *)malloc(sizeof(char) * (strlen(v) + 1));
    strcpy(var->value._string, v);
    return var;
}

static cube_native_var *NATIVE_BYTES()
{
    cube_native_var *var = NATIVE_VAR();
    var->type = TYPE_BYTES;
    var->value._bytes.length = 0;
    var->value._bytes.bytes = NULL;
    return var;
}

static cube_native_var *NATIVE_BYTES_ARG(unsigned int length, unsigned char *bytes)
{
    cube_native_var *var = NATIVE_VAR();
    var->type = TYPE_BYTES;
    var->value._bytes.length = length;
    var->value._bytes.bytes = bytes;
    return var;
}

static cube_native_var *NATIVE_LIST()
{
    cube_native_var *var = NATIVE_VAR();
    var->type = TYPE_VOID;
    var->is_list = true;
    var->size = 0;
    var->capacity = 8;
    var->list = (cube_native_var **)malloc(sizeof(cube_native_var *) * var->capacity);
    return var;
}

static cube_native_var *NATIVE_DICT()
{
    cube_native_var *var = NATIVE_VAR();
    var->type = TYPE_VOID;
    var->is_dict = true;
    var->size = 0;
    var->capacity = 8;
    var->dict = (cube_native_var **)malloc(sizeof(cube_native_var *) * var->capacity);
    return var;
}

static cube_native_var *NATIVE_FUNC()
{
    cube_native_var *var = NATIVE_VAR();
    var->type = TYPE_FUNC;
    return var;
}

static cube_native_var *TO_NATIVE_NULL(cube_native_var *var)
{
    var->type = TYPE_NULL;
    return var;
}

static cube_native_var *TO_NATIVE_BOOL(cube_native_var *var, bool v)
{
    var->type = TYPE_BOOL;
    var->value._bool = v;
    return var;
}

static cube_native_var *TO_NATIVE_NUMBER(cube_native_var *var, double v)
{
    var->type = TYPE_NUMBER;
    var->value._number = v;
    return var;
}

static cube_native_var *TO_NATIVE_STRING(cube_native_var *var, char *v)
{
    var->type = TYPE_STRING;
    var->value._string = v;
    return var;
}

static cube_native_var *TO_NATIVE_BYTES(cube_native_var *var)
{
    var->type = TYPE_BYTES;
    var->value._bytes.length = 0;
    var->value._bytes.bytes = NULL;
    return var;
}

static cube_native_var *TO_NATIVE_BYTES_ARG(cube_native_var *var, unsigned int length, unsigned char *bytes)
{
    var->type = TYPE_BYTES;
    var->value._bytes.length = length;
    var->value._bytes.bytes = bytes;
    return var;
}

static cube_native_var *TO_NATIVE_LIST(cube_native_var *var)
{
    var->type = TYPE_VOID;
    var->is_list = true;
    var->size = 0;
    var->capacity = 8;
    var->list = (cube_native_var **)malloc(sizeof(cube_native_var *) * var->capacity);
    return var;
}

static cube_native_var *TO_NATIVE_DICT(cube_native_var *var)
{
    var->type = TYPE_VOID;
    var->is_dict = true;
    var->key = NULL;
    var->size = 0;
    var->capacity = 8;
    var->dict = (cube_native_var **)malloc(sizeof(cube_native_var *) * var->capacity);
    return var;
}

static cube_native_var *TO_NATIVE_FUNC(cube_native_var *var)
{
    var->type = TYPE_FUNC;
    return var;
}

static void ADD_NATIVE_LIST(cube_native_var *list, cube_native_var *val)
{
    list->list[list->size] = val;
    list->size++;
    if (list->size == list->capacity)
    {
        list->capacity *= 2;
        list->list = (cube_native_var **)realloc(list->list, sizeof(cube_native_var *) * list->capacity);
    }
}

static void ADD_NATIVE_DICT(cube_native_var *dict, char *key, cube_native_var *val)
{
    val->key = key;
    dict->dict[dict->size] = val;
    dict->size++;
    if (dict->size == dict->capacity)
    {
        dict->capacity *= 2;
        dict->dict = (cube_native_var **)realloc(dict->dict, sizeof(cube_native_var *) * dict->capacity);
    }
}

#endif
