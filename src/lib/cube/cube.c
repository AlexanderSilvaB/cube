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
#include "packer.h"
#include "util.h"
#include "vm.h"
#include "linkedList.h"

#include "ansi_escapes.h"

extern linked_list *list_symbols(const char *path);

extern Value nativeToValue(cube_native_var *var, NativeTypes *nt);
extern void valueToNative(cube_native_var *var, Value value);
char *version_string;

void start(const char *path, const char *scriptName)
{
	list_symbols("C:/Users/Alexander/CMakeBuilds/964c335c-d18a-0937-bac8-aca7c879eef4/install/x64-Debug (padrão)/share/cube/libs/calc.dll");

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
	addPath("../share/cube/");
	addPath("../share/cube/libs/");
	addPath("../share/cube/stdlib/");

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
    const char *output = NULL;
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
        }
        else if (strcmp(argv[i], "-o") == 0)
        {
            i++;
            output = argv[i];
            argStart += 2;
        }
        else if (strcmp(argv[i], "-c") == 0)
        {
            execute = false;
            argStart++;
        }
        else if (strcmp(argv[i], "-b") == 0)
        {
            binary = true;
            execute = false;
            argStart++;
        }
        else if (strcmp(argv[i], "-d") == 0)
        {
            debug = true;
            argStart++;
        }
        else if (strcmp(argv[i], "-i") == 0)
        {
            forceInclude = true;
            argStart++;
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
        fprintf(stderr, "Could not open the source file.\n");
        rc = -1;
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
    char *source = readFile(path, true);
    if (source == NULL)
    {
        return 74;
    }

    InterpretResult result;
    if (execute)
        result = interpret(source, path);
    else
    {
        if (binary)
        {
            if (pack(source, path, output))
                result = INTERPRET_OK;
            else
                result = INTERPRET_COMPILE_ERROR;
        }
        else
            result = compileCode(source, path, output);
    }
    mp_free(source);

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