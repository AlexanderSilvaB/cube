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
    map<string, Var>::iterator it = vars.find(name);
    return it != vars.end();
}

Var* Env::get(const string& name)
{
    if(contains(name))
        return &vars[name];
    stringstream ss;
    ss << "Undefined variable '" << name << "'";
    Var *error = MKVAR();
    error->Error(ss.str());
    return error;
}

Var* Env::set(const string& name, Var* value)
{
    Env* scope = lookup(name);
    if(!scope && parent)
    {
        stringstream ss;
        ss << "Undefined variable '" << name << "'";
        Var* error = MKVAR();
        error->Error(ss.str());
        return error;
    }
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

string Env::toString()
{
    stringstream ss;
    for(map<string, Var>::iterator it = vars.begin(); it != vars.end(); it++)
    {
        ss << it->first << " : " << it->second << endl;
    }
    return ss.str();
}