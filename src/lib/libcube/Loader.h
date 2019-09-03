#ifndef _LOADER_H_
#define _LOADER_H_

#include <string>
#include <memory>
#include "Var.h"

class Loader;
typedef std::shared_ptr<Loader> LoaderPtr;

class Loader
{
    public:
        Loader();
        ~Loader();

        bool Load(const std::string& fileName, void **handler);  
};

#endif