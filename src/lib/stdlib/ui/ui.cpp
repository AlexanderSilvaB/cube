#include <cube/cubeext.h>
#include "WM.hpp"
#include <iostream>
#include <string.h>

using namespace std;

static WM *wm;

extern "C"
{
    int argc = 0;
    char *argv[0];
    QApplication app(argc, argv);

    char *copyStr(const char* str)
    {
        char *s = (char*)malloc(sizeof(char) * (strlen(str) + 1));
        strcpy(s, str);
        return s;
    }

    EXPORTED void ui_start()
    {
        wm = new WM(&app);
        wm->Init();
    }

    EXPORTED void ui_stop()
    {   
        wm->Destroy();
        delete wm;
    }

    EXPORTED cube_native_var* ui_create_window(cube_native_var* title, 
        cube_native_var* width, 
        cube_native_var* height)
    {   
        int id = wm->CreateWindow(AS_NATIVE_STRING(title), AS_NATIVE_NUMBER(width), AS_NATIVE_NUMBER(height));

        cube_native_var* ret = NATIVE_NUMBER(id);
        return ret;
    }

    EXPORTED void ui_destroy_window(cube_native_var* id)
    {   
        wm->DestroyWindow(AS_NATIVE_NUMBER(id));
    }

    EXPORTED cube_native_var* ui_run_cycle()
    {   
        vector<string> events = wm->Cycle();

        cube_native_var* list = NATIVE_LIST();
        for(int i = 0; i < events.size(); i++)
        {
            char *str = (char*)malloc(sizeof(char) * (events[i].length() + 1));
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

    EXPORTED cube_native_var* ui_has_quit()
    {   
        cube_native_var* ret = NATIVE_BOOL( wm->HasQuit() );
        return ret;
    }

    EXPORTED cube_native_var* ui_load(cube_native_var* id, cube_native_var* fileName)
    {   
        bool success = wm->Load(AS_NATIVE_NUMBER(id), AS_NATIVE_STRING(fileName));
        cube_native_var* ret = NATIVE_BOOL(success);
        
        return ret;
    }

    EXPORTED cube_native_var* ui_on_event(cube_native_var* id, cube_native_var* objName, cube_native_var* eventName)
    {   
        bool success = wm->RegisterEvent(AS_NATIVE_NUMBER(id), AS_NATIVE_STRING(objName), AS_NATIVE_STRING(eventName));

        cube_native_var* ret = NATIVE_BOOL(success);
        return ret;
    }

    EXPORTED cube_native_var* ui_get_event_args(cube_native_var *name)
    {   
        Dict args = wm->GetEventArgs(AS_NATIVE_STRING(name));
        cube_native_var* ret = NATIVE_DICT();
        for(Dict::iterator it = args.begin(); it != args.end(); it++)
        {
            cube_native_var *next = NATIVE_STRING_COPY(it->second.c_str());
            ADD_NATIVE_DICT(ret, copyStr(it->first.c_str()), next);
        }

        return ret;
    }

    EXPORTED cube_native_var* ui_get_property(cube_native_var* id, cube_native_var* objName, cube_native_var* propName)
    {   
        List list = wm->GetProperty(AS_NATIVE_NUMBER(id), AS_NATIVE_STRING(objName), AS_NATIVE_STRING(propName));

        cube_native_var* ret;

        if(!list.empty())
        {
            if(list.size() > 1)
            {
                ret = NATIVE_LIST();
                for(int i = 0; i < list.size(); i++)
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
            ret = NATIVE_NONE();
        }
        
        return ret;
    }

    EXPORTED cube_native_var* ui_set_property(cube_native_var* id, cube_native_var* objName, cube_native_var* propName, cube_native_var* value)
    {   
        bool success = wm->SetProperty(AS_NATIVE_NUMBER(id), AS_NATIVE_STRING(objName), AS_NATIVE_STRING(propName), AS_NATIVE_STRING(value));

        cube_native_var* ret = NATIVE_BOOL(success);
        return ret;
    }

    EXPORTED cube_native_var* ui_get_obj(cube_native_var* id, cube_native_var* objName)
    {   
        Dict props = wm->GetProperties(AS_NATIVE_NUMBER(id), AS_NATIVE_STRING(objName));
        cube_native_var* ret;
        if(props.size() == 0)
        {
            ret = NATIVE_NONE();
        }
        else
        {
            ret = NATIVE_DICT();
            for(Dict::iterator it = props.begin(); it != props.end(); it++)
            {
                cube_native_var *next = NATIVE_STRING_COPY(it->second.c_str());
                ADD_NATIVE_DICT(ret, copyStr(it->first.c_str()), next);
            }
        }

        return ret;
    }

    
}