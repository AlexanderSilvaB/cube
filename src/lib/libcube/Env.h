#ifndef _ENV_H_
#define _ENV_H_

#include <map>
#include "Var.h"
#include <memory>

class Env;

typedef std::shared_ptr<Env> EnvPtr;

class Env : public std::enable_shared_from_this<Env>
{
    private:
        VarDict vars;
        EnvPtr parent;
    
    public:
        Env();
        Env(EnvPtr parent);
        ~Env();

        void setParent(EnvPtr parent);

        EnvPtr extend();
        EnvPtr lookup(const std::string& name);
        EnvPtr lookup(const std::string& name, VarType::Types type);
        bool contains(const std::string& name);
        bool contains(const std::string& name, VarType::Types type);
        bool exists(const std::string& name);
        Var* get(const std::string& name);
        Var* get(const std::string& name, VarType::Types type);
        Var* set(const std::string& name, Var* value);
        Var* def(const std::string& name, Var* value);
        void del(const std::string& name);
        EnvPtr copy(EnvPtr env = NULL);
        void paste(EnvPtr env);

        VarDict& Vars();

        std::string toString();
};

#endif