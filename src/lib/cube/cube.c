#include <locale.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <linenoise/linenoise.h>

#include "chunk.h"
#include "common.h"
#include "cube.h"
#include "debug.h"
#include "linkedList.h"
#include "mempool.h"
#include "packer.h"
#include "util.h"
#include "vm.h"

#include "ansi_escapes.h"

extern Value nativeToValue(cube_native_var *var, NativeTypes *nt);
extern void valueToNative(cube_native_var *var, Value value);
char *version_string;
extern bool printCode;

void start(const char *path, const char *scriptName, const char *rootPath)
{

#ifdef UNICODE
    setlocale(LC_ALL, "");
    setlocale(LC_CTYPE, "UTF-8");
#endif

    setupConsole();

    if (!mp_init())
    {
        printf("Could not allocate memory. Exiting!");
        exit(-1);
    }

    version_string = (char *)mp_malloc(sizeof(char) * 32);
    snprintf(version_string, 31, "%d.%d", VERSION_MAJOR, VERSION_MINOR);

    char *folder = NULL;
    char cCurrentPath[FILENAME_MAX];
    bool findInPath = true;
    if (GetCurrentDir(cCurrentPath, sizeof(cCurrentPath)))
    {
        cCurrentPath[sizeof(cCurrentPath) - 1] = '\0';
        folder = cCurrentPath;
    }
    else
    {
        folder = getFolder(path);
        findInPath = false;
    }

    initVM(folder, scriptName);
    if (findInPath)
    {
        folder = getFolder(path);
        if (folder != NULL)
            addPath(folder);
    }

    if (rootPath != NULL)
    {
        vm.rootPath = STRING_VAL(rootPath);
        addPath(rootPath);

        char *newPath = (char *)mp_malloc(sizeof(char) * (strlen(rootPath) + 16));

        strcpy(newPath, rootPath);
        strcat(newPath, "/libs/");
        addPath(newPath);

        strcpy(newPath, rootPath);
        strcat(newPath, "/stdlib/");
        addPath(newPath);

        strcpy(newPath, rootPath);
        strcat(newPath, "/stdlib/libs/");
        addPath(newPath);

        mp_free(newPath);
    }

    addPath("libs/");
    addPath("stdlib/");
    addPath("stdlib/libs/");
    addPath("../share/cube/");
    addPath("../share/cube/libs/");
    addPath("../share/cube/stdlib/");
    addPath("../share/cube/stdlib/libs/");

#ifdef _WIN32
    addPath("C:/cube/share/cube/");
    addPath("C:/cube/share/cube/libs/");
    addPath("C:/cube/share/cube/stdlib/");
    addPath("C:/Program Files/cube/share/cube/");
    addPath("C:/Program Files/cube/share/cube/libs/");
    addPath("C:/Program Files/cube/share/cube/stdlib/");
    addPath("C:/Program Files (x86)/cube/share/cube/");
    addPath("C:/Program Files (x86)/cube/share/cube/libs/");
    addPath("C:/Program Files (x86)/cube/share/cube/stdlib/");
    addPath("C:/Program Files (x86)/cube/share/cube/stdlib/libs/");
#else
    addPath("/usr/local/share/cube/");
    addPath("/usr/local/share/cube/libs/");
    addPath("/usr/local/share/cube/stdlib/");
    addPath("/usr/local/share/cube/stdlib/libs/");
#endif

    addPath("~/.cube/");
    addPath("~/.cube/libs/");
}

void stop()
{
    freeVM();
    mp_free(version_string);
    mp_destroy();
    restoreConsole();
}

void startCube(int argc, const char *argv[])
{
    const char *rootPath = NULL;
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--root") == 0)
        {
            i++;
            rootPath = argv[i];
            break;
        }
    }
    start(argv[0], "__main__", rootPath);
}

int runCube(int argc, const char *argv[])
{
    int rc = 0;
    bool allocatedFileName = false;
    char *fileName = NULL;
    char *output = NULL;
    char *code = NULL;
    bool execute = true;
    bool binary = false;
    bool debug = false;
    bool forceInclude = false;
    int argStart = 1;
    for (int i = 1; i < argc; i++)
    {
        if (existsFile(argv[i]) && fileName == NULL)
        {
            fileName = argv[i];
            argStart++;
            break;
        }
        else if (strcmp(argv[i], "-e") == 0 || strcmp(argv[i], "--execute") == 0)
        {
            i++;
            code = argv[i];
            argStart += 2;
        }
        else if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0)
        {
            i++;
            output = argv[i];
            argStart += 2;
        }
        else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--compile") == 0)
        {
            execute = false;
            argStart++;
        }
        else if (strcmp(argv[i], "-b") == 0 || strcmp(argv[i], "--binary") == 0)
        {
            binary = true;
            execute = false;
            argStart++;
        }
        else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--debug") == 0)
        {
            debug = true;
            argStart++;
        }
        else if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--print") == 0)
        {
            printCode = true;
            argStart++;
        }
        else if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--include") == 0)
        {
            forceInclude = true;
            argStart++;
        }
        else if (strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--root") == 0)
        {
            i++;
            argStart += 2;
        }
        else if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--sample") == 0)
        {
            i++;
            allocatedFileName = true;
            fileName = (char *)mp_malloc(sizeof(char) * (strlen(argv[i]) + 128));
            strcpy(fileName, "~/.cube/samples/");
            strcat(fileName, argv[i]);
            argStart += 2;
        }
        else if (strcmp(argv[i], "-l") == 0 || strcmp(argv[i], "--lib") == 0)
        {
            i++;
            allocatedFileName = true;
            fileName = (char *)mp_malloc(sizeof(char) * (strlen(argv[i]) + 128));
            strcpy(fileName, "~/.cube/libs/");
            strcat(fileName, argv[i]);
            argStart += 2;
        }
        else if (strcmp(argv[i], "-m") == 0 || strcmp(argv[i], "--module") == 0)
        {
            i++;
            allocatedFileName = true;
            fileName = (char *)mp_malloc(sizeof(char) * (strlen(argv[i]) + 128));
            strcpy(fileName, "~/.cube/bin/");
            strcat(fileName, argv[i]);
            argStart += 2;
        }
    }

    vm.debug = debug;
    vm.forceInclude = forceInclude;

    loadArgs(argc, argv, argStart);

    if (argc == 1 || (argc == 2 && forceInclude) || (argc == 2 && debug))
    {
        rc = repl();
    }
    else if (fileName == NULL)
    {
        if (code == NULL)
        {
            fprintf(stderr, "Could not open the source file.\n");
            rc = -1;
        }
        else
        {
            char *folder = getFolder(".");
            if (folder != NULL)
                addPath(folder);

            InterpretResult result;
            if (execute)
                result = interpret(code, folder);
            else
            {
                if (binary)
                {
                    if (pack(code, folder, output))
                        result = INTERPRET_OK;
                    else
                        result = INTERPRET_COMPILE_ERROR;
                }
                else
                    result = compileCode(code, folder, output);
            }
            if (vm.newLine)
                printf("\n");
            mp_free(folder);

            if (result == INTERPRET_COMPILE_ERROR)
                rc = 65;
            if (result == INTERPRET_RUNTIME_ERROR)
                rc = 70;
            else
                rc = vm.exitCode;
        }
    }
    else
    {
        char *folder = getFolder(fileName);
        if (folder != NULL)
            addPath(folder);
        rc = runFile(fileName, output, execute, binary);
        if (vm.newLine)
            printf("\n");
    }

    if (allocatedFileName)
    {
        mp_free(fileName);
    }

    return rc;
}

void stopCube()
{
    stop();
}

int repl()
{
    linenoise_init();

    linenoise_add_keyword("and");
    linenoise_add_keyword("or");
    linenoise_add_keyword("not");
    linenoise_add_keyword("var");
    linenoise_add_keyword("for");
    linenoise_add_keyword("while");
    linenoise_add_keyword("do");
    linenoise_add_keyword("if");
    linenoise_add_keyword("else");
    linenoise_add_keyword("true");
    linenoise_add_keyword("false");
    linenoise_add_keyword("null");
    linenoise_add_keyword("class");
    linenoise_add_keyword("func");
    linenoise_add_keyword("static");
    linenoise_add_keyword("global");
    linenoise_add_keyword("return");
    linenoise_add_keyword("super");
    linenoise_add_keyword("this");
    linenoise_add_keyword("in");
    linenoise_add_keyword("is");
    linenoise_add_keyword("continue");
    linenoise_add_keyword("break");
    linenoise_add_keyword("switch");
    linenoise_add_keyword("case");
    linenoise_add_keyword("default");
    linenoise_add_keyword("nan");
    linenoise_add_keyword("inf");
    linenoise_add_keyword("import");
    linenoise_add_keyword("include");
    linenoise_add_keyword("require");
    linenoise_add_keyword("as");
    linenoise_add_keyword("native");
    linenoise_add_keyword("let");
    linenoise_add_keyword("with");
    linenoise_add_keyword("async");
    linenoise_add_keyword("await");
    linenoise_add_keyword("abort");
    linenoise_add_keyword("try");
    linenoise_add_keyword("catch");
    linenoise_add_keyword("for");
    linenoise_add_keyword("cube");

    linenoise_set_multiline(true);
    linenoise_set_history_max_len(20);

    char *historyPath = fixPath("~/.cube/history.txt");
    linenoise_load_history(historyPath);

    printf("%s %s\n", LANG_NAME, version_string);

    char line[LINENOISE_MAX_LINE];
    for (;;)
    {
        if (linenoise_read_line("> ", line))
        {
            printf("\n");
            break;
        }

        if (strlen(line) == 0 || line[0] == '\n')
            continue;

        linenoise_add_history(line);

        interpret(line, NULL);
        if (vm.newLine)
        {
            printf("\n");
            vm.newLine = false;
        }
        if (vm.print == false || !IS_NULL(vm.repl))
        {
            printValue(vm.repl);
            printf("\n");
        }
        vm.repl = NULL_VAL;
        vm.print = false;
        if (!vm.running)
            break;
    }

    linenoise_save_history(historyPath);
    mp_free(historyPath);

    return vm.exitCode;
}

int runFile(const char *path, const char *output, bool execute, bool binary)
{
    char *nPath = fixPath(path);
    char *source = readFile(nPath, true);
    if (source == NULL)
    {
        mp_free(nPath);
        return 74;
    }

    InterpretResult result;
    if (execute)
        result = interpret(source, nPath);
    else
    {
        if (binary)
        {
            if (pack(source, nPath, output))
                result = INTERPRET_OK;
            else
                result = INTERPRET_COMPILE_ERROR;
        }
        else
            result = compileCode(source, nPath, output);
    }
    mp_free(source);
    mp_free(nPath);

    if (result == INTERPRET_COMPILE_ERROR)
        return 65;
    if (result == INTERPRET_RUNTIME_ERROR)
        return 70;
    return vm.exitCode;
}

int runCode(const char *source, const char *path, int argc, const char *argv[])
{
    loadArgs(argc, argv, 1);

    InterpretResult result;
    result = interpret(source, path);

    if (result == INTERPRET_COMPILE_ERROR)
        return 65;
    if (result == INTERPRET_RUNTIME_ERROR)
        return 70;
    return vm.exitCode;
}

bool addGlobal(const char *name, cube_native_var *var)
{
    if (!vm.ready)
        return false;
    NativeTypes nt;
    Value val = nativeToValue(var, &nt);
    tableSet(&vm.globals, AS_STRING(STRING_VAL(name)), val);
    return true;
}

cube_native_var *getGlobal(const char *name)
{
    if (!vm.ready)
        return NULL;
    cube_native_var *var = NATIVE_NULL();
    Value val;
    if (tableGet(&vm.globals, AS_STRING(STRING_VAL(name)), &val))
    {
        valueToNative(var, val);
    }
    return var;
}

void debug(bool enable)
{
    vm.debug = enable;
}

void continueDebug()
{
    vm.continueDebug = true;
}

bool waitingDebug()
{
    return vm.waitingDebug;
}

DebugInfo debugInfo()
{
    return vm.debugInfo;
}