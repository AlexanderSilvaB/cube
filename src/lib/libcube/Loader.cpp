#include "Loader.h"
#include <dlfcn.h>

using namespace std;

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

    int (*func)(const char*);
    func = (int (*)(const char*))fn;

    double rt = func(args[0]->AsString().String().c_str());

    
    res = rt;
    return res;
}