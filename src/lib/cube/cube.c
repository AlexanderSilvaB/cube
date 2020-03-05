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
#include "mempool.h"
#include "util.h"
#include "vm.h"


#include "ansi_escapes.h"

extern Value nativeToValue(cube_native_var *var, NativeTypes *nt);
extern void valueToNative(cube_native_var *var, Value value);
char *version_string;

void start(const char *path, const char *scriptName)
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

    addPath("libs/");
    addPath("stdlib/");

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
#else
    addPath("/usr/local/share/cube/");
    addPath("/usr/local/share/cube/libs/");
    addPath("/usr/local/share/cube/stdlib/");
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
    start(argv[0], "__main__");
}

int runCube(int argc, const char *argv[])
{
    int rc = 0;
    const char *fileName = NULL;
    bool execute = true;
    bool debug = false;
    int argStart = 1;
    for (int i = 1; i < argc; i++)
    {
        if (existsFile(argv[i]) && fileName == NULL)
        {
            fileName = argv[i];
            argStart++;
        }
        else if (strcmp(argv[i], "-c") == 0 && execute == true)
        {
            execute = false;
            argStart++;
        }
        else if (strcmp(argv[i], "-d") == 0 && debug == false)
        {
            debug = true;
            argStart++;
        }
    }

    vm.debug = debug;

    loadArgs(argc, argv, argStart);

    if (fileName == NULL)
    {
        rc = repl();
    }
    else
    {
        char *folder = getFolder(fileName);
        if (folder != NULL)
            addPath(folder);
        rc = runFile(fileName, execute);
        if (vm.newLine)
            printf("\n");
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
    linenoise_add_keyword("none");
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
        if (vm.print == false || !IS_NONE(vm.repl))
        {
            printValue(vm.repl);
            printf("\n");
        }
        vm.repl = NONE_VAL;
        vm.print = false;
        if (!vm.running)
            break;
    }

    linenoise_save_history(historyPath);
    mp_free(historyPath);

    return vm.exitCode;
}

int runFile(const char *path, bool execute)
{
    char *source = readFile(path, true);
    if (source == NULL)
    {
        return 74;
    }

    InterpretResult result;
    if (execute)
        result = interpret(source, path);
    else
        result = compileCode(source, path);
    mp_free(source);

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
    cube_native_var *var = NATIVE_NONE();
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