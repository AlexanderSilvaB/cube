#ifndef _LOADER_H_
#define _LOADER_H_

#include <string>
#include <memory>
#include "Var.h"

class Loader;
typedef std::shared_ptr<Loader> LoaderPtr;

class Loader
{
    private:
        void callVoid();
        bool callBool();
        int callInt();
        char callChar();
        float callFloat();
        double callDouble();
        std::string callString();

    public:
        Loader();
        ~Loader();

        bool Load(const std::string& fileName, void **handler);  
        Var exec(const std::string& name, std::vector<Var*>& args, Var* funcDef, Var* caller);
};

#endif