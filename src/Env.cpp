#include "Env.h"
#include <sstream>
#include <iostream>

using namespace std;

Env::Env()
{
    this->parent = NULL;   
}

Env::Env(Env* parent)
{
    this->parent = parent;
}

Env::~Env()
{
    
}

Env* Env::extend()
{
    Env* env = new Env(this);
    return env;
}

Env* Env::lookup(const string& name)
{
    Env* scope = this;
    while(scope)
    {
        if(scope->contains(name))
            return scope;
        scope = scope->parent;
    }
    return scope;
}

bool Env::contains(const string& name)
{
    VarDict::iterator it = vars.find(name);
    return it != vars.end();
}

Var* Env::get(const string& name)
{
    Env* scope = lookup(name);
    if(!scope)
    {
        stringstream ss;
        ss << "Undefined variable or function '" << name << "'";
        Var *error = MKVAR();
        error->Error(ss.str());
        return error;
    }
    return &scope->vars[name];
}

Var* Env::set(const string& name, Var* value)
{
    Env* scope = lookup(name);
    /*
    if(!scope && parent)
    {
        stringstream ss;
        ss << "Undefined variable or function '" << name << "'";
        Var* error = MKVAR();
        error->Error(ss.str());
        return error;
    }
    */
    if(!scope)
    {
        this->vars[name] = *value;
        this->vars[name].Store();
        return &this->vars[name];
    }
    scope->vars[name] = *value;
    scope->vars[name].Store();
    return &scope->vars[name];
    
}

Var* Env::def(const string& name, Var* value)
{
    vars[name] = *value;
    vars[name].Store();
    return &vars[name];
}

VarDict& Env::Vars()
{
    return vars;
}

Env* Env::copy(Env *env)
{
    if(env == NULL)
        env = new Env();
    for(VarDict::iterator it = vars.begin(); it != vars.end(); it++)
    {
        env->def(it->first, &it->second);
    }
    if(parent != NULL)
        parent->copy(env);
    return env;
}

void Env::paste(Env *env)
{
    for(VarDict::iterator it = env->vars.begin(); it != env->vars.end(); it++)
    {
        set(it->first, &it->second);
    }
}

string Env::toString()
{
    stringstream ss;
    for(VarDict::iterator it = vars.begin(); it != vars.end(); it++)
    {
        ss << it->first << " : " << it->second << endl;
    }
    if(parent != NULL)
    {
        ss << "-----------" << endl;
        ss << parent->toString();
    }
    return ss.str();
}