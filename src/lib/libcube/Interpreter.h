#ifndef _INTERPRETER_H_
#define _INTERPRETER_H_

#include "Env.h"
#include "Var.h"
#include "Parser.h"
#include "Loader.h"
#include <map>
#include <set>

class Interpreter
{
    private:
        int exitCode;
        bool needBreak;
        bool exit;
        bool printInstr;
        Parser parser;
        EnvPtr env;
        LoaderPtr loader;

        std::set<std::string> paths;

        std::map<std::string, int> colors;
        bool IsColor(const std::string& color);
        void ReplaceString(std::string& subject, const std::string& search, const std::string& replace);
        std::string GetFolder(const std::string& path);
        std::string GetName(const std::string& path);
        std::string MakePath(const std::string& fileName);

        Var* CreateClass(Node node, EnvPtr env);

        void MakeError(Var* var, const std::string& text);
        void MakeError(Var* var, const std::string& text, Node& node);
        Var* ApplyOperator(const std::string& op, Var* left, Var* middle, Var* right, EnvPtr env);
        Var* Call(const std::string& func, std::vector<Var*>& args, EnvPtr env, Var* caller = NULL, bool createObj = false);
        bool IsValidInternCall(const std::string& func, Var* obj);
        Var* CallInternOrReturn(const std::string& func, Var* obj, std::vector<Var*> args = std::vector<Var*>());

        Var* Evaluate(Node node, EnvPtr env, bool isClass = false, Var *caller = NULL);
    public:
        Interpreter();
        ~Interpreter();

        int ExitCode();
        bool NeedBreak();

        std::string OpenFile(const std::string& fileName);
        void AddToPath(const std::string& fileName, bool extract = true);

        bool Evaluate(const std::string& src, bool interactive);
        std::vector<char> Compile(const std::string& src);
};

#endif