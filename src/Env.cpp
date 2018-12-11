#include "Env.h"
#include <sstream>
#include <iostream>

using namespace std;

Env::Env()
{
    
}

Env::Env(SEnv parent)
{
    this->parent = parent;
}

Env::~Env()
{
    
}

SEnv Env::extend()
{
    SEnv env(new Env(this));
    return env;
}

SEnv Env::lookup(const string& name)
{
    SEnv scope(this);
    while(!scope.empty())
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

SVar Env::get(const string& name)
{
    if(contains(name))
        return SVar(&vars[name]);
    stringstream ss;
    ss << "Undefined variable '" << name << "'";
    SVar error = MKVAR();
    error->Error(ss.str());
    return error;
}

SVar Env::set(const string& name, SVar value)
{
    SEnv scope = lookup(name);
    if(scope.empty() && !parent.empty())
    {
        stringstream ss;
        ss << "Undefined variable '" << name << "'";
        SVar error = MKVAR();
        error->Error(ss.str());
        return error;
    }
    if(scope.empty())
        scope = SEnv(this);
    scope->vars[name] = *value;
    return SVar(&scope->vars[name]);
}

SVar Env::def(const string& name, SVar value)
{
    vars[name] = *value;
    return value;
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