#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#include <string.h>

#include "cubeext.h"
#include "mempool.h"
#include "native.h"
#include "util.h"
#include "vm.h"

typedef void (*func_void)();
typedef cube_native_var *(*func_value)();

typedef union {
    func_void f_void;
    func_value f_value;
} lib_func;

typedef struct NativeLibPointer_st
{
    int counter;
    char *path;
    void *handler;
    linked_list *symbols;
    struct NativeLibPointer_st *next;
} NativeLibPointer;

static NativeLibPointer *libPointer = NULL;

#ifdef _WIN32
#include "native_w32.c"
#else
#include "native_posix.c"
#endif

void *getNativeHandler(const char *path)
{
    NativeLibPointer *pt = libPointer;
    while (pt != NULL)
    {
        if (strcmp(pt->path, path) == 0)
        {
            pt->counter++;
            return pt->handler;
        }
        pt = pt->next;
    }
    return NULL;
}

NativeLibPointer *getNativePointer(void *handle)
{
    NativeLibPointer *pt = libPointer;
    while (pt != NULL)
    {
        if (pt->handler == handle)
        {
            return pt;
        }
        pt = pt->next;
    }
    return NULL;
}

void deleteNativePointer(NativeLibPointer *rm)
{
    NativeLibPointer *parent = NULL;
    NativeLibPointer *pt = libPointer;
    while (pt != NULL)
    {
        if (pt == rm)
        {
            if (parent == NULL)
                libPointer = pt->next;
            else
                parent->next = pt->next;

            linked_list_destroy(pt->symbols, true);
            mp_free(pt->path);
            mp_free(pt);
            break;
        }
        parent = pt;
        pt = pt->next;
    }
}

bool openNativeLib(ObjNativeLib *lib)
{
    if (lib->handle == NULL)
    {
        char *path = findFile(lib->name->chars);
        if (path == NULL)
        {
            runtimeError("Unable to find native lib: '%s'", lib->name->chars);
            return false;
        }

        lib->handle = getNativeHandler(path);

        if (lib->handle == NULL)
        {
#ifdef _WIN32
            lib->handle = LoadLibrary(TEXT(path));
#else
            // lib->handle = dlopen(path, RTLD_NOW | RTLD_GLOBAL);
            lib->handle = dlopen(path, RTLD_LAZY);
#endif

            if (lib->handle == NULL)
            {
#ifdef _WIN32
                runtimeError("Unable to open native lib: '%s'\nError: #%ld", lib->name->chars, GetLastError());
#else
                runtimeError("Unable to open native lib: '%s'\nError: %s", lib->name->chars, dlerror());
#endif
                return false;
            }

            NativeLibPointer *pt = (NativeLibPointer *)mp_malloc(sizeof(NativeLibPointer));
            pt->counter = 1;
            pt->next = libPointer;
            pt->handler = lib->handle;
            pt->path = (char *)mp_malloc(sizeof(char) * (strlen(path) + 1));
            pt->symbols = list_symbols(path);
            strcpy(pt->path, path);

            libPointer = pt;

            lib_func fn;
#ifdef _WIN32
            fn.f_void = (func_void)GetProcAddress(lib->handle, "cube_init");
#else
            fn.f_void = (func_void)dlsym(lib->handle, "cube_init");
#endif

            if (fn.f_void != NULL)
                fn.f_void();
        }
    }
    return true;
}

void closeNativeLib(ObjNativeLib *lib)
{
    if (lib->handle != NULL)
    {
        NativeLibPointer *pt = getNativePointer(lib->handle);
        if (pt != NULL)
        {
            pt->counter--;
            if (pt->counter > 0)
            {
                lib->handle = NULL;
                return;
            }
            else
                deleteNativePointer(pt);
        }

        lib_func fn;
#ifdef _WIN32
        fn.f_void = (func_void)GetProcAddress(lib->handle, "cube_release");
#else
        fn.f_void = (func_void)dlsym(lib->handle, "cube_release");
#endif

        if (fn.f_void != NULL)
            fn.f_void();

#ifdef _WIN32
        FreeLibrary(lib->handle);
#else
        dlclose(lib->handle);
#endif
    }
    lib->handle = NULL;
}

Value nativeToValue(cube_native_var *var, NativeTypes *nt)
{
    Value result = NULL_VAL;
    if (IS_NATIVE_LIST(var))
    {
        ObjList *list = initList();

        for (int i = 0; i < var->size; i++)
        {
            result = nativeToValue(var->list[i], nt);
            writeValueArray(&list->values, result);
        }
        *nt = TYPE_LIST;
        result = OBJ_VAL(list);
    }
    else if (IS_NATIVE_DICT(var))
    {
        ObjDict *dict = initDict();

        for (int i = 0; i < var->size; i++)
        {
            result = nativeToValue(var->dict[i], nt);
            insertDict(dict, var->dict[i]->key, result);
        }
        *nt = TYPE_DICT;
        result = OBJ_VAL(dict);
    }
    else
    {
        *nt = NATIVE_TYPE(var);
        switch (NATIVE_TYPE(var))
        {
            case TYPE_VOID:
            case TYPE_NULL: {
                result = NULL_VAL;
            }
            break;
            case TYPE_BOOL: {
                result = BOOL_VAL(AS_NATIVE_BOOL(var));
            }
            break;
            case TYPE_NUMBER: {
                result = NUMBER_VAL(AS_NATIVE_NUMBER(var));
            }
            break;
            case TYPE_STRING: {
                result = STRING_VAL(AS_NATIVE_STRING(var));
            }
            break;
            case TYPE_BYTES: {
                result = BYTES_VAL(AS_NATIVE_BYTES(var).bytes, AS_NATIVE_BYTES(var).length);
            }
            break;
            default:
                break;
        }
    }

    return result;
}

void valueToNative(cube_native_var *var, Value value)
{
    if (IS_NULL(value))
    {
        TO_NATIVE_NULL(var);
    }
    else if (IS_BOOL(value))
    {
        TO_NATIVE_BOOL(var, AS_BOOL(value));
    }
    else if (IS_NUMBER(value))
    {
        TO_NATIVE_NUMBER(var, AS_NUMBER(value));
    }
    else if (IS_STRING(value))
    {
        TO_NATIVE_STRING(var, AS_CSTRING(value));
    }
    else if (IS_BYTES(value))
    {
        ObjBytes *bytes = AS_BYTES(value);
        TO_NATIVE_BYTES_ARG(var, bytes->length, bytes->bytes);
    }
    else if (IS_LIST(value))
    {
        ObjList *list = AS_LIST(value);
        TO_NATIVE_LIST(var);
        for (int i = 0; i < list->values.count; i++)
        {
            cube_native_var *next = NATIVE_VAR();
            valueToNative(next, list->values.values[i]);
            ADD_NATIVE_LIST(var, next);
        }
    }
    else if (IS_DICT(value))
    {
        ObjDict *dict = AS_DICT(value);
        TO_NATIVE_DICT(var);

        for (int i = 0; i < dict->capacity; i++)
        {
            if (!dict->items[i])
                continue;

            cube_native_var *next = NATIVE_VAR();
            valueToNative(next, dict->items[i]->item);
            ADD_NATIVE_DICT(var, dict->items[i]->key, next);
        }
    }
    else if (IS_CLOSURE(value))
    {
        TO_NATIVE_FUNC(var);
    }
    else
    {
        TO_NATIVE_BOOL(var, false);
    }
}

void freeNativeVar(cube_native_var *var, bool skipFirst, bool skipInterns)
{
    if (var == NULL)
        return;

    if (!skipInterns)
    {
        if (var->type == TYPE_STRING)
        {
            free(var->value._string);
        }
        else if (var->type == TYPE_BYTES)
        {
            free(var->value._bytes.bytes);
        }
    }
    if (var->is_list && var->list != NULL)
    {
        for (int i = 0; i < var->size; i++)
        {
            freeNativeVar(var->list[i], false, skipInterns);
        }
        free(var->list);
        var->list = NULL;
        var->size = 0;
        var->capacity = 0;
    }
    if (var->is_dict && var->dict != NULL)
    {
        for (int i = 0; i < var->size; i++)
        {
            freeNativeVar(var->dict[i], false, skipInterns);
        }
        free(var->dict);
        var->dict = NULL;
        var->size = 0;
        var->capacity = 0;
    }
    if (!skipFirst)
        free(var);
}

void freeVarNative(cube_native_var *var)
{
    freeNativeVar(var, false, false);
}

Value callNativeFn(lib_func func, NativeTypes ret, int count, cube_native_var **values)
{
    Value result = NULL_VAL;
    cube_native_var *res = NULL;
    if (ret == TYPE_VOID)
    {
        switch (count)
        {
            case 0:
                func.f_void();
                break;
            case 1:
                func.f_void(values[0]);
                break;
            case 2:
                func.f_void(values[0], values[1]);
                break;
            case 3:
                func.f_void(values[0], values[1], values[2]);
                break;
            case 4:
                func.f_void(values[0], values[1], values[2], values[3]);
                break;
            case 5:
                func.f_void(values[0], values[1], values[2], values[3], values[4]);
                break;
            case 6:
                func.f_void(values[0], values[1], values[2], values[3], values[4], values[5]);
                break;
            case 7:
                func.f_void(values[0], values[1], values[2], values[3], values[4], values[5], values[6]);
                break;
            case 8:
                func.f_void(values[0], values[1], values[2], values[3], values[4], values[5], values[6], values[7]);
                break;
            case 9:
                func.f_void(values[0], values[1], values[2], values[3], values[4], values[5], values[6], values[7],
                            values[8]);
                break;
            case 10:
                func.f_void(values[0], values[1], values[2], values[3], values[4], values[5], values[6], values[7],
                            values[8], values[9]);
                break;
            default:
                break;
        }
    }
    else
    {
        switch (count)
        {
            case 0:
                res = func.f_value();
                break;
            case 1:
                res = func.f_value(values[0]);
                break;
            case 2:
                res = func.f_value(values[0], values[1]);
                break;
            case 3:
                res = func.f_value(values[0], values[1], values[2]);
                break;
            case 4:
                res = func.f_value(values[0], values[1], values[2], values[3]);
                break;
            case 5:
                res = func.f_value(values[0], values[1], values[2], values[3], values[4]);
                break;
            case 6:
                res = func.f_value(values[0], values[1], values[2], values[3], values[4], values[5]);
                break;
            case 7:
                res = func.f_value(values[0], values[1], values[2], values[3], values[4], values[5], values[6]);
                break;
            case 8:
                res = func.f_value(values[0], values[1], values[2], values[3], values[4], values[5], values[6],
                                   values[7]);
                break;
            case 9:
                res = func.f_value(values[0], values[1], values[2], values[3], values[4], values[5], values[6],
                                   values[7], values[8]);
                break;
            case 10:
                res = func.f_value(values[0], values[1], values[2], values[3], values[4], values[5], values[6],
                                   values[7], values[8], values[9]);
                break;
            default:
                break;
        }

        NativeTypes nt;
        result = nativeToValue(res, &nt);

        // if (nt != ret)
        // {
        //     freeNativeVar(res, false, false);
        //     runtimeError("Native function returned an inconsistent type");
        //     return NULL_VAL;
        // }

        freeNativeVar(res, false, false);
    }
    return result;
}

Value callNative(ObjNativeFunc *func, int argCount, Value *args)
{
    if (!openNativeLib(func->lib))
        return NULL_VAL;

    lib_func fn;
#ifdef _WIN32
    fn.f_void = (func_void)GetProcAddress(func->lib->handle, func->name->chars);
#else
    fn.f_void = (func_void)dlsym(func->lib->handle, func->name->chars);
#endif

    if (fn.f_void == NULL)
    {
        runtimeError("Unable to find native func: '%s'", func->name->chars);
        return NULL_VAL;
    }

    if (func->params.count != argCount)
    {
        runtimeError("Invalid number of arguments: required '%d'", func->params.count);
        return NULL_VAL;
    }

    Value result = NULL_VAL;
    cube_native_var **values = (cube_native_var **)malloc(sizeof(cube_native_var *) * 10);
    for (int i = 0; i < 10; i++)
        values[i] = NULL;
    for (int i = 0; i < func->params.count && i < 10; i++)
    {
        NativeTypes type = getNativeType(AS_CSTRING(func->params.values[i]));
        if (type == TYPE_UNKNOWN || type == TYPE_VOID)
        {
            runtimeError("Invalid argument type in %s: '%s'", func->name->chars, AS_CSTRING(func->params.values[i]));
            return NULL_VAL;
        }
        values[i] = NATIVE_VAR();
        switch (type)
        {
            case TYPE_NULL:
                TO_NATIVE_NULL(values[i]);
                break;
            case TYPE_BOOL:
                TO_NATIVE_BOOL(values[i], AS_BOOL(toBool(args[i])));
                break;
            case TYPE_NUMBER:
                TO_NATIVE_NUMBER(values[i], (double)AS_NUMBER(toNumber(args[i])));
                break;
            case TYPE_STRING:
                TO_NATIVE_STRING(values[i], AS_CSTRING(toString(args[i])));
                break;
            case TYPE_BYTES: {
                ObjBytes *bytes = AS_BYTES(toBytes(args[i]));
                TO_NATIVE_BYTES_ARG(values[i], bytes->length, bytes->bytes);
            }
            break;
            case TYPE_LIST:
            case TYPE_DICT:
            case TYPE_VAR: {
                valueToNative(values[i], args[i]);
            }
            break;
            case TYPE_FUNC: {
                TO_NATIVE_FUNC(values[i]);
            }
            break;
            default:
                values[i]->type = TYPE_VOID;
                break;
        }
    }

    NativeTypes retType = getNativeType(func->returnType->chars);
    if (retType == TYPE_UNKNOWN)
    {
        for (int i = 0; i < func->params.count && i < 10; i++)
        {
            freeNativeVar(values[i], false, true);
            values[i] = NULL;
        }
        free(values);
        runtimeError("Invalid return type in %s: '%s'", func->name->chars, func->returnType->chars);
        return NULL_VAL;
    }
    result = callNativeFn(fn, retType, argCount, values);

    for (int i = 0; i < func->params.count && i < 10; i++)
    {
        freeNativeVar(values[i], false, true);
        values[i] = NULL;
    }
    free(values);
    return result;
}

NativeTypes getNativeType(const char *name)
{
    if (strcmp(name, "void") == 0)
        return TYPE_VOID;
    else if (strcmp(name, "null") == 0)
        return TYPE_NULL;
    else if (strcmp(name, "bool") == 0)
        return TYPE_BOOL;
    else if (strcmp(name, "num") == 0)
        return TYPE_NUMBER;
    else if (strcmp(name, "str") == 0)
        return TYPE_STRING;
    else if (strcmp(name, "bytes") == 0)
        return TYPE_BYTES;
    else if (strcmp(name, "list") == 0)
        return TYPE_LIST;
    else if (strcmp(name, "dict") == 0)
        return TYPE_DICT;
    else if (strcmp(name, "var") == 0)
        return TYPE_VAR;
    else if (strcmp(name, "func") == 0)
        return TYPE_FUNC;

    return TYPE_UNKNOWN;
}

static bool symbolsNativeLib(int argCount, int type)
{
    if (argCount != 1)
    {
        runtimeError("symbols() takes 1 argument (%d given)", argCount);
        return false;
    }

    ObjNativeLib *lib = AS_NATIVE_LIB(pop());
    if (!openNativeLib(lib))
    {
        runtimeError("Unable to open native library: '%s'", lib->name->chars);
        return false;
    }

    NativeLibPointer *pt = getNativePointer(lib->handle);

    ObjList *list = initList();

    if (pt != NULL && pt->symbols != NULL)
    {
        do
        {
            symbol_t *sym = linked_list_get(pt->symbols);
            if (sym != NULL)
            {
                if (type == 0)
                    writeValueArray(&list->values, STRING_VAL(sym->name));
                else if (type == 1 && sym->is_func)
                    writeValueArray(&list->values, STRING_VAL(sym->name));
                else if (type == 2 && !sym->is_func)
                    writeValueArray(&list->values, STRING_VAL(sym->name));
            }
        } while (linked_list_next(&pt->symbols));
        linked_list_reset(&pt->symbols);
    }

    push(OBJ_VAL(list));
    return true;
}

static bool getNativeLib(int argCount)
{
    if (argCount != 3)
    {
        runtimeError("get() takes 2 arguments (%d given)", argCount);
        return false;
    }

    ObjString *typeName = AS_STRING(pop());
    ObjString *name = AS_STRING(pop());

    ObjNativeLib *lib = AS_NATIVE_LIB(pop());
    if (!openNativeLib(lib))
    {
        runtimeError("Unable to open native library: '%s'", lib->name->chars);
        return false;
    }

    void *obj = NULL;
#ifdef _WIN32
    obj = (func_void)GetProcAddress(lib->handle, name->chars);
#else
    obj = (func_void)dlsym(lib->handle, name->chars);
#endif

    if (obj == NULL)
    {
        runtimeError("Unable to find native symbol: '%s'", name->chars);
        return false;
    }

    Value val = NULL_VAL;

    if (strcmp(typeName->chars, "bool") == 0)
    {
        bool *ptr = (bool *)obj;
        val = BOOL_VAL(*ptr);
    }
    else if (strcmp(typeName->chars, "num") == 0)
    {
        double *ptr = (double *)obj;
        val = NUMBER_VAL(*ptr);
    }
    else if (strcmp(typeName->chars, "double") == 0)
    {
        double *ptr = (double *)obj;
        val = NUMBER_VAL(*ptr);
    }
    else if (strcmp(typeName->chars, "float") == 0)
    {
        float *ptr = (float *)obj;
        val = NUMBER_VAL(*ptr);
    }
    else if (strcmp(typeName->chars, "int") == 0)
    {
        int *ptr = (int *)obj;
        val = NUMBER_VAL(*ptr);
    }
    else if (strcmp(typeName->chars, "str") == 0)
    {
        char *ptr = (char *)obj;
        val = STRING_VAL(ptr);
    }
    else if (strcmp(typeName->chars, "char") == 0)
    {
        char *ptr = (char *)obj;
        char _ptr[2];
        _ptr[0] = ptr[0];
        _ptr[1] = '\0';
        val = STRING_VAL(_ptr);
    }

    push(val);

    return true;
}

static bool setNativeLib(int argCount)
{
    if (argCount != 3)
    {
        runtimeError("set() takes 3 arguments (%d given)", argCount);
        return false;
    }

    Value value = pop();
    ObjString *name = AS_STRING(pop());

    ObjNativeLib *lib = AS_NATIVE_LIB(pop());
    if (!openNativeLib(lib))
    {
        runtimeError("Unable to open native library: '%s'", lib->name->chars);
        return false;
    }

    void *obj = NULL;
#ifdef _WIN32
    obj = (func_void)GetProcAddress(lib->handle, name->chars);
#else
    obj = (func_void)dlsym(lib->handle, name->chars);
#endif

    if (obj == NULL)
    {
        runtimeError("Unable to find native symbol: '%s'", name->chars);
        return false;
    }

    Value val = FALSE_VAL;

    if (IS_BOOL(value))
    {
        bool *ptr = (bool *)obj;
        *ptr = AS_BOOL(value);
        val = TRUE_VAL;
    }
    else if (IS_NUMBER(value))
    {
        double *ptr = (double *)obj;
        *ptr = AS_NUMBER(value);
        val = TRUE_VAL;
    }
    else if (IS_STRING(value))
    {
        char *ptr = (char *)obj;
        strcpy(ptr, AS_CSTRING(value));
        val = TRUE_VAL;
    }

    push(val);
    return true;
}

static bool setFloatNativeLib(int argCount)
{
    if (argCount != 4)
    {
        runtimeError("setFloat() takes 3 arguments (%d given)", argCount);
        return false;
    }

    Value value = pop();
    ObjString *name = AS_STRING(pop());

    ObjNativeLib *lib = AS_NATIVE_LIB(pop());
    if (!openNativeLib(lib))
    {
        runtimeError("Unable to open native library: '%s'", lib->name->chars);
        return false;
    }

    void *obj = NULL;
#ifdef _WIN32
    obj = (func_void)GetProcAddress(lib->handle, name->chars);
#else
    obj = (func_void)dlsym(lib->handle, name->chars);
#endif

    if (obj == NULL)
    {
        runtimeError("Unable to find native symbol: '%s'", name->chars);
        return false;
    }

    Value val = FALSE_VAL;

    float *ptr = (float *)obj;

    if (IS_BOOL(value))
    {
        *ptr = AS_BOOL(value);
        val = TRUE_VAL;
    }
    else if (IS_NUMBER(value))
    {
        *ptr = AS_NUMBER(value);
        val = TRUE_VAL;
    }

    push(val);
    return true;
}

static bool setIntNativeLib(int argCount)
{
    if (argCount != 4)
    {
        runtimeError("setInt() takes 3 arguments (%d given)", argCount);
        return false;
    }

    Value value = pop();
    ObjString *name = AS_STRING(pop());

    ObjNativeLib *lib = AS_NATIVE_LIB(pop());
    if (!openNativeLib(lib))
    {
        runtimeError("Unable to open native library: '%s'", lib->name->chars);
        return false;
    }

    void *obj = NULL;
#ifdef _WIN32
    obj = (func_void)GetProcAddress(lib->handle, name->chars);
#else
    obj = (func_void)dlsym(lib->handle, name->chars);
#endif

    if (obj == NULL)
    {
        runtimeError("Unable to find native symbol: '%s'", name->chars);
        return false;
    }

    Value val = FALSE_VAL;

    int *ptr = (int *)obj;

    if (IS_BOOL(value))
    {
        *ptr = AS_BOOL(value);
        val = TRUE_VAL;
    }
    else if (IS_NUMBER(value))
    {
        *ptr = AS_NUMBER(value);
        val = TRUE_VAL;
    }

    push(val);
    return true;
}

static bool setCharNativeLib(int argCount)
{
    if (argCount != 4)
    {
        runtimeError("setChar() takes 3 arguments (%d given)", argCount);
        return false;
    }

    Value value = pop();
    ObjString *name = AS_STRING(pop());

    ObjNativeLib *lib = AS_NATIVE_LIB(pop());
    if (!openNativeLib(lib))
    {
        runtimeError("Unable to open native library: '%s'", lib->name->chars);
        return false;
    }

    void *obj = NULL;
#ifdef _WIN32
    obj = (func_void)GetProcAddress(lib->handle, name->chars);
#else
    obj = (func_void)dlsym(lib->handle, name->chars);
#endif

    if (obj == NULL)
    {
        runtimeError("Unable to find native symbol: '%s'", name->chars);
        return false;
    }

    Value val = FALSE_VAL;

    char *ptr = (char *)obj;

    if (IS_BOOL(value))
    {
        *ptr = AS_BOOL(value);
        val = TRUE_VAL;
    }
    else if (IS_NUMBER(value))
    {
        *ptr = (char)AS_NUMBER(value);
        val = TRUE_VAL;
    }
    else if (IS_STRING(value))
    {
        *ptr = AS_CSTRING(value)[0];
        val = TRUE_VAL;
    }

    push(val);
    return true;
}

static bool callNativeLib(int argCount)
{
    if (argCount < 2 || argCount > 3)
    {
        runtimeError("call() takes 2 or 3 arguments (%d given)", argCount);
        return false;
    }

    ObjList *list = NULL;
    int nargs = 0;
    Value args = NULL_VAL;
    if (argCount == 3)
    {
        args = pop();
        nargs = 1;
    }

    if (nargs == 1)
    {
        if (!IS_LIST(args))
        {
            list = initList();
            writeValueArray(&list->values, args);
            args = OBJ_VAL(list);
        }
        list = AS_LIST(args);
        nargs = list->values.count;
    }

    ObjString *name = AS_STRING(pop());

    ObjNativeLib *lib = AS_NATIVE_LIB(pop());
    if (!openNativeLib(lib))
    {
        runtimeError("Unable to open native library: '%s'", lib->name->chars);
        return false;
    }

    lib_func fn;
#ifdef _WIN32
    fn.f_void = (func_void)GetProcAddress(lib->handle, name->chars);
#else
    fn.f_void = (func_void)dlsym(lib->handle, name->chars);
#endif

    if (fn.f_void == NULL)
    {
        runtimeError("Unable to find native symbol: '%s'", name->chars);
        return false;
    }

    Value result = NULL_VAL;
    cube_native_var **values = (cube_native_var **)malloc(sizeof(cube_native_var *) * 10);
    for (int i = 0; i < 10; i++)
        values[i] = NULL;
    for (int i = 0; i < nargs && i < 10; i++)
    {
        values[i] = NATIVE_VAR();
        valueToNative(values[i], list->values.values[i]);
    }

    result = callNativeFn(fn, TYPE_VOID, argCount, values);

    for (int i = 0; i < nargs && i < 10; i++)
    {
        freeNativeVar(values[i], false, true);
        values[i] = NULL;
    }
    free(values);

    push(result);

    return true;
}

static bool callRetNativeLib(int argCount)
{
    if (argCount < 3 || argCount > 4)
    {
        runtimeError("callRet() takes 3 or 4 arguments (%d given)", argCount);
        return false;
    }

    ObjList *list = NULL;
    int nargs = 0;
    Value args = NULL_VAL;
    if (argCount == 4)
    {
        args = pop();
        nargs = 1;
    }

    if (nargs == 1)
    {
        if (!IS_LIST(args))
        {
            list = initList();
            writeValueArray(&list->values, args);
            args = OBJ_VAL(list);
        }
        list = AS_LIST(args);
        nargs = list->values.count;
    }

    ObjString *returnType = AS_STRING(pop());

    ObjString *name = AS_STRING(pop());

    ObjNativeLib *lib = AS_NATIVE_LIB(pop());
    if (!openNativeLib(lib))
    {
        runtimeError("Unable to open native library: '%s'", lib->name->chars);
        return false;
    }

    lib_func fn;
#ifdef _WIN32
    fn.f_void = (func_void)GetProcAddress(lib->handle, name->chars);
#else
    fn.f_void = (func_void)dlsym(lib->handle, name->chars);
#endif

    if (fn.f_void == NULL)
    {
        runtimeError("Unable to find native symbol: '%s'", name->chars);
        return false;
    }

    Value result = NULL_VAL;
    cube_native_var **values = (cube_native_var **)malloc(sizeof(cube_native_var *) * 10);
    for (int i = 0; i < 10; i++)
        values[i] = NULL;
    for (int i = 0; i < nargs && i < 10; i++)
    {
        values[i] = NATIVE_VAR();
        valueToNative(values[i], list->values.values[i]);
    }

    NativeTypes retType = getNativeType(returnType->chars);
    if (retType == TYPE_UNKNOWN)
        retType = TYPE_VOID;

    result = callNativeFn(fn, retType, argCount, values);

    for (int i = 0; i < nargs && i < 10; i++)
    {
        freeNativeVar(values[i], false, true);
        values[i] = NULL;
    }
    free(values);

    push(result);

    return true;
}

bool nativeLibMethods(char *method, int argCount)
{
    if (strcmp(method, "symbols") == 0)
        return symbolsNativeLib(argCount, 0);
    else if (strcmp(method, "functions") == 0)
        return symbolsNativeLib(argCount, 1);
    else if (strcmp(method, "vars") == 0)
        return symbolsNativeLib(argCount, 2);
    else if (strcmp(method, "get") == 0)
        return getNativeLib(argCount);
    else if (strcmp(method, "set") == 0)
        return setNativeLib(argCount);
    else if (strcmp(method, "setFloat") == 0)
        return setFloatNativeLib(argCount);
    else if (strcmp(method, "setInt") == 0)
        return setIntNativeLib(argCount);
    else if (strcmp(method, "setChar") == 0)
        return setCharNativeLib(argCount);
    else if (strcmp(method, "call") == 0)
        return callNativeLib(argCount);
    else if (strcmp(method, "callRet") == 0)
        return callRetNativeLib(argCount);

    runtimeError("NativeLib has no method %s()", method);
    return false;
}