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

typedef enum
{
    TYPE_VOID,
    TYPE_NONE,
    TYPE_BOOL,
    TYPE_NUMBER,
    TYPE_STRING,
    TYPE_BYTES,
    TYPE_LIST,
    TYPE_DICT,
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

typedef struct cube_native_var_t {
    NativeTypes type;
    bool is_list;
    bool is_dict;
    cube_native_value value;
    char *key;
    struct cube_native_var_t *next;
} cube_native_var;


#define NATIVE_TYPE(var) (var->type)
#define AS_NATIVE_BOOL(var) var->value._bool
#define AS_NATIVE_NUMBER(var) var->value._number
#define AS_NATIVE_STRING(var) var->value._string
#define AS_NATIVE_BYTES(var) var->value._bytes
#define IS_NATIVE_LIST(var) (var->is_list)
#define IS_NATIVE_DICT(var) (var->is_dict)
#define NATIVE_NEXT(var) (var->next)
#define HAS_NATIVE_NEXT(var) (var->next != NULL)

static cube_native_var* NATIVE_VAR()
{
    cube_native_var* var = (cube_native_var*)malloc(sizeof(cube_native_var));
    var->is_list = false;
    var->is_dict = false;
    var->type = TYPE_VOID;
    var->next = NULL;
    var->key = NULL;
    return var;
}

static cube_native_var* NATIVE_NONE()
{
    cube_native_var* var = NATIVE_VAR();
    var->type = TYPE_NONE;
    return var;
}

static cube_native_var* NATIVE_BOOL(bool v)
{
    cube_native_var* var = NATIVE_VAR();
    var->type = TYPE_BOOL;
    var->value._bool = v;
    return var;
}

static cube_native_var* NATIVE_NUMBER(double v)
{
    cube_native_var* var = NATIVE_VAR();
    var->type = TYPE_NUMBER;
    var->value._number = v;
    return var;
}

static cube_native_var* NATIVE_STRING(char* v)
{
    cube_native_var* var = NATIVE_VAR();
    var->type = TYPE_STRING;
    var->value._string = v;
    return var;
}

static cube_native_var* NATIVE_STRING_COPY(const char* v)
{
    cube_native_var* var = NATIVE_VAR();
    var->type = TYPE_STRING;
    var->value._string = (char*)malloc(sizeof(char) * (strlen(v) + 1) );
    strcpy(var->value._string, v);
    return var;
}

static cube_native_var* NATIVE_BYTES()
{
    cube_native_var* var = NATIVE_VAR();
    var->type = TYPE_BYTES;
    var->value._bytes.length = 0;
    var->value._bytes.bytes = NULL;
    return var;
}

static cube_native_var* NATIVE_LIST()
{
    cube_native_var* var = NATIVE_VAR();
    var->type = TYPE_VOID;
    var->is_list = true;
    var->next = NULL;
    return var;
}

static cube_native_var* NATIVE_DICT()
{
    cube_native_var* var = NATIVE_VAR();
    var->type = TYPE_VOID;
    var->is_dict = true;
    var->key = NULL;
    var->next = NULL;
    return var;
}

static void ADD_NATIVE_LIST(cube_native_var *list, cube_native_var *val)
{
    if(list->next == NULL)
    {
        list->next = val;
    }
    else
    {
        ADD_NATIVE_LIST(list->next, val);
    }
}

static void ADD_NATIVE_DICT(cube_native_var *dict, char *key, cube_native_var *val)
{
    if(dict->next == NULL)
    {
        dict->next = val;
        dict->key = key;
    }
    else
    {
        ADD_NATIVE_DICT(dict->next, key, val);
    }
}

#endif
