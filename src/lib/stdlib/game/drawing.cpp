#include "drawing.hpp"
#include <SDL.h>
#include <cube/cubeext.h>
#include <map>

using namespace std;

int surfacesCount = 0;
map<int, SurfaceContainer> surfaces;

void freeSurfaces()
{
    for (map<int, SurfaceContainer>::iterator it = surfaces.begin(); it != surfaces.end(); it++)
    {
        if (it->second.type == TEXTURE)
        {
            SDL_DestroyTexture(it->second.texture);
            SDL_FreeSurface(it->second.surface);
        }
    }
    surfaces.clear();
}

SurfaceContainer *getContainer(int id)
{
    if (surfaces.find(id) == surfaces.end())
        return NULL;
    return &surfaces[id];
}

void translateShape(int id, int dx, int dy)
{
    SurfaceContainer *container = getContainer(id);
    if (!container)
        return;

    container->dest.x += dx;
    container->dest.y += dy;
}
void moveShape(int id, int x, int y)
{
    SurfaceContainer *container = getContainer(id);
    if (!container)
        return;

    container->dest.x = x;
    container->dest.y = y;
}

void alphaShape(int id, unsigned char value)
{
    SurfaceContainer *container = getContainer(id);
    if (!container)
        return;

    if (container->type != TEXTURE)
        return;

    SDL_SetTextureAlphaMod(container->texture, value);
}

void rotateShape(int id, double angle)
{
    SurfaceContainer *container = getContainer(id);
    if (!container)
        return;

    container->angle += angle;
}

void rotationShape(int id, double angle)
{
    SurfaceContainer *container = getContainer(id);
    if (!container)
        return;

    container->angle = angle;
}

void flipShape(int id, bool horizontal, bool vertical)
{
    SurfaceContainer *container = getContainer(id);
    if (!container)
        return;

    container->flip = SDL_FLIP_NONE;
    if (horizontal && !vertical)
        container->flip = SDL_FLIP_HORIZONTAL;
    else if (!horizontal && vertical)
        container->flip = SDL_FLIP_VERTICAL;
    else if (horizontal && vertical)
        container->flip = (SDL_RendererFlip)(SDL_FLIP_HORIZONTAL | SDL_FLIP_VERTICAL);
}

void drawShape(int id)
{
    SurfaceContainer *container = getContainer(id);
    if (!container)
        return;

    switch (container->type)
    {
        case TEXTURE:
            SDL_RenderCopyEx(renderer, container->texture, &container->src, &container->dest, container->angle, NULL,
                             container->flip);
            break;
        case RECT:
            SDL_SetRenderDrawColor(renderer, container->color.values.r, container->color.values.g,
                                   container->color.values.b, container->color.values.a);
            SDL_RenderDrawRect(renderer, &container->dest);
            break;
        case RECT_FILL:
            SDL_SetRenderDrawColor(renderer, container->color.values.r, container->color.values.g,
                                   container->color.values.b, container->color.values.a);
            SDL_RenderFillRect(renderer, &container->dest);
            break;
        default:
            break;
    }
}