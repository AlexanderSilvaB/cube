#include <cube/cubeext.h>
#include <iostream>

#include "Storage.h"

using namespace std;

void ValueToNative(cube_native_var *var, Value value)
{
    switch (value.Type())
    {
        case Value::OBJECT: {
            TO_NATIVE_DICT(var);
            map<string, Value> values = value.Object();

            for (map<string, Value>::iterator it = values.begin(); it != values.end(); it++)
            {
                cube_native_var *next = NATIVE_VAR();
                ValueToNative(next, it->second);
                ADD_NATIVE_DICT(var, COPY_STR(it->first.c_str()), next);
            }
            break;
        }
        case Value::ARRAY: {
            TO_NATIVE_LIST(var);
            vector<Value> values = value.Array();

            for (vector<Value>::iterator it = values.begin(); it != values.end(); it++)
            {
                cube_native_var *next = NATIVE_VAR();
                ValueToNative(next, *it);
                ADD_NATIVE_LIST(var, next);
            }
            break;
        }
        case Value::BOOL: {
            TO_NATIVE_BOOL(var, value.Bool());
            break;
        }
        case Value::INT: {
            TO_NATIVE_NUMBER(var, value.Int());
            break;
        }
        case Value::FLOAT: {
            TO_NATIVE_NUMBER(var, value.Float());
            break;
        }
        case Value::STRING: {
            TO_NATIVE_STRING(var, COPY_STR(value.String().c_str()));
            break;
        }
        default: {
            break;
        }
    }
}

Value NativeToValue(cube_native_var *var)
{
    Value ret;
    if (IS_NATIVE_LIST(var))
    {
        vector<Value> values;

        for (int i = 0; i < var->size; i++)
        {
            Value value = NativeToValue(var->list[i]);
            values.push_back(value);
        }

        ret = Value(values);
    }
    else if (IS_NATIVE_DICT(var))
    {
        map<string, Value> values;

        for (int i = 0; i < var->size; i++)
        {
            Value value = NativeToValue(var->dict[i]);
            values[string(var->dict[i]->key)] = value;
        }
        ret = Value(values);
    }
    else
    {
        switch (NATIVE_TYPE(var))
        {
            case TYPE_VOID:
            case TYPE_NULL: {
            }
            break;
            case TYPE_BOOL: {
                ret = Value(AS_NATIVE_BOOL(var));
            }
            break;
            case TYPE_NUMBER: {
                ret = Value((float)AS_NATIVE_NUMBER(var));
            }
            break;
            case TYPE_STRING: {
                ret = Value(string(AS_NATIVE_STRING(var)));
            }
            break;
            default:
                break;
        }
    }

    return ret;
}

extern "C"
{
    EXPORTED cube_native_var *json_native_parse(cube_native_var *str)
    {
        cube_native_var *ret = NATIVE_NULL();

        Value value;
        if (value.Parse(string(AS_NATIVE_STRING(str))))
        {
            ValueToNative(ret, value);
        }

        return ret;
    }

    EXPORTED cube_native_var *json_native_read(cube_native_var *fileName)
    {
        cube_native_var *ret = NATIVE_NULL();

        Storage storage(string(AS_NATIVE_STRING(fileName)));
        if (storage.IsOpened())
        {
            ValueToNative(ret, storage.Data());
        }

        return ret;
    }

    EXPORTED cube_native_var *json_native_write(cube_native_var *fileName, cube_native_var *data)
    {
        Value value = NativeToValue(data);

        cube_native_var *ret = NATIVE_BOOL(false);

        Storage storage(string(AS_NATIVE_STRING(fileName)));
        if (storage.IsOpened())
        {
            storage.Data() = value;
            storage.Save();
            TO_NATIVE_BOOL(ret, true);
        }

        return ret;
    }
}