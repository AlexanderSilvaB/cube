#include "drawing.hpp"
#include <SDL.h>
#include <cube/cubeext.h>
#include <map>

using namespace std;

extern "C"
{
    EXPORTED cube_native_var *load(cube_native_var *_path)
    {
        char *path = AS_NATIVE_STRING(_path);

        // SDL_Surface *surface = IMG_Load(path);
        SDL_Surface *surface = SDL_LoadBMP(path);

        if (surface == NULL)
            return NATIVE_NULL();

        SurfaceContainer container;
        container.type = TEXTURE;
        container.dest = {0, 0, surface->w, surface->h};
        container.src = {0, 0, surface->w, surface->h};
        container.surface = surface;
        container.color.rgba = 0xFF000000;
        container.angle = 0;
        container.flip = SDL_FLIP_NONE;
        container.texture = SDL_CreateTextureFromSurface(renderer, surface);
        if (container.texture == NULL)
        {
            printf("Unable to create texture from %s! SDL Error: %s\n", path, SDL_GetError());
            SDL_FreeSurface(surface);
            return NATIVE_NULL();
        }

        SDL_SetTextureBlendMode(container.texture, SDL_BLENDMODE_BLEND);
        SDL_SetTextureAlphaMod(container.texture, 255);

        surfaces[surfacesCount] = container;
        cube_native_var *res = NATIVE_NUMBER(surfacesCount);

        surfacesCount++;
        return res;
    }

    EXPORTED void alpha(cube_native_var *id, cube_native_var *alpha)
    {
        if (!renderer)
            return;
        alphaShape(AS_NATIVE_NUMBER(id), AS_NATIVE_NUMBER(alpha));
    }
}