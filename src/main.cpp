#include <string>
#include <fstream>
#include <streambuf>
#include <iostream>
#include "Interpreter.h"

using namespace std;

int main(int argc, char *argv[])
{
    bool interactive = true;

    string src = "";
    if(argc > 1)
    {
        string fileName(argv[1]);
        ifstream t(fileName);
        t.seekg(0, std::ios::end);   
        src.reserve(t.tellg());
        t.seekg(0, std::ios::beg);
        src.assign((istreambuf_iterator<char>(t)), istreambuf_iterator<char>());
        interactive = false;
    }

    Interpreter interpreter;
    if(!interactive)
    {
        interpreter.Evaluate(src, false);
    }
    else
    {
        while(true)
        {
            cout << ">>> ";
            string line;
            getline(cin, line);
            if(!interpreter.Evaluate(line, true))
                break;
            if(interpreter.NeedBreak())
                cout << endl;
        }
    }

    return interpreter.ExitCode();
}