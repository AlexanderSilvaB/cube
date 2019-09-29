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
        void callVoid(NamesArray& names, std::vector<Var*>& args, void* handler);
        bool callBool(NamesArray& names, std::vector<Var*>& args, void* handler);
        int callInt(NamesArray& names, std::vector<Var*>& args, void* handler);
        char callChar(NamesArray& names, std::vector<Var*>& args, void* handler);
        float callFloat(NamesArray& names, std::vector<Var*>& args, void* handler);
        double callDouble(NamesArray& names, std::vector<Var*>& args, void* handler);
        std::string callString(NamesArray& names, std::vector<Var*>& args, void* handler);

    public:
        Loader();
        ~Loader();

        bool Load(const std::string& fileName, void **handler);  
        Var exec(const std::string& name, std::vector<Var*>& args, Var* funcDef, Var* caller);
};

#endif