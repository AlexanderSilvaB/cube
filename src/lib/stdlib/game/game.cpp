#include <SDL.h>
#include <cube/cubeext.h>
#include <map>
#include <stdio.h>

using namespace std;

// The window we'll be rendering to
SDL_Window *window = NULL;
// The surface contained by the window
SDL_Renderer *renderer = NULL;

extern void freeSurfaces();

bool createRenderer()
{
    // Create renderer for window
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (renderer == NULL)
    {
        printf("Renderer could not be created! SDL Error: %s\n", SDL_GetError());
        return false;
    }

    // Initialize renderer color
    SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);

    // // Initialize PNG loading
    // int imgFlags = IMG_INIT_PNG;
    // if (!(IMG_Init(imgFlags) & imgFlags))
    // {
    //     printf("SDL_image could not initialize! SDL_image Error: %s\n", IMG_GetError());
    // }
    return true;
}

extern "C"
{
    EXPORTED void cube_init()
    {
        // Initialize SDL
        if (SDL_Init(SDL_INIT_VIDEO) < 0)
        {
            printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
            return;
        }

        // Create window
        window = SDL_CreateWindow("", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 600, 480, SDL_WINDOW_SHOWN);
        if (window == NULL)
        {
            printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
            return;
        }

        if (!createRenderer())
        {
            return;
        }
    }

    EXPORTED void cube_release()
    {
        freeSurfaces();

        if (renderer)
            SDL_DestroyRenderer(renderer);
        renderer = NULL;

        // Destroy window
        if (window)
            SDL_DestroyWindow(window);
        window = NULL;

        // Quit SDL subsystems
        SDL_Quit();
    }

    EXPORTED cube_native_var *valid()
    {
        return NATIVE_BOOL(window != NULL && renderer != NULL);
    }

    EXPORTED cube_native_var *width()
    {
        if (!window)
            return NATIVE_NUMBER(0);
        int w;
        SDL_GetWindowSize(window, &w, NULL);
        return NATIVE_NUMBER(w);
    }

    EXPORTED cube_native_var *height()
    {
        if (!window)
            return NATIVE_NUMBER(0);
        int h;
        SDL_GetWindowSize(window, NULL, &h);
        return NATIVE_NUMBER(h);
    }

    EXPORTED void delay(cube_native_var *_ms)
    {
        int ms = AS_NATIVE_NUMBER(_ms);
        if (window)
            SDL_Delay(ms);
    }
}