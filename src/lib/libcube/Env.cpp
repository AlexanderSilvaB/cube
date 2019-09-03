#include "Env.h"
#include <sstream>
#include <iostream>

using namespace std;

Env::Env()
{
    this->parent = NULL;   
}

Env::Env(EnvPtr parent)
{
    this->parent = parent;
}

Env::~Env()
{
    
}

void Env::setParent(EnvPtr parent)
{
    this->parent = parent;
}

EnvPtr Env::extend()
{
    Env* env = new Env( shared_from_this() );
    return EnvPtr(env);
}

EnvPtr Env::lookup(const string& name)
{
    EnvPtr scope = shared_from_this();
    while(scope)
    {
        if(scope->contains(name))
            return scope;
        scope = scope->parent;
    }
    return scope;
}

EnvPtr Env::lookup(const string& name, VarType::Types type)
{
    EnvPtr scope = shared_from_this();
    while(scope)
    {
        if(scope->contains(name, type))
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

bool Env::contains(const string& name, VarType::Types type)
{
    VarDict::iterator it = vars.find(name);
    if(it != vars.end())
    {
        if(it->second.Type() == type)
            return true;
    }
    return false;
}

bool Env::exists(const string& name)
{
    EnvPtr scope = lookup(name);
    return scope != NULL;
}

Var* Env::get(const string& name)
{
    EnvPtr scope = lookup(name);
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

Var* Env::get(const string& name, VarType::Types type)
{
    EnvPtr scope = lookup(name, type);
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
    EnvPtr scope = lookup(name);
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

EnvPtr Env::copy(EnvPtr env)
{
    if(!env)
    {
        env = EnvPtr(new Env());
    }
    for(VarDict::iterator it = vars.begin(); it != vars.end(); it++)
    {
        env->def(it->first, &it->second);
    }
    if(parent != NULL)
        parent->copy(env);
    return env;
}

void Env::paste(EnvPtr env)
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