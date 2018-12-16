#include <string>
#include <fstream>
#include <streambuf>
#include <iostream>
#include "Interpreter.h"

using namespace std;

int main(int argc, char *argv[])
{
    bool interactive = true;

    Interpreter interpreter;

    string src = "";
    if(argc > 1)
    {
        string fileName(argv[1]);
        src = interpreter.OpenFile(fileName);
        interactive = false;
    }

    if(!interactive)
    {
        interpreter.Evaluate(src, false);
        if(interpreter.NeedBreak())
            cout << "\033[0m" << endl;
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
                cout << "\033[0m" << endl;
        }
    }

    return interpreter.ExitCode();
}