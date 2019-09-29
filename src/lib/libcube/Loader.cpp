#include "Loader.h"
#include <dlfcn.h>

using namespace std;

typedef int (*int_void)();

typedef int (*int_char)(char);
typedef int (*int_int)(int);
typedef int (*int_float)(float);
typedef int (*int_double)(float);
typedef int (*int_string)(const char*);

typedef int (*int_char_int)(char,int);
typedef int (*int_int_int)(int,int);
typedef int (*int_float_int)(float,int);
typedef int (*int_double_int)(float,int);
typedef int (*int_string_int)(const char*,int);

Loader::Loader()
{

}

Loader::~Loader()
{

}

bool Loader::Load(const string& fileName, void **handler)
{
    void* h = dlopen(fileName.c_str(), RTLD_LAZY);
    if(!h)
        return false;
    *handler = h;
    return true;
}

Var Loader::exec(const std::string& name, std::vector<Var*>& args, Var* funcDef, Var* caller)
{
    Var res;
    void* handler = caller->Handler();
    if(!handler)
    {
        res.Error("Invalid native library");
        return res;
    }

    void* fn = dlsym(handler, name.c_str());
    if(!fn)
    {
        res.Error("Invalid native library call [" + name + "]");
        return res;
    }

    string retType = funcDef->ReturnType();
    NamesArray& names = funcDef->Names();

    if(retType == "none")
    {
        callVoid(names, args, fn);
    }
    else if(retType == "bool")
    {
        bool rt = callBool(names, args, fn);
        res = rt;
    }
    else if(retType == "int")
    {
        double rt = callInt(names, args, fn);
        res = rt;
    }
    else if(retType == "char")
    {
        char rt = callChar(names, args, fn);
        string str = "";
        str += rt;
        res = str;
    }
    else if(retType == "float")
    {
        double rt = callFloat(names, args, fn);
        res = rt;
    }
    else if(retType == "double")
    {
        double rt = callDouble(names, args, fn);
        res = rt;
    }
    else if(retType == "string")
    {
        string rt = callString(names, args, fn);
        res = rt;
    }
    else
    {
        res.Error("Invalid return type '" + retType + "'");
    }

    return res;
}


void Loader::callVoid(NamesArray& names, std::vector<Var*>& args, void *handler)
{

}

bool Loader::callBool(NamesArray& names, std::vector<Var*>& args, void *handler)
{
    return false;
}

int Loader::callInt(NamesArray& names, std::vector<Var*>& args, void *handler)
{
    int rt = 0;
    if(names.size() == 0)
    {
        int_void fn = (int_void)handler;
        rt = fn();
    }
    else if(names.size() == 1)
    {
        if(names[0] == "char")
        {
            int_char fn = (int_char)handler;
            rt = fn(args[0]->AsString().String()[0]);
        }
        else if(names[0] == "int")
        {
            int_int fn = (int_int)handler;
            rt = fn((int)args[0]->AsNumber().Number());
        }
        else if(names[0] == "float")
        {
            int_float fn = (int_float)handler;
            rt = fn((float)args[0]->AsNumber().Number());
        }
        else if(names[0] == "double")
        {
            int_double fn = (int_double)handler;
            rt = fn((double)args[0]->AsNumber().Number());
        }
        else if(names[0] == "string")
        {
            int_string fn = (int_string)handler;
            rt = fn(args[0]->AsString().String().c_str());
        }
    }
    else if(names.size() == 2)
    {
        if(names[0] == "char")
        {
            
        }
        else if(names[0] == "int")
        {
            if(names[1] == "char")
            {
                
            }
            else if(names[1] == "int")
            {
                int_int_int fn = (int_int_int)handler;
                rt = fn((int)args[0]->AsNumber().Number(), (int)args[1]->AsNumber().Number());
            }
            else if(names[1] == "float")
            {
                
            }
            else if(names[1] == "double")
            {
                
            }
            else if(names[1] == "string")
            {
                
            }
        }
        else if(names[0] == "float")
        {
            
        }
        else if(names[0] == "double")
        {
            
        }
        else if(names[0] == "string")
        {
            
        }
    }
    
    return rt;
}

char Loader::callChar(NamesArray& names, std::vector<Var*>& args, void *handler)
{
    return '0';
}

float Loader::callFloat(NamesArray& names, std::vector<Var*>& args, void *handler)
{
    return 0;
}

double Loader::callDouble(NamesArray& names, std::vector<Var*>& args, void *handler)
{
    return 0;
}

std::string Loader::callString(NamesArray& names, std::vector<Var*>& args, void *handler)
{
    return "0";
}