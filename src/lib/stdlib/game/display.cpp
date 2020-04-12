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

    EXPORTED cube_native_var *group()
    {
        if (!renderer)
            return NATIVE_NULL();

        SurfaceContainer container;
        container.type = GROUP;
        container.dest = {0, 0, 0, 0};
        container.surface = NULL;
        container.color.rgba = 0xFF000000;
        container.texture = NULL;
        container.angle = 0;
        container.scale = 1.0f;
        container.w = 0;
        container.h = 0;
        container.index = 0;
        container.rows = 1;
        container.cols = 1;
        container.flip = SDL_FLIP_NONE;

        surfaces[surfacesCount] = container;
        cube_native_var *res = NATIVE_NUMBER(surfacesCount);

        surfacesCount++;
        return res;
    }

    EXPORTED void add(cube_native_var *parent, cube_native_var *id)
    {
        if (!renderer)
            return;
        addShape(AS_NATIVE_NUMBER(parent), AS_NATIVE_NUMBER(id));
    }

    EXPORTED cube_native_var *copy(cube_native_var *id)
    {
        cube_native_var *res = NATIVE_NULL();
        if (!renderer)
            return res;

        int nid = cloneShape(AS_NATIVE_NUMBER(id));
        if (nid >= 0)
            TO_NATIVE_NUMBER(res, nid);

        return res;
    }

    EXPORTED void center(cube_native_var *id)
    {
        if (!renderer)
            return;
        centerShape(AS_NATIVE_NUMBER(id));
    }

    EXPORTED void left(cube_native_var *id)
    {
        if (!renderer)
            return;
        leftShape(AS_NATIVE_NUMBER(id));
    }

    EXPORTED void right(cube_native_var *id)
    {
        if (!renderer)
            return;
        rightShape(AS_NATIVE_NUMBER(id));
    }

    EXPORTED void top(cube_native_var *id)
    {
        if (!renderer)
            return;
        topShape(AS_NATIVE_NUMBER(id));
    }

    EXPORTED void bottom(cube_native_var *id)
    {
        if (!renderer)
            return;
        bottomShape(AS_NATIVE_NUMBER(id));
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

    EXPORTED void rotateTo(cube_native_var *id, cube_native_var *angle)
    {
        if (!renderer)
            return;
        rotationShape(AS_NATIVE_NUMBER(id), AS_NATIVE_NUMBER(angle));
    }

    EXPORTED void scale(cube_native_var *id, cube_native_var *factor)
    {
        if (!renderer)
            return;
        scaleShape(AS_NATIVE_NUMBER(id), AS_NATIVE_NUMBER(factor));
    }

    EXPORTED void scaleTo(cube_native_var *id, cube_native_var *factor)
    {
        if (!renderer)
            return;
        scaleToShape(AS_NATIVE_NUMBER(id), AS_NATIVE_NUMBER(factor));
    }

    EXPORTED void flip(cube_native_var *id, cube_native_var *horizontal, cube_native_var *vertical)
    {
        if (!renderer)
            return;
        flipShape(AS_NATIVE_NUMBER(id), AS_NATIVE_BOOL(horizontal), AS_NATIVE_BOOL(vertical));
    }

    EXPORTED void view(cube_native_var *id, cube_native_var *x, cube_native_var *y, cube_native_var *w,
                       cube_native_var *h)
    {
        if (!renderer)
            return;
        viewShape(AS_NATIVE_NUMBER(id), AS_NATIVE_NUMBER(x), AS_NATIVE_NUMBER(y), AS_NATIVE_NUMBER(w),
                  AS_NATIVE_NUMBER(h));
    }

    EXPORTED void set(cube_native_var *id, cube_native_var *x, cube_native_var *y, cube_native_var *w,
                      cube_native_var *h)
    {
        if (!renderer)
            return;
        setShape(AS_NATIVE_NUMBER(id), AS_NATIVE_NUMBER(x), AS_NATIVE_NUMBER(y), AS_NATIVE_NUMBER(w),
                 AS_NATIVE_NUMBER(h));
    }

    EXPORTED cube_native_var *rect(cube_native_var *id)
    {
        if (!renderer)
            return NATIVE_NULL();

        int x, y, w, h;
        if (!getShape(AS_NATIVE_NUMBER(id), &x, &y, &w, &h))
            return NATIVE_NULL();

        cube_native_var *res = NATIVE_LIST();
        ADD_NATIVE_LIST(res, NATIVE_NUMBER(x));
        ADD_NATIVE_LIST(res, NATIVE_NUMBER(y));
        ADD_NATIVE_LIST(res, NATIVE_NUMBER(w));
        ADD_NATIVE_LIST(res, NATIVE_NUMBER(h));
        return res;
    }

    EXPORTED cube_native_var *dims(cube_native_var *id)
    {
        if (!renderer)
            return NATIVE_NULL();

        int w, h;
        if (!dimsShape(AS_NATIVE_NUMBER(id), &w, &h))
            return NATIVE_NULL();

        cube_native_var *res = NATIVE_LIST();
        ADD_NATIVE_LIST(res, NATIVE_NUMBER(w));
        ADD_NATIVE_LIST(res, NATIVE_NUMBER(h));
        return res;
    }

    EXPORTED void draw(cube_native_var *id)
    {
        if (!renderer)
            return;
        drawShape(AS_NATIVE_NUMBER(id));
    }
}