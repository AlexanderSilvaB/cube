#include "Env.h"
#include "VM.h"
#include <iostream>

using namespace std;

Env::Env()
{

}

Env::Env(EnvPtr env)
{
    parent = env;
}

Env::~Env()
{
    for(Map::iterator it = map.begin(); it != map.end(); it++)
    {
        it->second->release();
        VM::release(it->second);
    }
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

bool Env::contains(const string& name)
{
    Map::iterator it = map.find(name);
    return it != map.end();
}

bool Env::contains(const string& name, ObjectTypes type)
{
    Map::iterator it = map.find(name);
    if(it != map.end())
    {
        if(it->second->getType() == type)
            return true;
    }
    return false;
}

bool Env::exists(const string& name)
{
    EnvPtr scope = lookup(name);
    return scope != NULL;
}

void Env::set(const string& name, Object* obj, bool local)
{
    if(local)
    {
        if(map.find(name) != map.end())
        {
            map[name]->release();
            VM::release(map[name]);
        }
        map[name] = obj;
    }
    else
    {
        EnvPtr scope = lookup(name);
        if(!scope)
        {
            if(map.find(name) != map.end())
            {
                map[name]->release();
                VM::release(map[name]);
            }
            map[name] = obj;
        }
        else
        {
            if(scope->map.find(name) != scope->map.end())
            {
                scope->map[name]->release();
                VM::release(scope->map[name]);
            }
            scope->map[name] = obj;
        }
    }
    obj->save();
}

Object* Env::get(const string& name)
{
    EnvPtr scope = lookup(name);
    if(!scope)
    {
        return NULL;
    }
    return scope->map[name];
}

bool Env::del(const std::string& name)
{
    EnvPtr scope = lookup(name);
    if(scope)
    {
        Map::iterator it = scope->map.find(name);
        it->second->release();
        VM::release(it->second);
        scope->map.erase(it);
        return true;
    }
    else if(name == "local")
    {
        for(Map::iterator it = map.begin(); it != map.end(); it++)
        {
            it->second->release();
            VM::release(it->second);
        }
        map.clear();
        return true;
    }
    else if(name == "all")
    {
        scope = shared_from_this();
        while(scope)
        {
            for(Map::iterator it = scope->map.begin(); it != scope->map.end(); it++)
            {
                it->second->release();
                VM::release(it->second);
            }
            scope->map.clear();
            scope = scope->parent;
        }
        return true;
    }
    return false;
}

EnvPtr Env::copy(EnvPtr env)
{
    if(!env)
    {
        env = EnvPtr(new Env());
    }
    for(Map::iterator it = map.begin(); it != map.end(); it++)
    {
        Object* obj = VM::create();
        VM::push(obj);
        obj->from(it->second);
        env->set(it->first, obj, true);
    }
    if(parent != NULL)
        parent->copy(env);
    return env;
}

void Env::paste(EnvPtr env)
{
    for(Map::iterator it = env->map.begin(); it != env->map.end(); it++)
    {
        set(it->first, it->second, false);
    }
}

void Env::toDict(Object *obj)
{
    obj->toDict(map);
}