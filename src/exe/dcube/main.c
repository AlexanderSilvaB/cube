#include <cube/cube.h>
#include <cube/threads.h>
#include <cube/util.h>
#include <stdio.h>
#include "debug.h"

extern int rc;
extern bool running;

int main(int argc, const char *argv[])
{
    init_debugger(argc, argv);

    char line[1024];
    int steps = INT32_MAX;
    int len;

    while (running)
    {
        if(waitingDebug())
        {
            steps--;
            DebugInfo info = debugInfo();
            if(hasBreakPoint(info.line, info.path) || steps == 0)
            {
                printf("Step: %s(%d)\n", info.path == NULL ? "NULL" : info.path, info.line);

                getOption:
                printf(": ");
                if (!fgets(line, 1024, stdin))
                {
                    printf("Invalid option\n");
                    goto getOption;
                }

                len = strlen(line);

                for(int i = 0; i < 2; i++)
                {
                    if(len > 0)
                    {
                        if(line[len-1] == '\n' || line[len-1] == '\r')
                        {
                            line[len-1] = '\0';
                            len--;
                        }
                    }
                }

                if(len == 0)
                    goto getOption;
                
                if(strcmp(line, "c") == 0)
                {
                    printf("Continue\n");
                    continueDebug();
                    continue;
                }
                else if(line[0] == 'b' && line[1] == ' ')
                {
                    addBreakpoint(&line[2]);
                }
                else if(line[0] == 'r' && line[1] == ' ')
                {
                    removeBreakpoint(&line[2]);
                }
                else if(line[0] == 's' && line[1] == ' ')
                {
                    if(len > 2)
                        steps = atoi(&line[2]);
                    else
                        steps = 1;
                    printf("Continue for %d steps\n", steps);
                    continueDebug();
                    continue;
                }
                else
                {
                    printf("Invalid option\n");
                }
                
                goto getOption;
            }
            else
            {
                continueDebug();
            }
        }
        cube_wait(10);
    }    

    return 0;
}