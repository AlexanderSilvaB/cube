#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cube.h"
#include "util.h"
#include "common.h"
#include "chunk.h"
#include "debug.h"
#include "vm.h"

void start(const char* path, const char* scriptName)
{
    //signal(SIGQUIT, handle_sigint);

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
    const char* fileName = NULL;
    bool execute = true;
    int argStart = 1;
    if(argc >= 2)
    {
        if(existsFile(argv[1]))
        {
            fileName = argv[1];
            argStart = 2;
        }
        if (argc > 2 && strcmp(argv[2], "-c") == 0)
        {
            execute = false;
            argStart = 3;
        }
    }

    loadArgs(argc, argv, argStart);

    if (fileName == NULL)
    {
        rc = repl();
    }
    else
    {
        rc = runFile(fileName, execute);
    }

    return rc;
}

void stopCube()
{
    stop();
}

/*
char *readLine(const char* str)
{
    printf("%s", str);

    int buffSz = 1024;
    char *line = (char*)malloc(buffSz);
    if (!fgets(line, sizeof(line), stdin))
    {
        printf("\n");
        free(line);
        return NULL;
    }

    return line;
}
*/

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
        
        interpret(line);
        if(vm.newLine)
        {
            printf("\n");
            vm.newLine = false;
        }
    }
    return 0;
}

int runFile(const char *path, bool execute)
{
    char *source = readFile(path, true);
    if (source == NULL)
    {
        return 74;
    }

    InterpretResult result;
    if(execute)
        result = interpret(source);
    else
        result = compileCode(source, path);
    free(source);

    if (result == INTERPRET_COMPILE_ERROR)
        return 65;
    if (result == INTERPRET_RUNTIME_ERROR)
        return 70;
    return 0;
}