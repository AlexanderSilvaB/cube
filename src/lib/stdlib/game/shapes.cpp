#include "drawing.hpp"
#include <SDL.h>
#include <cube/cubeext.h>
#include <map>

using namespace std;

extern "C"
{
    EXPORTED cube_native_var *border(cube_native_var *_w, cube_native_var *_h)
    {
        int w = AS_NATIVE_NUMBER(_w);
        int h = AS_NATIVE_NUMBER(_h);

        SurfaceContainer container;
        container.type = RECT;
        container.dest = {0, 0, w, h};
        container.surface = NULL;
        container.color.rgba = 0xFF000000;
        container.texture = NULL;
        container.angle = 0;
        container.scale = 1.0f;
        container.w = w;
        container.h = h;
        container.index = 0;
        container.rows = 1;
        container.cols = 1;
        container.flip = SDL_FLIP_NONE;

        surfaces[surfacesCount] = container;
        cube_native_var *res = NATIVE_NUMBER(surfacesCount);

        surfacesCount++;
        return res;
    }

    EXPORTED cube_native_var *fill(cube_native_var *_w, cube_native_var *_h)
    {
        int w = AS_NATIVE_NUMBER(_w);
        int h = AS_NATIVE_NUMBER(_h);

        SurfaceContainer container;
        container.type = RECT_FILL;
        container.dest = {0, 0, w, h};
        container.surface = NULL;
        container.color.rgba = 0xFF000000;
        container.texture = NULL;
        container.angle = 0;
        container.w = w;
        container.h = h;
        container.index = 0;
        container.rows = 1;
        container.cols = 1;
        container.scale = 1.0f;
        container.flip = SDL_FLIP_NONE;

        surfaces[surfacesCount] = container;
        cube_native_var *res = NATIVE_NUMBER(surfacesCount);

        surfacesCount++;
        return res;
    }
}