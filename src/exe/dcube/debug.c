#include <cube/cube.h>
#include <cube/threads.h>
#include <cube/util.h>
#include <stdio.h>

int rc;
bool running;
int _argc;
char **_argv;

typedef struct BreakPoint_st
{
    int line;
    char *path;
    bool enabled;
    struct BreakPoint_st *next;
}BreakPoint;

BreakPoint *breakpoints = NULL;
BreakPoint *openBreakPoints = NULL;

bool stopOnInit = false;

void *cube(void *arg);

void init_debugger(int argc, const char *argv[])
{
    if(argc > 1)
    {
        _argc = 3;
    }
    else
    {
        _argc = 2;
    }

    _argv = (char**)malloc(sizeof(char*) * _argc);
    _argv[0] = (char*)argv[0];
    if(_argc == 2)
    {
        _argv[1] = "-d";
    }
    else
    {
        _argv[1] = (char*)argv[1];
        _argv[2] = "-d";
        if(argc > 2)
        {
            if(strcmp(argv[2], "-s") == 0)
            {
                stopOnInit = true;
            }
        }
    }

    printf("Start debugging\n");
    if(stopOnInit)
        printf("Stop on init: true\n");
    else
        printf("Stop on init: false\n");
    

    running = true;
    thread_create(cube, NULL);
}

void finish_debugger()
{
    printf("Finish debugging\n");
    free(_argv);
}

void *cube(void *arg)
{
    startCube(_argc, (const char**)_argv);
    rc = runCube(_argc, (const char**)_argv);
    stopCube();
    running = false;
    return NULL;
}

void addBreakpoint(char *line)
{
    char *path = NULL;
    for(int i = 0; i < strlen(line); i++)
    {
        if(line[i] == ' ')
        {
            line[i] = '\0';
            i++;
            path = &line[i];
            break;
        }
    }

    if(line == NULL || path == NULL)
        return;

    int ln = atoi(line);

    printf("Add breakpoint: %s(%d)\n", path, ln);

    BreakPoint *current = breakpoints;
    BreakPoint *parent = NULL;
    while(current != NULL)
    {
        if(current->line == ln && strcmp(current->path, path) == 0)
        {
            break;
        }
        parent = current;
        current = current->next;
    }

    if(current == NULL)
    {
        current = (BreakPoint*)malloc(sizeof(BreakPoint));
        current->line = ln;
        current->path = (char*)malloc(sizeof(char) * (strlen(path) + 1));
        strcpy(current->path, path);
        current->enabled = true;
        current->next = NULL;
        if(parent == NULL)
            breakpoints = current;
        else
            parent->next = current;
    }
}

void removeBreakpoint(char *line)
{
    char *path = NULL;
    for(int i = 0; i < strlen(line); i++)
    {
        if(line[i] == ' ')
        {
            line[i] = '\0';
            i++;
            path = &line[i];
            break;
        }
    }

    if(line == NULL || path == NULL)
        return;

    int ln = atoi(line);

    printf("Remove breakpoint: %s(%d)\n", path, ln);

    BreakPoint *current = breakpoints;
    BreakPoint *parent = NULL;
    while(current != NULL)
    {
        if(current->line == ln && strcmp(current->path, path) == 0)
        {
            if(parent != NULL)
                parent->next = current->next;
            free(current->path);
            free(current);
            break;
        }
        parent = current;
        current = current->next;
    }
}

bool isOpenBreakPoint(int line, const char *path)
{
    BreakPoint *current = openBreakPoints;
    BreakPoint *parent = NULL;
    while(current != NULL)
    {
        if(current->line == line && strcmp(current->path, path) == 0)
        {
            return true;
        }
        parent = current;
        current = current->next;
    }
    return false;
}

void addOpenBreakPoint(int line, const char *path)
{
    BreakPoint *current = openBreakPoints;
    BreakPoint *parent = NULL;
    while(current != NULL)
    {
        if(strcmp(current->path, path) == 0)
        {
            current->line = line;
            return;
        }
        parent = current;
        current = current->next;
    }
    current = (BreakPoint*)malloc(sizeof(BreakPoint));
    current->line = line;
    current->path = (char*)malloc(sizeof(char) * (strlen(path) + 1));
    strcpy(current->path, path);
    current->enabled = true;
    current->next = NULL;
    if(parent == NULL)
        openBreakPoints = current;
    else
        parent->next = current;
}

bool hasBreakPoint(int line, const char *path)
{
    if(stopOnInit)
    {
        stopOnInit = false;
        return true;
    }
    
    BreakPoint *current = breakpoints;
    while(current != NULL)
    {
        if(current->line == line && strcmp(current->path, path) == 0)
        {
            if(isOpenBreakPoint(line, path))
            {
                return false;
            }

            addOpenBreakPoint(line, path);
            return true;
        }
        current = current->next;
    }

    addOpenBreakPoint(line, path);

    return false;
}