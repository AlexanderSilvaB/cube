#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cube.h"
#include "util.h"

void start(const char* path, const char* scriptName)
{
    char *folder = getFolder(path);
    initVM(folder, scriptName);
}

void stop()
{
    freeVM();
}

void startCube(int argc, const char *argv[])
{
    start(argv[0], "__main__");
}

int runCube(int argc, const char *argv[])
{
    int rc = 0;
    if (argc == 1)
    {
        rc = repl();
    }
    else if (argc >= 2)
    {
        bool justCompile = false;
        const char *path = argv[1];
        if (argc > 2 && strcmp(argv[2], "-c") == 0)
            justCompile = true;

        rc = runFile(path, justCompile);
    }
    else
    {
        fprintf(stderr, "Usage: cube [path]\n");
        rc = 64;
    }

    return rc;
}

void stopCube()
{
    stop();
}

int repl()
{
    char line[1024];
    for (;;)
    {
        printf("> ");
        if (!fgets(line, sizeof(line), stdin))
        {
            printf("\n");
            break;
        }

        //interpret(line, NULL, false);
        interpret(line);
        if(vm.newLine)
        {
            printf("\n");
            vm.newLine = false;
        }
    }
    return 0;
}

int runFile(const char *path, bool justCompile)
{
    char *source = readFile(path, true);
    if (source == NULL)
    {
        return 74;
    }
    //InterpretResult result = interpret(source, path, justCompile);
    InterpretResult result = interpret(source);
    free(source);

    if (result == INTERPRET_COMPILE_ERROR)
        return 65;
    if (result == INTERPRET_RUNTIME_ERROR)
        return 70;
    return 0;
}