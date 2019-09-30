#include <string>
#include <fstream>
#include <streambuf>
#include <iostream>
#include <libcube/Interpreter.h>

using namespace std;

int main(int argc, char *argv[])
{
    VM::Init();
    
    bool interactive = true;

    Interpreter interpreter;
    interpreter.AddToPath(string(argv[0]));
    interpreter.AddToPath("examples/");

    string src = "";
    string output = "";
    for(int i = 1; i < argc; i++)
    {
        string arg = argv[i];
        if(arg == "-c")
        {
            i++;
            output = argv[i];
        }
        else
        {
            string fileName(arg);
            src = interpreter.OpenFile(fileName);
            interactive = false;
        }
    }

    if(output.size() > 0)
    {
        vector<char> data = interpreter.Compile(src);
        if(data.size() > 0)
        {
            ofstream out(output);
            out.write(data.data(), data.size());
            out.close();
        }
    }
    else
    {
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
    }

    VM::Destroy();

    return interpreter.ExitCode();
}