#ifndef _ENV_H_
#define _ENV_H_

#include <string>
#include <unordered_map>
#include <memory>
#include "Object.h"

class Env;
typedef std::shared_ptr<Env> EnvPtr;

class Env : public std::enable_shared_from_this<Env>
{
    private:
        EnvPtr parent;
        Map map;

        EnvPtr lookup(const std::string& name);
    public:
        Env();
        Env(EnvPtr env);
        virtual ~Env();

        EnvPtr extend();
        EnvPtr copy(EnvPtr env = EnvPtr());
        void paste(EnvPtr env = EnvPtr());

        bool contains(const std::string& name);
        bool contains(const std::string& name, ObjectTypes type);
        bool exists(const std::string& name);

        void set(const std::string& name, Object* obj, bool local);
        Object* get(const std::string& name);
        bool del(const std::string& name);
        void toDict(Object* obj);
};

#endif
