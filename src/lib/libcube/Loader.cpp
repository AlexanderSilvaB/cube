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