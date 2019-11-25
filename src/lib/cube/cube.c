#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cube.h"

void start()
{
    initVM();
}

void stop()
{
    freeVM();
}

int cube(int argc, const char* argv[])
{
    start();

    int rc = 0;
    if(argc == 1)
    {
        rc = repl();
    }
    else if(argc >= 2)
    {
        bool justCompile = false;
        const char* path = argv[1];
        if(argc > 2 && strcmp(argv[2], "-c") == 0)
            justCompile = true;

        rc = runFile(path, justCompile);
    }
    else
    {
        fprintf(stderr, "Usage: cube [path]\n");
        rc = 64;
    }

    stop();
    return rc;
}

int repl()
{
    char line[1024];
    for(;;)
    {
        printf("> ");
        if (!fgets(line, sizeof(line), stdin)) 
        {
            printf("\n");
            break;
        }

        interpret(line, NULL, false);
    }
    return 0;
}

char* readFile(const char* path)
{
    FILE* file = fopen(path, "rb");
    if(file == NULL)
    {
        fprintf(stderr, "Could not open file \"%s\".\n", path);
        return NULL;
    }

    fseek(file, 0L, SEEK_END);
    size_t fileSize = ftell(file);
    rewind(file);

    char* buffer = (char*)malloc(fileSize + 1);
    if(buffer == NULL)
    {
        fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
        fclose(file);
        return NULL;
    }

    size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
    if(bytesRead < fileSize)
    {
        fprintf(stderr, "Could not read file \"%s\".\n", path);
        free(buffer);
        fclose(file);
        return NULL;
    }
    buffer[bytesRead] = '\0';

    fclose(file);
    return buffer;
}

int runFile(const char* path, bool justCompile)
{
    char* source = readFile(path);
    if(source == NULL)
    {
        return 74;
    }
    InterpretResult result = interpret(source, path, justCompile);
    free(source);

    if (result == INTERPRET_COMPILE_ERROR) return 65;
    if (result == INTERPRET_RUNTIME_ERROR) return 70;
    return 0;
}