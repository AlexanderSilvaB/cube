#ifndef _INTERPRETER_H_
#define _INTERPRETER_H_

#include "Env.h"
#include "Var.h"
#include "Parser.h"

class Interpreter
{
    private:
        int exitCode;
        Parser parser;
        SEnv env;

        void MakeError(SVar& var, const std::string& text, Node& node);

        SVar Evaluate(Node node, SEnv env);
    public:
        Interpreter();
        ~Interpreter();

        int ExitCode();

        bool Evaluate(const std::string& src, bool interactive);
};

#endif