#ifdef _WIN32
#include <windows.h>
#else
#define _GNU_SOURCE
#ifdef __APPLE__
#include <mach-o/dyld.h>
#else // Linux
#include <link.h>
#endif
#include <dlfcn.h>
#endif

#include <string.h>

#include <ffi.h>

#include "cubeext.h"
#include "mempool.h"
#include "native.h"
#include "threads.h"
#include "util.h"
#include "vm.h"

typedef void (*func_void)();

typedef union {
    bool _bool;
    uint8_t _uint8;
    uint16_t _uint16;
    uint32_t _uint32;
    uint64_t _uint64;
    int8_t _sint8;
    int16_t _sint16;
    int32_t _sint32;
    int64_t _sint64;
    float _float32;
    double _float64;
    void *_ptr;
} var_value_t;

typedef struct
{
    var_value_t val;
    bool alloc;
} var_t;

typedef struct NativeLibPointer_st
{
    int counter;
    char *path;
    void *handler;
    linked_list *symbols;
    struct NativeLibPointer_st *next;
} NativeLibPointer;

static NativeLibPointer *libPointer = NULL;

extern linked_list *list_symbols(const char *path);

Value nativeToValue(cube_native_var *var, NativeTypes *nt);

const char *dlpath(void *handle)
{
    const char *path = NULL;
#ifdef __APPLE__
    for (int32_t i = _dyld_image_count(); i >= 0; i--)
    {

        bool found = FALSE;
        const char *probe_path = _dyld_get_image_name(i);
        void *probe_handle = dlopen(probe_path, RTLD_NOW | RTLD_LOCAL | RTLD_NOLOAD);

        if (handle == probe_handle)
        {
            found = TRUE;
            path = probe_path;
        }

        dlclose(probe_handle);

        if (found)
            break;
    }
#else // Linux
#ifdef _WIN32
    char Filename[MAX_PATH];
    int size = GetModuleFileNameA(handle, Filename, MAX_PATH);
    if (size != 0)
    {
        path = malloc(size + 1);
        strcpy(path, Filename);
    }
#else
    struct link_map *map;
    dlinfo(handle, RTLD_DI_LINKMAP, &map);

    if (map)
        path = map->l_name;
#endif
#endif
    return path;
}

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

void native_callback(void *fn, cube_native_var *argsNative)
{
    ObjClosure *closure = (ObjClosure *)fn;
    if (closure == NULL)
        return;

    Value args;
    ObjList *list = NULL;
    NativeTypes nt;
    if (argsNative != NULL)
    {
        args = nativeToValue(argsNative, &nt);
        if (!IS_LIST(args))
        {
            list = initList();
            writeValueArray(&list->values, args);
            args = OBJ_VAL(list);
        }
        else
        {
            list = AS_LIST(args);
        }
    }
    else
    {
        list = initList();
        args = OBJ_VAL(list);
    }

    ThreadFrame *threadFrame = currentThread();

    char *name = (char *)mp_malloc(sizeof(char) * 32);
    name[0] = '\0';
    sprintf(name, "Task[%d-%d]", thread_id(), threadFrame->tasksCount);
    threadFrame->tasksCount++;

    TaskFrame *tf = createTaskFrame(name);

    ObjString *strTaskName = AS_STRING(STRING_VAL(name));
    mp_free(name);

    // Push the context
    *tf->stackTop = OBJ_VAL(closure);
    tf->stackTop++;

    // Push the args
    int M = closure->function->arity;
    if (list->values.count < M)
        M = list->values.count;

    for (int i = 0; i < M; i++)
    {
        *tf->stackTop = list->values.values[i];
        tf->stackTop++;
    }

    // Add the context to the frames stack

    CallFrame *frame = &tf->frames[tf->frameCount++];
    frame->closure = closure;
    frame->ip = closure->function->chunk.code;
    frame->package = threadFrame->frame->package;
    frame->type = CALL_FRAME_TYPE_FUNCTION;
    frame->nextPackage = NULL;
    frame->require = false;

    frame->slots = tf->stackTop - M - 1;

    tf->currentArgs = args;

    ((ThreadFrame *)tf->threadFrame)->running = true;
}

bool openNativeLib(ObjNativeLib *lib)
{
    if (lib->handle == NULL)
    {
        char *path = findFile(lib->name->chars);
        if (path == NULL)
        {
            // runtimeError("Unable to find native lib: '%s'", lib->name->chars);
            // return false;
            path = lib->name->chars;
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

            char *p2 = (char *)dlpath(lib->handle);
            if (p2)
                path = p2;

            NativeLibPointer *pt = (NativeLibPointer *)mp_malloc(sizeof(NativeLibPointer));
            pt->counter = 1;
            pt->next = libPointer;
            pt->handler = lib->handle;
            pt->path = (char *)mp_malloc(sizeof(char) * (strlen(path) + 1));
            pt->symbols = list_symbols(path);
            strcpy(pt->path, path);

            libPointer = pt;

            func_void fn;
#ifdef _WIN32
            fn = (func_void)GetProcAddress(lib->handle, "cube_init");
#else
            fn = (func_void)dlsym(lib->handle, "cube_init");
#endif

            if (fn != NULL)
                fn();
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

        func_void fn;
#ifdef _WIN32
        fn = (func_void)GetProcAddress(lib->handle, "cube_release");
#else
        fn = (func_void)dlsym(lib->handle, "cube_release");
#endif

        if (fn != NULL)
            fn();

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
        TO_NATIVE_FUNC(var, (cube_native_func)native_callback, AS_CLOSURE(value));
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

void free_var(var_t var)
{
    if (var.alloc)
    {
        freeNativeVar((cube_native_var *)var.val._ptr, false, true);
    }
}

int to_var(var_t *var, Value value, NativeTypes type, ffi_type **ffi_arg)
{
    int sz = 0;
    switch (type)
    {

        case TYPE_NULL:
        case TYPE_BOOL:
        case TYPE_NUMBER:
        case TYPE_STRING:
        case TYPE_BYTES:
        case TYPE_LIST:
        case TYPE_DICT:
        case TYPE_VAR:
        case TYPE_FUNC: {
            *ffi_arg = &ffi_type_pointer;
            var->val._ptr = NATIVE_VAR();
            var->alloc = true;
            valueToNative((cube_native_var *)var->val._ptr, value);
            sz = sizeof(void *);
        }
        break;
        case TYPE_VOID:
            *ffi_arg = &ffi_type_void;
            var->val._uint64 = 0;
            sz = sizeof(void *);
            break;
        case TYPE_CBOOL:
            *ffi_arg = &ffi_type_sint8;
            var->val._bool = (bool)(AS_BOOL(toBool(value)));
            sz = sizeof(bool);
            break;
        case TYPE_INT8:
            *ffi_arg = &ffi_type_sint8;
            var->val._sint8 = (int8_t)(AS_NUMBER(toNumber(value)));
            sz = sizeof(int8_t);
            break;
        case TYPE_INT16:
            *ffi_arg = &ffi_type_sint16;
            var->val._sint16 = (int16_t)(AS_NUMBER(toNumber(value)));
            sz = sizeof(int16_t);
            break;
        case TYPE_INT32:
            *ffi_arg = &ffi_type_sint32;
            var->val._sint32 = (int32_t)(AS_NUMBER(toNumber(value)));
            sz = sizeof(int32_t);
            break;
        case TYPE_INT64:
            *ffi_arg = &ffi_type_sint64;
            var->val._sint64 = (int64_t)(AS_NUMBER(toNumber(value)));
            sz = sizeof(int64_t);
            break;
        case TYPE_UINT8:
            *ffi_arg = &ffi_type_uint8;
            var->val._uint8 = (uint8_t)(AS_NUMBER(toNumber(value)));
            sz = sizeof(uint8_t);
            break;
        case TYPE_UINT16:
            *ffi_arg = &ffi_type_uint16;
            var->val._uint16 = (uint16_t)(AS_NUMBER(toNumber(value)));
            sz = sizeof(uint16_t);
            break;
        case TYPE_UINT32:
            *ffi_arg = &ffi_type_uint32;
            var->val._uint32 = (uint32_t)(AS_NUMBER(toNumber(value)));
            sz = sizeof(uint32_t);
            break;
        case TYPE_UINT64:
            *ffi_arg = &ffi_type_uint64;
            var->val._uint64 = (uint64_t)(AS_NUMBER(toNumber(value)));
            sz = sizeof(uint64_t);
            break;
        case TYPE_FLOAT32:
            *ffi_arg = &ffi_type_float;
            var->val._float32 = (float)(AS_NUMBER(toNumber(value)));
            sz = sizeof(float);
            break;
        case TYPE_FLOAT64:
            *ffi_arg = &ffi_type_double;
            var->val._float64 = (double)(AS_NUMBER(toNumber(value)));
            sz = sizeof(double);
            break;
        case TYPE_CSTRING:
            *ffi_arg = &ffi_type_pointer;
            var->val._ptr = (char *)(AS_CSTRING(toString(value)));
            sz = sizeof(char *);
            break;
        case TYPE_CBYTES:
            *ffi_arg = &ffi_type_pointer;
            var->val._ptr = (uint8_t *)(AS_CBYTES(toBytes(value)));
            sz = sizeof(uint8_t *);
            break;
        default:
            *ffi_arg = &ffi_type_pointer;
            var->val._ptr = NATIVE_NULL();
            var->alloc = true;
            sz = sizeof(void *);
            break;
    }
    return sz;
}

int size_var(NativeTypes type)
{
    int sz = 0;
    switch (type)
    {

        case TYPE_NULL:
        case TYPE_BOOL:
        case TYPE_NUMBER:
        case TYPE_STRING:
        case TYPE_BYTES:
        case TYPE_LIST:
        case TYPE_DICT:
        case TYPE_FUNC:
        case TYPE_VAR:
            sz = sizeof(void *);
            break;
        case TYPE_VOID:
            sz = sizeof(void *);
            break;
        case TYPE_CBOOL:
            sz = sizeof(bool);
            break;
        case TYPE_INT8:
            sz = sizeof(int8_t);
            break;
        case TYPE_INT16:
            sz = sizeof(int16_t);
            break;
        case TYPE_INT32:
            sz = sizeof(int32_t);
            break;
        case TYPE_INT64:
            sz = sizeof(int64_t);
            break;
        case TYPE_UINT8:
            sz = sizeof(uint8_t);
            break;
        case TYPE_UINT16:
            sz = sizeof(uint16_t);
            break;
        case TYPE_UINT32:
            sz = sizeof(uint32_t);
            break;
        case TYPE_UINT64:
            sz = sizeof(uint64_t);
            break;
        case TYPE_FLOAT32:
            sz = sizeof(float);
            break;
        case TYPE_FLOAT64:
            sz = sizeof(double);
            break;
        case TYPE_CSTRING:
            sz = sizeof(char *);
            break;
        case TYPE_CBYTES:
            sz = sizeof(uint8_t *);
            break;
        default:
            sz = sizeof(void *);
            break;
    }
    return sz;
}

int size_struct(ObjNativeStruct *str)
{
    int count = str->names.count;

    int sz = 0;

    for (int i = 0; i < count; i++)
    {
        NativeTypes type = getNativeType(AS_CSTRING(str->types.values[i]));
        if (type == TYPE_UNKNOWN || type == TYPE_VOID)
        {
            ObjNativeStruct *str = getNativeStruct(str->lib, AS_CSTRING(str->types.values[i]));
            if (str == NULL)
            {
                runtimeError("Invalid argument type in %s: '%s'", str->name->chars, AS_CSTRING(str->types.values[i]));
                return -1;
            }
            else
            {
                sz += size_struct(str);
                if (sz < 0)
                {
                    return -1;
                }
            }
        }
        else
        {
            sz += size_var(type);
        }
    }

    return sz;
}

int to_struct(var_t *var, Value value, ObjNativeStruct *str, ffi_type **ffi_arg)
{
    if (!IS_DICT(value))
    {
        runtimeError("Invalid struct value for %s: '%s'", str->name->chars, valueType(value));
        return -1;
    }

    ObjDict *dict = AS_DICT(value);

    int count = str->names.count;

    ffi_type *st_type = (ffi_type *)malloc(sizeof(ffi_type));
    ffi_type **st_type_elements = (ffi_type **)malloc(sizeof(ffi_type *) * (count + 1));
    var_t st_type_var;
    int i;

    st_type->size = st_type->alignment = 0;
    st_type->type = FFI_TYPE_STRUCT;
    st_type->elements = st_type_elements;

    int sz = 0;
    int cr = 0;
    int cp = 128;
    var->val._ptr = malloc(cp);

    for (i = 0; i < count; i++)
    {
        Value itemValue = searchDict(dict, AS_CSTRING(str->names.values[i]));

        NativeTypes type = getNativeType(AS_CSTRING(str->types.values[i]));
        if (type == TYPE_UNKNOWN || type == TYPE_VOID)
        {
            ObjNativeStruct *str = getNativeStruct(str->lib, AS_CSTRING(str->types.values[i]));
            if (str == NULL)
            {
                free(st_type);
                free(st_type_elements);
                runtimeError("Invalid argument type in %s: '%s'", str->name->chars, AS_CSTRING(str->types.values[i]));
                return -1;
            }
            else
            {
                st_type_var.alloc = false;
                st_type_var.val._ptr = NULL;

                sz = to_struct(&st_type_var, itemValue, str, &st_type_elements[i]);
                if (sz < 0)
                {
                    free(st_type);
                    free(st_type_elements);
                    return -1;
                }

                if (sz + cr > cp)
                {
                    while (sz + cr > cp)
                    {
                        cp *= 2;
                    }
                    var->val._ptr = realloc(var->val._ptr, cp);
                }

                memcpy((uint8_t *)var->val._ptr + cr, st_type_var.val._ptr, sz);
                cr += sz;
            }
        }
        else
        {
            st_type_var.alloc = false;

            sz = to_var(&st_type_var, itemValue, type, &st_type_elements[i]);

            if (sz + cr > cp)
            {
                while (sz + cr > cp)
                {
                    cp *= 2;
                }
                var->val._ptr = realloc(var->val._ptr, cp);
            }

            memcpy((uint8_t *)var->val._ptr + cr, &st_type_var.val, sz);
            cr += sz;
        }
    }

    st_type_elements[count] = NULL;
    *ffi_arg = st_type;
    return cr;
}

ffi_type *prepare_ret_var(var_t *var, NativeTypes type)
{
    ffi_type *ffi_ret_type = &ffi_type_void;
    var->alloc = false;

    switch (type)
    {
        case TYPE_NULL:
        case TYPE_BOOL:
        case TYPE_NUMBER:
        case TYPE_STRING:
        case TYPE_BYTES:
        case TYPE_LIST:
        case TYPE_DICT:
        case TYPE_VAR:
        case TYPE_FUNC: {
            ffi_ret_type = &ffi_type_pointer;
            var->alloc = true;
        }
        break;
        case TYPE_CBOOL:
            ffi_ret_type = &ffi_type_sint8;
            break;
        case TYPE_INT8:
            ffi_ret_type = &ffi_type_sint8;
            break;
        case TYPE_INT16:
            ffi_ret_type = &ffi_type_sint16;
            break;
        case TYPE_INT32:
            ffi_ret_type = &ffi_type_sint32;
            break;
        case TYPE_INT64:
            ffi_ret_type = &ffi_type_sint64;
            break;
        case TYPE_UINT8:
            ffi_ret_type = &ffi_type_uint8;
            break;
        case TYPE_UINT16:
            ffi_ret_type = &ffi_type_uint16;
            break;
        case TYPE_UINT32:
            ffi_ret_type = &ffi_type_uint32;
            break;
        case TYPE_UINT64:
            ffi_ret_type = &ffi_type_uint64;
            break;
        case TYPE_FLOAT32:
            ffi_ret_type = &ffi_type_float;
            break;
        case TYPE_FLOAT64:
            ffi_ret_type = &ffi_type_double;
            break;
        case TYPE_CSTRING:
            ffi_ret_type = &ffi_type_pointer;
            break;
        case TYPE_CBYTES:
            ffi_ret_type = &ffi_type_pointer;
        default:
            break;
    }

    return ffi_ret_type;
}

ffi_type *prepare_ret_struct(var_t *var, ObjNativeStruct *str)
{
    int count = str->names.count;

    ffi_type *st_type = (ffi_type *)malloc(sizeof(ffi_type));
    ffi_type **st_type_elements = (ffi_type **)malloc(sizeof(ffi_type *) * (count + 1));
    int i;

    st_type->size = st_type->alignment = 0;
    st_type->type = FFI_TYPE_STRUCT;
    st_type->elements = st_type_elements;

    for (i = 0; i < count; i++)
    {
        NativeTypes type = getNativeType(AS_CSTRING(str->types.values[i]));
        if (type == TYPE_UNKNOWN || type == TYPE_VOID)
        {
            ObjNativeStruct *str = getNativeStruct(str->lib, AS_CSTRING(str->types.values[i]));
            if (str == NULL)
            {
                free(st_type);
                free(st_type_elements);
                runtimeError("Invalid argument type in %s: '%s'", str->name->chars, AS_CSTRING(str->types.values[i]));
                return NULL;
            }
            else
            {
                st_type_elements[i] = prepare_ret_struct(var, str);
            }
        }
        else
        {
            st_type_elements[i] = prepare_ret_var(var, type);
        }
    }

    st_type_elements[count] = NULL;
    int sz = size_struct(str);
    if (sz < 0)
        return NULL;

    var->val._ptr = malloc(sz);
    return st_type;
}

Value from_var(var_t *var, NativeTypes retType)
{
    Value result = NULL_VAL;

    switch (retType)
    {
        case TYPE_NULL:
        case TYPE_BOOL:
        case TYPE_NUMBER:
        case TYPE_STRING:
        case TYPE_BYTES:
        case TYPE_LIST:
        case TYPE_DICT:
        case TYPE_VAR:
        case TYPE_FUNC: {
            if (var->val._ptr)
            {
                NativeTypes nt;
                result = nativeToValue((cube_native_var *)var->val._ptr, &nt);
                freeNativeVar((cube_native_var *)var->val._ptr, false, false);
            }
        }
        break;
        case TYPE_CBOOL:
            result = BOOL_VAL((bool)(var->val._bool));
            break;
        case TYPE_INT8:
            result = NUMBER_VAL((int8_t)(var->val._sint8));
            break;
        case TYPE_INT16:
            result = NUMBER_VAL((int16_t)(var->val._sint16));
            break;
        case TYPE_INT32:
            result = NUMBER_VAL((int32_t)(var->val._sint32));
            break;
        case TYPE_INT64:
            result = NUMBER_VAL((uint64_t)(var->val._sint64));
            break;
        case TYPE_UINT8:
            result = NUMBER_VAL((uint8_t)(var->val._uint8));
            break;
        case TYPE_UINT16:
            result = NUMBER_VAL((uint16_t)(var->val._uint16));
            break;
        case TYPE_UINT32:
            result = NUMBER_VAL((uint32_t)(var->val._uint32));
            break;
        case TYPE_UINT64:
            result = NUMBER_VAL((uint64_t)(var->val._uint64));
            break;
        case TYPE_FLOAT32:
            result = NUMBER_VAL((float)(var->val._float32));
            break;
        case TYPE_FLOAT64:
            result = NUMBER_VAL((double)(var->val._float64));
            break;
        case TYPE_CSTRING:
            if (var->val._ptr)
                result = STRING_VAL((char *)var->val._ptr);
            break;
        case TYPE_CBYTES:
            if (var->val._ptr)
                result = UNSAFE_VAL((uint8_t *)var->val._ptr);
            break;
        default:
            break;
    }

    return result;
}

Value from_struct(void *ptr, ObjNativeStruct *str)
{
    unsigned char *data = ptr;

    ObjDict *dict = initDict();

    int count = str->names.count;
    int sz;
    var_t var;

    for (int i = 0; i < count; i++)
    {
        NativeTypes type = getNativeType(AS_CSTRING(str->types.values[i]));
        if (type == TYPE_UNKNOWN || type == TYPE_VOID)
        {
            ObjNativeStruct *str = getNativeStruct(str->lib, AS_CSTRING(str->types.values[i]));
            if (str != NULL)
            {
                sz = size_struct(str);
                Value val = from_struct(data, str);
                insertDict(dict, AS_CSTRING(str->names.values[i]), val);
                data += sz;
            }
        }
        else
        {
            sz = size_var(type);
            memcpy(&var.val, data, sz);
            data += sz;

            Value val = from_var(&var, type);
            insertDict(dict, AS_CSTRING(str->names.values[i]), val);
        }
    }

    return OBJ_VAL(dict);
}

ObjNativeStruct *getNativeStruct(ObjNativeLib *lib, const char *name)
{
    for (int i = 0; i < lib->objs.count; i++)
    {
        if (IS_NATIVE_STRUCT(lib->objs.values[i]))
        {
            ObjNativeStruct *str = AS_NATIVE_STRUCT(lib->objs.values[i]);
            if (strcmp(name, str->name->chars) == 0)
                return str;
        }
    }
    return NULL;
}

Value callNative(ObjNativeFunc *func, int argCount, Value *args)
{
    if (!openNativeLib(func->lib))
        return NULL_VAL;

    func_void fn;
#ifdef _WIN32
    fn = (func_void)GetProcAddress(func->lib->handle, func->name->chars);
#else
    fn = (func_void)dlsym(func->lib->handle, func->name->chars);
#endif

    if (fn == NULL)
    {
        runtimeError("Unable to find native func: '%s'", func->name->chars);
        return NULL_VAL;
    }

    if (func->params.count != argCount)
    {
        runtimeError("Invalid number of arguments: required '%d'", func->params.count);
        return NULL_VAL;
    }

    // Arguments ------------------------
    ffi_type **ffi_args = (ffi_type **)malloc(sizeof(ffi_type *) * argCount);
    void **ffi_values = (void **)malloc(sizeof(void *) * argCount);
    var_t *vars = (var_t *)malloc(sizeof(var_t) * argCount);

    for (int i = 0; i < argCount; i++)
    {
        NativeTypes type = getNativeType(AS_CSTRING(func->params.values[i]));
        if (type == TYPE_UNKNOWN || type == TYPE_VOID)
        {
            ObjNativeStruct *str = getNativeStruct(func->lib, AS_CSTRING(func->params.values[i]));
            if (str == NULL)
            {
                free(ffi_args);
                free(ffi_values);
                free(vars);
                runtimeError("Invalid argument type in %s: '%s'", func->name->chars,
                             AS_CSTRING(func->params.values[i]));
                return NULL_VAL;
            }
            else
            {
                vars[i].val._ptr = NULL;
                vars[i].alloc = false;

                if (to_struct(&vars[i], args[i], str, &ffi_args[i]) < 0)
                {
                    free(ffi_args);
                    free(ffi_values);
                    free(vars);
                    return NULL_VAL;
                }

                ffi_values[i] = vars[i].val._ptr;
            }
        }
        else
        {
            ffi_values[i] = &vars[i];
            vars[i].alloc = false;

            to_var(&vars[i], args[i], type, &ffi_args[i]);
        }
    }

    // Return -------------------------
    NativeTypes retType = getNativeType(func->returnType->chars);
    var_t retVal;
    ObjNativeStruct *_struct = NULL;
    void *ret;
    ffi_type *ffi_ret_type = NULL;

    if (retType != TYPE_UNKNOWN)
    {
        ffi_ret_type = prepare_ret_var(&retVal, retType);
        ret = &retVal;
    }
    else
    {
        _struct = getNativeStruct(func->lib, func->returnType->chars);
        if (_struct == NULL)
        {
            for (int i = 0; i < argCount; i++)
            {
                free_var(vars[i]);
            }
            free(ffi_args);
            free(ffi_values);
            free(vars);
            runtimeError("Invalid return type in %s: '%s'", func->name->chars, func->returnType->chars);
            return NULL_VAL;
        }
        else
        {
            ffi_ret_type = prepare_ret_struct(&retVal, _struct);
            if (ffi_ret_type == NULL)
            {
                for (int i = 0; i < argCount; i++)
                {
                    free_var(vars[i]);
                }
                free(ffi_args);
                free(ffi_values);
                free(vars);
                return NULL_VAL;
            }
            ret = retVal.val._ptr;
        }
    }

    // Prepare for call -----------------------
    ffi_cif cif;
    if (ffi_prep_cif(&cif, FFI_DEFAULT_ABI, argCount, ffi_ret_type, ffi_args) != FFI_OK)
    {
        for (int i = 0; i < argCount; i++)
        {
            free_var(vars[i]);
        }
        free(ffi_args);
        free(ffi_values);
        free(vars);
        runtimeError("Could not call '%s'.", func->name->chars);
        return NULL_VAL;
    }

    // Call ------------------------------------
    ffi_call(&cif, FFI_FN(fn), ret, ffi_values);

    // Get the result
    Value result = NULL_VAL;
    if (_struct != NULL)
        result = from_struct(ret, _struct);
    else
        result = from_var(ret, retType);

    // Free data
    for (int i = 0; i < argCount; i++)
    {
        free_var(vars[i]);
    }
    free(ffi_args);
    free(ffi_values);
    free(vars);

    return result;
}

Value createNativeStruct(ObjNativeStruct *str, int argCount, Value *args)
{
    ObjDict *dict = initDict();

    NativeTypes type;
    Value value;

    for (int i = 0; i < str->names.count; i++)
    {
        type = getNativeType(AS_CSTRING(str->types.values[i]));
        value = getDefaultValue(type);
        insertDict(dict, AS_CSTRING(str->names.values[i]), value);
    }

    int count = argCount;
    if (str->names.count < count)
        count = str->names.count;

    for (int i = 0; i < count; i++)
    {
        insertDict(dict, AS_CSTRING(str->names.values[i]), args[i]);
    }

    return OBJ_VAL(dict);
}

Value getDefaultValue(NativeTypes type)
{
    Value result = NULL_VAL;

    switch (type)
    {
        case TYPE_NULL:
        case TYPE_VAR:
        case TYPE_FUNC:
        case TYPE_CBOOL:
            break;
        case TYPE_BOOL:
            result = BOOL_VAL(false);
            break;
        case TYPE_NUMBER:
        case TYPE_INT8:
        case TYPE_INT16:
        case TYPE_INT32:
        case TYPE_INT64:
        case TYPE_UINT8:
        case TYPE_UINT16:
        case TYPE_UINT32:
        case TYPE_UINT64:
        case TYPE_FLOAT32:
        case TYPE_FLOAT64:
            result = NUMBER_VAL(0);
            break;
        case TYPE_STRING:
        case TYPE_CSTRING:
            result = STRING_VAL("");
            break;
        case TYPE_BYTES:
        case TYPE_CBYTES:
            result = BYTES_VAL("", 0);
            break;
        case TYPE_LIST:
            result = OBJ_VAL(initList());
            break;
        case TYPE_DICT:
            result = OBJ_VAL(initDict());
            break;
        default:
            break;
    }

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
    else if (strcmp(name, "cbool") == 0)
        return TYPE_CBOOL;
    else if (strcmp(name, "int8") == 0 || strcmp(name, "char") == 0)
        return TYPE_INT8;
    else if (strcmp(name, "int16") == 0 || strcmp(name, "short") == 0)
        return TYPE_INT16;
    else if (strcmp(name, "int32") == 0 || strcmp(name, "int") == 0)
        return TYPE_INT32;
    else if (strcmp(name, "int64") == 0 || strcmp(name, "long") == 0)
        return TYPE_INT64;
    else if (strcmp(name, "uint8") == 0 || strcmp(name, "uchar") == 0)
        return TYPE_UINT8;
    else if (strcmp(name, "uint16") == 0 || strcmp(name, "ushort") == 0)
        return TYPE_UINT16;
    else if (strcmp(name, "uint32") == 0 || strcmp(name, "uint") == 0)
        return TYPE_UINT32;
    else if (strcmp(name, "uint64") == 0 || strcmp(name, "ulong") == 0)
        return TYPE_UINT64;
    else if (strcmp(name, "float") == 0 || strcmp(name, "float32") == 0)
        return TYPE_FLOAT32;
    else if (strcmp(name, "double") == 0 || strcmp(name, "float64") == 0)
        return TYPE_FLOAT64;
    else if (strcmp(name, "char_array") == 0 || strcmp(name, "cstring") == 0 || strcmp(name, "cstr") == 0)
        return TYPE_CSTRING;
    else if (strcmp(name, "uchar_array") == 0 || strcmp(name, "cbytes") == 0 || strcmp(name, "raw") == 0)
        return TYPE_CBYTES;

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
    if (argCount < 2 || argCount > 4)
    {
        runtimeError("call() takes 2, 3 or 4 arguments (%d given)", argCount);
        return false;
    }

    ObjList *listTypes = NULL;
    int ntypes = 0;
    Value types = NULL_VAL;
    if (argCount == 4)
    {
        types = pop();
        ntypes = 1;
    }

    if (ntypes == 1)
    {
        if (!IS_LIST(types))
        {
            listTypes = initList();
            writeValueArray(&listTypes->values, types);
            types = OBJ_VAL(listTypes);
        }
        listTypes = AS_LIST(types);
        ntypes = listTypes->values.count;
    }

    ObjList *list = NULL;
    int nargs = 0;
    Value args = NULL_VAL;
    if (argCount == 3 || argCount == 4)
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

    if (listTypes != NULL && nargs != ntypes)
    {
        runtimeError("Types and values must be the same size");
        return false;
    }

    ObjString *name = AS_STRING(pop());

    ObjNativeLib *lib = AS_NATIVE_LIB(pop());
    if (!openNativeLib(lib))
    {
        runtimeError("Unable to open native library: '%s'", lib->name->chars);
        return false;
    }

    func_void fn;
#ifdef _WIN32
    fn = (func_void)GetProcAddress(lib->handle, name->chars);
#else
    fn = (func_void)dlsym(lib->handle, name->chars);
#endif

    if (fn == NULL)
    {
        runtimeError("Unable to find native symbol: '%s'", name->chars);
        return false;
    }

    // Arguments ------------------------
    ffi_type **ffi_args = (ffi_type **)malloc(sizeof(ffi_type *) * nargs);
    void **ffi_values = (void **)malloc(sizeof(void *) * nargs);
    var_t *vars = (var_t *)malloc(sizeof(var_t) * nargs);

    for (int i = 0; i < nargs; i++)
    {
        ffi_values[i] = &vars[i];
        vars[i].alloc = false;

        NativeTypes type = TYPE_VAR;
        if (listTypes != NULL)
        {
            if (!IS_STRING(listTypes->values.values[i]))
            {
                for (int i = 0; i < nargs; i++)
                {
                    free_var(vars[i]);
                }
                free(ffi_args);
                free(ffi_values);
                free(vars);
                runtimeError("Type must be a string.");
                return false;
            }
            else
            {
                type = getNativeType(AS_CSTRING(listTypes->values.values[i]));
                if (type == TYPE_UNKNOWN || type == TYPE_VOID)
                {
                    for (int i = 0; i < nargs; i++)
                    {
                        free_var(vars[i]);
                    }
                    free(ffi_args);
                    free(ffi_values);
                    free(vars);
                    runtimeError("Invalid argument type '%s'.", AS_CSTRING(listTypes->values.values[i]));
                    return false;
                }
            }
        }

        to_var(&vars[i], list->values.values[i], type, &ffi_args[i]);
    }

    // Return -------------------------
    var_t ret;
    ffi_type *ffi_ret_type = prepare_ret_var(&ret, TYPE_VOID);

    // Prepare for call -----------------------
    ffi_cif cif;
    if (ffi_prep_cif(&cif, FFI_DEFAULT_ABI, nargs, ffi_ret_type, ffi_args) != FFI_OK)
    {
        for (int i = 0; i < nargs; i++)
        {
            free_var(vars[i]);
        }
        free(ffi_args);
        free(ffi_values);
        free(vars);
        runtimeError("Could not call '%s'.", name->chars);
        return false;
    }

    // Call ------------------------------------
    ffi_call(&cif, FFI_FN(fn), &ret.val, ffi_values);

    // Get the result
    Value result = from_var(&ret, TYPE_VOID);

    // Free data
    for (int i = 0; i < nargs; i++)
    {
        free_var(vars[i]);
    }
    free(ffi_args);
    free(ffi_values);
    free(vars);

    push(result);
    return true;
}

static bool callRetNativeLib(int argCount)
{
    if (argCount < 3 || argCount > 5)
    {
        runtimeError("callRet() takes 3, 4 or 5 arguments (%d given)", argCount);
        return false;
    }

    ObjList *listTypes = NULL;
    int ntypes = 0;
    Value types = NULL_VAL;
    if (argCount == 5)
    {
        types = pop();
        ntypes = 1;
    }

    if (ntypes == 1)
    {
        if (!IS_LIST(types))
        {
            listTypes = initList();
            writeValueArray(&listTypes->values, types);
            types = OBJ_VAL(listTypes);
        }
        listTypes = AS_LIST(types);
        ntypes = listTypes->values.count;
    }

    ObjList *list = NULL;
    int nargs = 0;
    Value args = NULL_VAL;
    if (argCount == 4 || argCount == 5)
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

    if (listTypes != NULL && nargs != ntypes)
    {
        runtimeError("Types and values must be the same size");
        return false;
    }

    ObjString *returnType = AS_STRING(pop());

    ObjString *name = AS_STRING(pop());

    ObjNativeLib *lib = AS_NATIVE_LIB(pop());
    if (!openNativeLib(lib))
    {
        runtimeError("Unable to open native library: '%s'", lib->name->chars);
        return false;
    }

    func_void fn;
#ifdef _WIN32
    fn = (func_void)GetProcAddress(lib->handle, name->chars);
#else
    fn = (func_void)dlsym(lib->handle, name->chars);
#endif

    if (fn == NULL)
    {
        runtimeError("Unable to find native symbol: '%s'", name->chars);
        return false;
    }

    // Arguments ------------------------
    ffi_type **ffi_args = (ffi_type **)malloc(sizeof(ffi_type *) * nargs);
    void **ffi_values = (void **)malloc(sizeof(void *) * nargs);
    var_t *vars = (var_t *)malloc(sizeof(var_t) * nargs);

    for (int i = 0; i < nargs; i++)
    {
        ffi_values[i] = &vars[i];
        vars[i].alloc = false;

        NativeTypes type = TYPE_VAR;
        if (listTypes != NULL)
        {
            if (!IS_STRING(listTypes->values.values[i]))
            {
                for (int i = 0; i < nargs; i++)
                {
                    free_var(vars[i]);
                }
                free(ffi_args);
                free(ffi_values);
                free(vars);
                runtimeError("Type must be a string.");
                return false;
            }
            else
            {
                type = getNativeType(AS_CSTRING(listTypes->values.values[i]));
                if (type == TYPE_UNKNOWN || type == TYPE_VOID)
                {
                    for (int i = 0; i < nargs; i++)
                    {
                        free_var(vars[i]);
                    }
                    free(ffi_args);
                    free(ffi_values);
                    free(vars);
                    runtimeError("Invalid argument type '%s'.", AS_CSTRING(listTypes->values.values[i]));
                    return false;
                }
            }
        }

        to_var(&vars[i], list->values.values[i], type, &ffi_args[i]);
    }

    // Return -------------------------
    NativeTypes retType = getNativeType(returnType->chars);
    if (retType == TYPE_UNKNOWN)
        retType = TYPE_VOID;

    var_t ret;
    ffi_type *ffi_ret_type = prepare_ret_var(&ret, retType);

    // Prepare for call -----------------------
    ffi_cif cif;
    if (ffi_prep_cif(&cif, FFI_DEFAULT_ABI, nargs, ffi_ret_type, ffi_args) != FFI_OK)
    {
        for (int i = 0; i < nargs; i++)
        {
            free_var(vars[i]);
        }
        free(ffi_args);
        free(ffi_values);
        free(vars);
        runtimeError("Could not call '%s'.", name->chars);
        return false;
    }

    // Call ------------------------------------
    ffi_call(&cif, FFI_FN(fn), &ret.val, ffi_values);

    // Get the result
    Value result = from_var(&ret, retType);

    // Free data
    for (int i = 0; i < nargs; i++)
    {
        free_var(vars[i]);
    }
    free(ffi_args);
    free(ffi_values);
    free(vars);

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