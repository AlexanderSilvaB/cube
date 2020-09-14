#include "WM.hpp"
#include <cube/cubeext.h>
#include <iostream>
#include <sstream>
#include <string.h>

using namespace std;

static WM *wm = NULL;

string NativeToValue(cube_native_var *var)
{
    switch (NATIVE_TYPE(var))
    {
        case TYPE_VOID:
        case TYPE_NULL: {
            return "";
        }
        break;
        case TYPE_BOOL: {
            return AS_NATIVE_BOOL(var) ? "true" : "false";
        }
        break;
        case TYPE_NUMBER: {
            stringstream ss;
            ss << AS_NATIVE_NUMBER(var);
            return ss.str();
        }
        break;
        case TYPE_STRING: {
            return string(AS_NATIVE_STRING(var));
        }
        break;
        default:
            break;
    }

    return "";
}

extern "C"
{
    int argc = 0;
    char *argv[1];
    QApplication app(argc, argv);

    char *copyStr(const char *str)
    {
        char *s = (char *)malloc(sizeof(char) * (strlen(str) + 1));
        strcpy(s, str);
        return s;
    }

    EXPORTED void ui_start()
    {
        if (wm != NULL)
            return;
        wm = new WM(&app);
        wm->Init();
    }

    EXPORTED void ui_stop()
    {
        if (wm == NULL)
            return;
        wm->Destroy();
        delete wm;
        wm = NULL;
    }

    EXPORTED cube_native_var *ui_create_window(cube_native_var *title, cube_native_var *width, cube_native_var *height)
    {
        int id = wm->NewWindow(AS_NATIVE_STRING(title), AS_NATIVE_NUMBER(width), AS_NATIVE_NUMBER(height));

        cube_native_var *ret = NATIVE_NUMBER(id);
        return ret;
    }

    EXPORTED void ui_destroy_window(cube_native_var *id)
    {
        wm->DestroyWindow(AS_NATIVE_NUMBER(id));
    }

    EXPORTED cube_native_var *ui_run_cycle()
    {
        vector<string> events = wm->Cycle();

        cube_native_var *list = NATIVE_LIST();
        for (int i = 0; i < events.size(); i++)
        {
            char *str = (char *)malloc(sizeof(char) * (events[i].length() + 1));
            strcpy(str, events[i].c_str());
            cube_native_var *ev = NATIVE_STRING(str);
            ADD_NATIVE_LIST(list, ev);
        }

        return list;
    }

    EXPORTED void ui_run_forever()
    {
        wm->Exec();
    }

    EXPORTED cube_native_var *ui_has_quit()
    {
        cube_native_var *ret = NATIVE_BOOL(wm->HasQuit());
        return ret;
    }

    EXPORTED cube_native_var *ui_load(cube_native_var *id, cube_native_var *fileName)
    {
        bool success = wm->Load(AS_NATIVE_NUMBER(id), AS_NATIVE_STRING(fileName));
        cube_native_var *ret = NATIVE_BOOL(success);

        return ret;
    }

    EXPORTED void ui_resize_window(cube_native_var *id, cube_native_var *width, cube_native_var *height)
    {
        wm->Resize(AS_NATIVE_NUMBER(id), AS_NATIVE_NUMBER(width), AS_NATIVE_NUMBER(height));
    }

    EXPORTED cube_native_var *ui_on_event(cube_native_var *id, cube_native_var *objName, cube_native_var *eventName)
    {
        bool success = wm->RegisterEvent(AS_NATIVE_NUMBER(id), AS_NATIVE_STRING(objName), AS_NATIVE_STRING(eventName));

        cube_native_var *ret = NATIVE_BOOL(success);
        return ret;
    }

    EXPORTED cube_native_var *ui_get_event_args(cube_native_var *name)
    {
        Dict args = wm->GetEventArgs(AS_NATIVE_STRING(name));
        cube_native_var *ret = NATIVE_DICT();
        for (Dict::iterator it = args.begin(); it != args.end(); it++)
        {
            cube_native_var *next = NATIVE_STRING_COPY(it->second.c_str());
            ADD_NATIVE_DICT(ret, copyStr(it->first.c_str()), next);
        }

        return ret;
    }

    EXPORTED cube_native_var *ui_get_property(cube_native_var *id, cube_native_var *objName, cube_native_var *propName)
    {
        List list = wm->GetProperty(AS_NATIVE_NUMBER(id), AS_NATIVE_STRING(objName), AS_NATIVE_STRING(propName));

        cube_native_var *ret;

        if (!list.empty())
        {
            if (list.size() > 1)
            {
                ret = NATIVE_LIST();
                for (int i = 0; i < list.size(); i++)
                {
                    cube_native_var *prop = NATIVE_STRING_COPY(list[i].c_str());
                    ADD_NATIVE_LIST(ret, prop);
                }
            }
            else
            {
                ret = NATIVE_STRING_COPY(list[0].c_str());
            }
        }
        else
        {
            ret = NATIVE_NULL();
        }

        return ret;
    }

    EXPORTED cube_native_var *ui_set_property(cube_native_var *id, cube_native_var *objName, cube_native_var *propName,
                                              cube_native_var *value)
    {
        bool success = wm->SetProperty(AS_NATIVE_NUMBER(id), AS_NATIVE_STRING(objName), AS_NATIVE_STRING(propName),
                                       AS_NATIVE_STRING(value));

        cube_native_var *ret = NATIVE_BOOL(success);
        return ret;
    }

    EXPORTED cube_native_var *ui_get_obj(cube_native_var *id, cube_native_var *objName)
    {
        Dict props = wm->GetProperties(AS_NATIVE_NUMBER(id), AS_NATIVE_STRING(objName));
        cube_native_var *ret;
        if (props.size() == 0)
        {
            ret = NATIVE_NULL();
        }
        else
        {
            ret = NATIVE_DICT();
            for (Dict::iterator it = props.begin(); it != props.end(); it++)
            {
                cube_native_var *next = NATIVE_STRING_COPY(it->second.c_str());
                ADD_NATIVE_DICT(ret, copyStr(it->first.c_str()), next);
            }
        }

        return ret;
    }

    EXPORTED cube_native_var *ui_get_methods(cube_native_var *id, cube_native_var *objName)
    {
        List methods = wm->GetMethods(AS_NATIVE_NUMBER(id), AS_NATIVE_STRING(objName));
        cube_native_var *ret;
        if (methods.size() == 0)
        {
            ret = NATIVE_NULL();
        }
        else
        {
            ret = NATIVE_LIST();
            for (int i = 0; i < methods.size(); i++)
            {
                cube_native_var *method = NATIVE_STRING_COPY(methods[i].c_str());
                ADD_NATIVE_LIST(ret, method);
            }
        }

        return ret;
    }

    EXPORTED cube_native_var *ui_get_method(cube_native_var *id, cube_native_var *objName, cube_native_var *methodName)
    {
        string name = wm->GetMethod(AS_NATIVE_NUMBER(id), AS_NATIVE_STRING(objName), AS_NATIVE_STRING(methodName));

        cube_native_var *ret;
        if (name.empty())
            ret = NATIVE_NULL();
        else
        {
            ret = NATIVE_STRING_COPY(name.c_str());
        }
        return ret;
    }

    EXPORTED cube_native_var *ui_call_method(cube_native_var *id, cube_native_var *objName, cube_native_var *methodName,
                                             cube_native_var *arguments)
    {
        List args;
        if (arguments->is_list)
        {
            for (int i = 0; i < arguments->size; i++)
            {
                args.push_back(string(AS_NATIVE_STRING(arguments->list[i])));
            }
        }
        List list;
        bool success =
            wm->CallMethod(AS_NATIVE_NUMBER(id), AS_NATIVE_STRING(objName), AS_NATIVE_STRING(methodName), args, list);

        cube_native_var *ret;

        if (!success)
        {
            ret = NATIVE_NULL();
        }
        else
        {
            if (list.size() > 1)
            {
                ret = NATIVE_LIST();
                for (int i = 0; i < list.size(); i++)
                {
                    cube_native_var *prop = NATIVE_STRING_COPY(list[i].c_str());
                    ADD_NATIVE_LIST(ret, prop);
                }
            }
            else
            {
                ret = NATIVE_STRING_COPY(list[0].c_str());
            }
        }

        return ret;
    }

    // Shapes
    EXPORTED void ui_antialias(cube_native_var *id, cube_native_var *enabled)
    {
        wm->SetAntialias(AS_NATIVE_NUMBER(id), AS_NATIVE_BOOL(enabled));
    }

    EXPORTED void ui_rm_shape(cube_native_var *id, cube_native_var *shape_id)
    {
        wm->RmShape(AS_NATIVE_NUMBER(id), AS_NATIVE_NUMBER(shape_id));
    }

    EXPORTED cube_native_var *ui_add_shape(cube_native_var *id, cube_native_var *shape)
    {
        map<string, string> values;
        for (int i = 0; i < shape->size; i++)
        {
            values[string(shape->dict[i]->key)] = NativeToValue(shape->dict[i]);
        }
        int shape_id = wm->AddShape(AS_NATIVE_NUMBER(id), values);
        return NATIVE_NUMBER(shape_id);
    }

    EXPORTED void ui_update_shape(cube_native_var *id, cube_native_var *shape)
    {
        map<string, string> values;
        for (int i = 0; i < shape->size; i++)
        {
            values[string(shape->dict[i]->key)] = NativeToValue(shape->dict[i]);
        }
        wm->UpdateShape(AS_NATIVE_NUMBER(id), values);
    }
}