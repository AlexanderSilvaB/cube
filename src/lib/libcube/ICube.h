#ifndef _ICUBE_H_
#define _ICUBE_H_

#include <string>
#include "Parser.h"
#include "Object.h"
#include "Env.h"
#include "VM.h"

class ICube
{
    private:
        Parser parser;
        EnvPtr rootEnv;

        int exitCode;
        bool exit;
        bool canPrint;
        std::vector<std::string> directFunctions;

        void allowPrint(bool allow);
        Object* run(Node node, EnvPtr env, Object* caller = NULL);
        Object* call(const std::string& func, Array& args, EnvPtr env, Object* caller, bool createObj);
        Object* callCallable(Object* obj, Array args = Array());
    public:
        ICube();
        virtual ~ICube();

        int getExitCode();

        bool run(const std::string& src, bool cmd);
};

#endif
