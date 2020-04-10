#include "drawing.hpp"
#include <SDL.h>
#include <cube/cubeext.h>
#include <map>
#include <stdio.h>

using namespace std;

extern SDL_Window *window;

extern "C"
{
    EXPORTED void title(cube_native_var *_title)
    {
        char *title = AS_NATIVE_STRING(_title);
        if (window)
            SDL_SetWindowTitle(window, title);
    }

    EXPORTED void size(cube_native_var *_width, cube_native_var *_height)
    {
        int width = AS_NATIVE_NUMBER(_width);
        int height = AS_NATIVE_NUMBER(_height);
        if (!window)
            return;

        SDL_SetWindowSize(window, width, height);
    }

    EXPORTED cube_native_var *full(cube_native_var *enabled)
    {
        if (!window)
            return NATIVE_BOOL(false);

        int rc = SDL_SetWindowFullscreen(window, AS_NATIVE_BOOL(enabled) ? SDL_WINDOW_FULLSCREEN : 0);
        return NATIVE_BOOL(rc == 0);
    }

    EXPORTED void rgb(cube_native_var *_r, cube_native_var *_g, cube_native_var *_b)
    {
        unsigned char r, g, b;
        r = AS_NATIVE_NUMBER(_r);
        g = AS_NATIVE_NUMBER(_g);
        b = AS_NATIVE_NUMBER(_b);

        if (renderer)
            SDL_SetRenderDrawColor(renderer, r, g, b, 0xFF);
    }

    EXPORTED void rgba(cube_native_var *_r, cube_native_var *_g, cube_native_var *_b, cube_native_var *_a)
    {
        unsigned char r, g, b, a;
        r = AS_NATIVE_NUMBER(_r);
        g = AS_NATIVE_NUMBER(_g);
        b = AS_NATIVE_NUMBER(_b);
        a = AS_NATIVE_NUMBER(_a);

        if (renderer)
            SDL_SetRenderDrawColor(renderer, r, g, b, a);
    }

    EXPORTED void update()
    {
        if (renderer)
            SDL_RenderPresent(renderer);
    }

    EXPORTED void clear()
    {
        if (renderer)
            SDL_RenderClear(renderer);
    }

    EXPORTED void translate(cube_native_var *id, cube_native_var *dx, cube_native_var *dy)
    {
        if (!renderer)
            return;
        translateShape(AS_NATIVE_NUMBER(id), AS_NATIVE_NUMBER(dx), AS_NATIVE_NUMBER(dy));
    }

    EXPORTED void move(cube_native_var *id, cube_native_var *x, cube_native_var *y)
    {
        if (!renderer)
            return;
        moveShape(AS_NATIVE_NUMBER(id), AS_NATIVE_NUMBER(x), AS_NATIVE_NUMBER(y));
    }

    EXPORTED void rotate(cube_native_var *id, cube_native_var *angle)
    {
        if (!renderer)
            return;
        rotateShape(AS_NATIVE_NUMBER(id), AS_NATIVE_NUMBER(angle));
    }

    EXPORTED void rotation(cube_native_var *id, cube_native_var *angle)
    {
        if (!renderer)
            return;
        rotationShape(AS_NATIVE_NUMBER(id), AS_NATIVE_NUMBER(angle));
    }

    EXPORTED void flip(cube_native_var *id, cube_native_var *horizontal, cube_native_var *vertical)
    {
        if (!renderer)
            return;
        flipShape(AS_NATIVE_NUMBER(id), AS_NATIVE_BOOL(horizontal), AS_NATIVE_BOOL(vertical));
    }

    EXPORTED void draw(cube_native_var *id)
    {
        if (!renderer)
            return;
        drawShape(AS_NATIVE_NUMBER(id));
    }
}