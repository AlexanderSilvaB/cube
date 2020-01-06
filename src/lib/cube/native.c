#include <dlfcn.h>
#include <string.h>

#include "native.h"
#include "vm.h"
#include "util.h"
#include "cubeext.h"

typedef void (*func_void)();
typedef cube_native_var* (*func_value)();

typedef union {
    func_void f_void;
    func_value f_value;
} lib_func;

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
        //lib->handle = dlopen(path, RTLD_NOW | RTLD_GLOBAL);
        lib->handle = dlopen(path, RTLD_LAZY);
        if (lib->handle == NULL)
        {
            runtimeError("Unable to open native lib: '%s'\nError: %s", lib->name->chars, dlerror());
            return false;
        }
    }
    return true;
}

void closeNativeLib(ObjNativeLib *lib)
{
    if (lib->handle != NULL)
    {
        dlclose(lib->handle);
    }
    lib->handle = NULL;
}

Value nativeToValue(cube_native_var *var, NativeTypes *nt)
{
    Value result = NONE_VAL;
    if(IS_NATIVE_LIST(var))
    {
        ObjList *list = initList();
        
        while(HAS_NATIVE_NEXT(var))
        {
            var = NATIVE_NEXT(var);
            result = nativeToValue(var, nt);
            writeValueArray(&list->values, result);
        }
        *nt = TYPE_LIST;
        result = OBJ_VAL(list);
    }
    else if(IS_NATIVE_DICT(var))
    {
        ObjDict *dict = initDict();
        char *key;
        
        while(HAS_NATIVE_NEXT(var))
        {
            key = var->key;
            var = NATIVE_NEXT(var);
            result = nativeToValue(var, nt);
            insertDict(dict, key, result);
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
            case TYPE_NONE:
            {
                result = NONE_VAL;
            }
            break;
            case TYPE_BOOL:
            {
                result = BOOL_VAL(AS_NATIVE_BOOL(var));
            }
            break;
            case TYPE_NUMBER:
            {
                result = NUMBER_VAL(AS_NATIVE_NUMBER(var));
            }
            break;
            case TYPE_STRING:
            {
                result = STRING_VAL(AS_NATIVE_STRING(var));
                
            }
            break;
            case TYPE_BYTES:
            {
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
    if(IS_NONE(value))
    {
        var->type = TYPE_NONE;
    }
    else if(IS_BOOL(value))
    {
        var->type = TYPE_BOOL;
        var->value._bool = AS_BOOL(value);
    }
    else if(IS_NUMBER(value))
    {
        var->type = TYPE_NUMBER;
        var->value._number = AS_NUMBER(value);
    }
    else if(IS_STRING(value))
    {
        var->type = TYPE_STRING;
        var->value._string = AS_CSTRING(value);
    }
    else if(IS_BYTES(value))
    {
        ObjBytes *bytes = AS_BYTES(value);
        var->type = TYPE_BYTES;
        var->value._bytes.length = bytes->length;
        var->value._bytes.bytes = bytes->bytes;
    }
    else if(IS_LIST(value))
    {
        ObjList *list = AS_LIST(value);
        var->type = TYPE_VOID;
        var->is_list = true;
        for(int i = 0; i < list->values.count; i++)
        {
            cube_native_var *next = NATIVE_VAR();
            valueToNative(next, list->values.values[i]);
            ADD_NATIVE_LIST(var, next);
        }
    }
    else if(IS_DICT(value))
    {
        ObjDict *dict = AS_DICT(value);
        var->type = TYPE_VOID;
        var->is_dict = true;

        for (int i = 0; i < dict->capacity; i++)
        {
            if (!dict->items[i])
                continue;

            cube_native_var *next = NATIVE_VAR();
            valueToNative(next, dict->items[i]->item);
            ADD_NATIVE_DICT(var, dict->items[i]->key, next);
        }
    }
    else
    {
        var->type = TYPE_BOOL;
        var->value._bool = false;
    }
}

void freeNativeVar(cube_native_var *var, bool skipFirst, bool skipInterns)
{
    if(var == NULL)
        return;

    if(!skipInterns)
    {
        if(var->type == TYPE_STRING)
        {
            free(var->value._string);
        }
        else if(var->type == TYPE_BYTES)
        {
            free(var->value._bytes.bytes);
        }
    }
    if(var->next != NULL)
    {
        freeNativeVar(var->next, false, skipInterns);
        var->next = NULL;
    }
    if(!skipFirst)
        free(var);
}

void freeVarNative(cube_native_var *var)
{
    freeNativeVar(var, false, false);
}

Value callNativeFn(lib_func func, NativeTypes ret, int count, cube_native_var **values)
{
    Value result = NONE_VAL;
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
            func.f_void(values[0], values[1], values[2], values[3], values[4], values[5], values[6], values[7], values[8]);
            break;
        case 10:
            func.f_void(values[0], values[1], values[2], values[3], values[4], values[5], values[6], values[7], values[8], values[9]);
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
            res = func.f_value(values[0], values[1], values[2], values[3], values[4], values[5], values[6], values[7]);
            break;
        case 9:
            res = func.f_value(values[0], values[1], values[2], values[3], values[4], values[5], values[6], values[7], values[8]);
            break;
        case 10:
            res = func.f_value(values[0], values[1], values[2], values[3], values[4], values[5], values[6], values[7], values[8], values[9]);
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
        //     return NONE_VAL;
        // }

        freeNativeVar(res, false, false);
    }
    return result;
}



Value callNative(ObjNativeFunc *func, int argCount, Value *args)
{
    if (!openNativeLib(func->lib))
        return NONE_VAL;

    lib_func fn;
    fn.f_void = (func_void)dlsym(func->lib->handle, func->name->chars);

    if (fn.f_void == NULL)
    {
        runtimeError("Unable to find native func: '%s'", func->name->chars);
        return NONE_VAL;
    }

    if (func->params.count != argCount)
    {
        runtimeError("Invalid number of arguments: required '%d'", func->params.count);
        return NONE_VAL;
    }

    Value result = NONE_VAL;
    cube_native_var **values = (cube_native_var **)malloc(sizeof(cube_native_var*) * 10);
    for (int i = 0; i < 10; i++)
        values[i] = NULL;
    for (int i = 0; i < func->params.count && i < 10; i++)
    {
        NativeTypes type = getNativeType(AS_CSTRING(func->params.values[i]));
        if (type == TYPE_UNKNOWN || type == TYPE_VOID)
        {
            runtimeError("Invalid argument type in %s: '%s'", func->name->chars, AS_CSTRING(func->params.values[i]));
            return NONE_VAL;
        }
        values[i] = NATIVE_VAR();
        switch (type)
        {
        case TYPE_NONE:
            values[i]->type = TYPE_NONE;
            break;
        case TYPE_BOOL:
            values[i]->type = TYPE_BOOL;
            values[i]->value._bool = AS_BOOL(toBool(args[i]));
            break;
        case TYPE_NUMBER:
            values[i]->type = TYPE_NUMBER;
            values[i]->value._number = (double)AS_NUMBER(toNumber(args[i]));
            break;
        case TYPE_STRING:
            values[i]->type = TYPE_STRING;
            values[i]->value._string = AS_CSTRING(toString(args[i]));
            break;
        case TYPE_BYTES:
        {
            ObjBytes *bytes = AS_BYTES(toBytes(args[i]));
            values[i]->type = TYPE_BYTES;
            values[i]->value._bytes.length = bytes->length;
            values[i]->value._bytes.bytes = bytes->bytes;
        }
        break;
        case TYPE_LIST:
        case TYPE_DICT:
        {
            valueToNative(values[i], args[i]);
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
        return NONE_VAL;
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
    else if (strcmp(name, "none") == 0)
        return TYPE_NONE;
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

    return TYPE_UNKNOWN;
}