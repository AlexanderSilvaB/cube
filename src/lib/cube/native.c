#include <dlfcn.h>
#include <string.h>

#include "native.h"
#include "vm.h"
#include "util.h"
#include "cubeext.h"

typedef void (*func_void)();
typedef cube_native_value (*func_value)();

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
        lib->handle = dlopen(path, RTLD_NOW | RTLD_GLOBAL);
        if (lib->handle == NULL)
        {
            runtimeError("Unable to open native lib: '%s'", lib->name->chars);
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

Value callNativeFn(lib_func func, NativeTypes ret, int count, cube_native_value *values)
{
    Value result = NONE_VAL;
    cube_native_value res;
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

        switch (ret)
        {
        case TYPE_VOID:
        {
            result = NONE_VAL;
        }
        break;
        case TYPE_BOOL:
        {
            result = BOOL_VAL(res._bool);
        }
        break;
        case TYPE_NUMBER:
        {
            result = NUMBER_VAL(res._number);
        }
        break;
        case TYPE_STRING:
        {
            result = STRING_VAL(res._string);
            free(res._string);
        }
        break;
        case TYPE_BYTES:
        {
            result = BYTES_VAL(res._bytes.bytes, res._bytes.length);
            free(res._bytes.bytes);
        }
        break;
        default:
            break;
        }
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
    cube_native_value values[10];
    for (int i = 0; i < func->params.count && i < 10; i++)
    {
        NativeTypes type = getNativeType(AS_CSTRING(func->params.values[i]));
        if (type == TYPE_UNKNOWN || type == TYPE_VOID)
        {
            runtimeError("Invalid argument type in %s: '%s'", func->name->chars, AS_CSTRING(func->params.values[i]));
            return NONE_VAL;
        }
        switch (type)
        {
        case TYPE_BOOL:
            values[i]._bool = AS_BOOL(toBool(args[i]));
            break;
        case TYPE_NUMBER:
            values[i]._number = (double)AS_NUMBER(toNumber(args[i]));
            break;
        case TYPE_STRING:
            values[i]._string = AS_CSTRING(toString(args[i]));
            break;
        case TYPE_BYTES:
        {
            ObjBytes *bytes = AS_BYTES(toBytes(args[i]));
            values[i]._bytes.length = bytes->length;
            values[i]._bytes.bytes = bytes->bytes;
        }
        break;
        default:
            values[i]._bool = false;
            break;
        }
    }

    NativeTypes retType = getNativeType(func->returnType->chars);
    if (retType == TYPE_UNKNOWN)
    {
        runtimeError("Invalid return type in %s: '%s'", func->name->chars, func->returnType->chars);
        return NONE_VAL;
    }
    result = callNativeFn(fn, retType, argCount, values);

    return result;
}

NativeTypes getNativeType(const char *name)
{
    if (strcmp(name, "void") == 0)
        return TYPE_VOID;
    else if (strcmp(name, "bool") == 0)
        return TYPE_BOOL;
    else if (strcmp(name, "num") == 0 || strcmp(name, "number") == 0)
        return TYPE_NUMBER;
    else if (strcmp(name, "str") == 0 || strcmp(name, "string") == 0)
        return TYPE_STRING;
    else if (strcmp(name, "bytes") == 0)
        return TYPE_BYTES;

    return TYPE_UNKNOWN;
}