#ifndef _INTERPRETER_H_
#define _INTERPRETER_H_

#include "Env.h"
#include "Var.h"
#include "Parser.h"
#include <map>

class Interpreter
{
    private:
        int exitCode;
        bool needBreak;
        bool exit;
        Parser parser;
        Env* env;

        std::map<std::string, int> colors;
        bool IsColor(const std::string& color);
        void ReplaceString(std::string& subject, const std::string& search, const std::string& replace);

        void MakeError(Var* var, const std::string& text);
        void MakeError(Var* var, const std::string& text, Node& node);
        Var* ApplyOperator(const std::string& op, Var* left, Var* middle, Var* right, Env *env);
        Var* Call(const std::string& func, std::vector<Var*>& args, Env *env);

        Var* Evaluate(Node node, Env* env);
    public:
        Interpreter();
        ~Interpreter();

        int ExitCode();
        bool NeedBreak();

        bool Evaluate(const std::string& src, bool interactive);
};

#endif