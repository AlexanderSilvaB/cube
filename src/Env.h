#ifndef _ENV_H_
#define _ENV_H_

#include <map>
#include "Var.h"
#include <memory>

class Env;
typedef std::shared_ptr<Env> SEnv;

class Env
{
    private:
        std::map<std::string, SVar > vars;
        SEnv parent;
    
    public:
        Env();
        Env(SEnv parent);
        ~Env();

        SEnv extend();
        SEnv lookup(const std::string& name);
        bool contains(const std::string& name);
        SVar get(const std::string& name);
        SVar set(const std::string& name, SVar value);
        SVar def(const std::string& name, SVar value);

        std::string toString();
};

#endif