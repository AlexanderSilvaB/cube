#ifndef _ENV_H_
#define _ENV_H_

#include <map>
#include "Var.h"
#include <memory>

class Env
{
    private:
        VarDict vars;
        Env* parent;
    
    public:
        Env();
        Env(Env* parent);
        ~Env();

        Env* extend();
        Env* lookup(const std::string& name);
        bool contains(const std::string& name);
        Var* get(const std::string& name);
        Var* set(const std::string& name, Var* value);
        Var* def(const std::string& name, Var* value);

        std::string toString();
};

#endif