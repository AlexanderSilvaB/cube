#include <string>
#include <fstream>
#include <streambuf>
#include <iostream>
#include <libcube/ICube.h>

using namespace std;

int main(int argc, char *argv[])
{
    ICube icube;

    while(true)
    {
        cout << ">>> ";
        string line;
        getline(cin, line);
        if(!icube.run(line, true))
            break;
    }

    return icube.getExitCode();
}