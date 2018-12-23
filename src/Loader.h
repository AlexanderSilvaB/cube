#ifndef _LOADER_H_
#define _LOADER_H_

#include <string>
#include "Var.h"

class Loader
{
    public:
        Loader();
        ~Loader();

        bool Load(const std::string& fileName, void **handler);  
};

#endif