#ifndef _INTERPRETER_H_
#define _INTERPRETER_H_

#include <string>
#include <vector>

class Interpreter
{
    public:
        Interpreter();
        ~Interpreter();

        void AddToPath(const std::string& path);

        std::string OpenFile(const std::string& fileName);

        std::vector<char> Compile(const std::string& code);
        bool Evaluate(std::string& code);

        bool NeedBreak();
        int ExitCode();
};


#endif
