#include "drawing.hpp"
#include <SDL.h>
#include <cube/cubeext.h>
#include <map>

using namespace std;

Uint32 getpixel(SDL_Surface *surface, int x, int y)
{
    int bpp = surface->format->BytesPerPixel;
    /* Here p is the address to the pixel we want to retrieve */
    Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

    switch (bpp)
    {
        case 1:
            return *p;
            break;

        case 2:
            return *(Uint16 *)p;
            break;

        case 3:
            if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
                return p[0] << 16 | p[1] << 8 | p[2];
            else
                return p[0] | p[1] << 8 | p[2] << 16;
            break;

        case 4:
            return *(Uint32 *)p;
            break;

        default:
            return 0; /* shouldn't happen, but avoids warnings */
    }
}

extern "C"
{
    EXPORTED cube_native_var *load(cube_native_var *_path)
    {
        char *path = AS_NATIVE_STRING(_path);

        // SDL_Surface *surface = IMG_Load(path);
        SDL_Surface *surface = SDL_LoadBMP(path);

        if (surface == NULL)
            return NATIVE_NULL();

        Uint32 pixel = getpixel(surface, 0, 0);
        unsigned char r, g, b;
        SDL_GetRGB(pixel, surface->format, &r, &g, &b);
        Uint32 key = SDL_MapRGB(surface->format, r, g, b);
        SDL_SetColorKey(surface, SDL_TRUE, key);

        SurfaceContainer container;
        container.type = TEXTURE;
        container.scale = 1.0f;
        container.w = surface->w;
        container.h = surface->h;
        container.dest = {0, 0, surface->w, surface->h};
        container.src = {0, 0, surface->w, surface->h};
        container.surface = surface;
        container.color.rgba = 0xFF000000;
        container.angle = 0;
        container.index = 0;
        container.rows = 1;
        container.cols = 1;
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

    EXPORTED cube_native_var *loadColorKey(cube_native_var *_path, cube_native_var *_key)
    {
        if (!IS_NATIVE_LIST(_key))
            return NATIVE_NULL();

        if (_key->size == 0)
            return NATIVE_NULL();

        char *path = AS_NATIVE_STRING(_path);

        // SDL_Surface *surface = IMG_Load(path);
        SDL_Surface *surface = SDL_LoadBMP(path);

        if (surface == NULL)
            return NATIVE_NULL();

        unsigned char r = AS_NATIVE_NUMBER(_key->list[0]);
        unsigned char g = r;
        if (_key->size > 1)
            g = AS_NATIVE_NUMBER(_key->list[1]);
        unsigned char b = g;
        if (_key->size > 2)
            b = AS_NATIVE_NUMBER(_key->list[2]);

        Uint32 key = SDL_MapRGB(surface->format, r, g, b);
        SDL_SetColorKey(surface, SDL_TRUE, key);

        SurfaceContainer container;
        container.type = TEXTURE;
        container.scale = 1.0f;
        container.w = surface->w;
        container.h = surface->h;
        container.dest = {0, 0, surface->w, surface->h};
        container.src = {0, 0, surface->w, surface->h};
        container.surface = surface;
        container.color.rgba = 0xFF000000;
        container.angle = 0;
        container.index = 0;
        container.rows = 1;
        container.cols = 1;
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

    EXPORTED cube_native_var *loadRaw(cube_native_var *_path)
    {
        char *path = AS_NATIVE_STRING(_path);

        // SDL_Surface *surface = IMG_Load(path);
        SDL_Surface *surface = SDL_LoadBMP(path);

        if (surface == NULL)
            return NATIVE_NULL();

        SurfaceContainer container;
        container.type = TEXTURE;
        container.scale = 1.0f;
        container.w = surface->w;
        container.h = surface->h;
        container.dest = {0, 0, surface->w, surface->h};
        container.src = {0, 0, surface->w, surface->h};
        container.surface = surface;
        container.color.rgba = 0xFF000000;
        container.angle = 0;
        container.index = 0;
        container.rows = 1;
        container.cols = 1;
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

    EXPORTED void sprite(cube_native_var *id, cube_native_var *rows, cube_native_var *cols)
    {
        if (!renderer)
            return;
        spriteShape(AS_NATIVE_NUMBER(id), AS_NATIVE_NUMBER(rows), AS_NATIVE_NUMBER(cols));
    }

    EXPORTED void next(cube_native_var *id)
    {
        if (!renderer)
            return;
        nextSpriteShape(AS_NATIVE_NUMBER(id));
    }

    EXPORTED void previous(cube_native_var *id)
    {
        if (!renderer)
            return;
        previousSpriteShape(AS_NATIVE_NUMBER(id));
    }

    EXPORTED void index(cube_native_var *id, cube_native_var *i)
    {
        if (!renderer)
            return;
        indexSpriteShape(AS_NATIVE_NUMBER(id), AS_NATIVE_NUMBER(i));
    }

    EXPORTED void tile(cube_native_var *id, cube_native_var *row, cube_native_var *col)
    {
        if (!renderer)
            return;
        tileSpriteShape(AS_NATIVE_NUMBER(id), AS_NATIVE_NUMBER(row), AS_NATIVE_NUMBER(col));
    }
}