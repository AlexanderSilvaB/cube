#include "drawing.hpp"
#include <SDL.h>
#include <cube/cubeext.h>
#include <map>

using namespace std;

int surfacesCount = 0;
map<int, SurfaceContainer> surfaces;
extern SDL_Window *window;

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

void addShape(int parent, int id)
{
    SurfaceContainer *container = getContainer(parent);
    if (!container)
        return;

    SurfaceContainer *child = getContainer(id);
    if (!child)
        return;

    if (container->type != GROUP)
        return;

    container->items.push_back(id);
}

int cloneShape(int id)
{
    SurfaceContainer *container = getContainer(id);
    if (!container)
        return -1;

    SurfaceContainer clone;
    clone.type = container->type;
    clone.scale = container->scale;
    clone.dataSize = container->dataSize;
    clone.w = container->w;
    clone.h = container->h;
    clone.dest = container->dest;
    clone.src = container->src;
    clone.surface = container->surface;
    clone.color = container->color;
    clone.angle = container->angle;
    clone.index = container->index;
    clone.rows = container->rows;
    clone.cols = container->cols;
    clone.flip = container->flip;
    clone.texture = container->texture;
    clone.data = container->data;
    clone.hasAlpha = container->hasAlpha;
    for (int i = 0; i < container->items.size(); i++)
        clone.items.push_back(cloneShape(container->items[i]));

    int nid = surfacesCount;
    surfaces[nid] = clone;
    surfacesCount++;
    return nid;
}

void centerShape(int id)
{
    SurfaceContainer *container = getContainer(id);
    if (!container)
        return;

    if (container->type == GROUP)
    {
        for (int i = 0; i < container->items.size(); i++)
            centerShape(container->items[i]);
        return;
    }

    int w, h;
    SDL_GetWindowSize(window, &w, &h);

    int x = (w - container->dest.w) / 2;
    int y = (h - container->dest.h) / 2;
    container->dest.x = x;
    container->dest.y = y;
}

void leftShape(int id)
{
    SurfaceContainer *container = getContainer(id);
    if (!container)
        return;

    if (container->type == GROUP)
    {
        for (int i = 0; i < container->items.size(); i++)
            leftShape(container->items[i]);
        return;
    }

    container->dest.x = 0;
}

void rightShape(int id)
{
    SurfaceContainer *container = getContainer(id);
    if (!container)
        return;

    if (container->type == GROUP)
    {
        for (int i = 0; i < container->items.size(); i++)
            rightShape(container->items[i]);
        return;
    }

    int w;
    SDL_GetWindowSize(window, &w, NULL);

    int x = w - container->dest.w;
    container->dest.x = x;
}

void topShape(int id)
{
    SurfaceContainer *container = getContainer(id);
    if (!container)
        return;

    if (container->type == GROUP)
    {
        for (int i = 0; i < container->items.size(); i++)
            topShape(container->items[i]);
        return;
    }

    container->dest.y = 0;
}

void bottomShape(int id)
{
    SurfaceContainer *container = getContainer(id);
    if (!container)
        return;

    if (container->type == GROUP)
    {
        for (int i = 0; i < container->items.size(); i++)
            bottomShape(container->items[i]);
        return;
    }

    int h;
    SDL_GetWindowSize(window, NULL, &h);

    int y = h - container->dest.h;
    container->dest.y = y;
}

void translateShape(int id, int dx, int dy)
{
    SurfaceContainer *container = getContainer(id);
    if (!container)
        return;

    if (container->type == GROUP)
    {
        for (int i = 0; i < container->items.size(); i++)
            translateShape(container->items[i], dx, dy);
        return;
    }

    container->dest.x += dx;
    container->dest.y += dy;
}
void moveShape(int id, int x, int y)
{
    SurfaceContainer *container = getContainer(id);
    if (!container)
        return;

    if (container->type == GROUP)
    {
        for (int i = 0; i < container->items.size(); i++)
            moveShape(container->items[i], x, y);
        return;
    }

    container->dest.x = x;
    container->dest.y = y;
}

void alphaShape(int id, unsigned char value)
{
    SurfaceContainer *container = getContainer(id);
    if (!container)
        return;

    if (container->type == GROUP)
    {
        for (int i = 0; i < container->items.size(); i++)
            alphaShape(container->items[i], value);
        return;
    }

    if (container->type != TEXTURE)
        return;

    SDL_SetTextureAlphaMod(container->texture, value);
}

void rotateShape(int id, double angle)
{
    SurfaceContainer *container = getContainer(id);
    if (!container)
        return;

    if (container->type == GROUP)
    {
        for (int i = 0; i < container->items.size(); i++)
            rotateShape(container->items[i], angle);
        return;
    }

    container->angle += angle;
}

void rotationShape(int id, double angle)
{
    SurfaceContainer *container = getContainer(id);
    if (!container)
        return;

    if (container->type == GROUP)
    {
        for (int i = 0; i < container->items.size(); i++)
            rotationShape(container->items[i], angle);
        return;
    }

    container->angle = angle;
}

void scaleShape(int id, double factor)
{
    SurfaceContainer *container = getContainer(id);
    if (!container)
        return;

    if (container->type == GROUP)
    {
        for (int i = 0; i < container->items.size(); i++)
            scaleShape(container->items[i], factor);
        return;
    }

    container->scale += factor;
}

void scaleToShape(int id, double factor)
{
    SurfaceContainer *container = getContainer(id);
    if (!container)
        return;

    if (container->type == GROUP)
    {
        for (int i = 0; i < container->items.size(); i++)
            scaleToShape(container->items[i], factor);
        return;
    }

    container->scale = factor;
}

void flipShape(int id, bool horizontal, bool vertical)
{
    SurfaceContainer *container = getContainer(id);
    if (!container)
        return;

    if (container->type == GROUP)
    {
        for (int i = 0; i < container->items.size(); i++)
            flipShape(container->items[i], horizontal, vertical);
        return;
    }

    container->flip = SDL_FLIP_NONE;
    if (horizontal && !vertical)
        container->flip = SDL_FLIP_HORIZONTAL;
    else if (!horizontal && vertical)
        container->flip = SDL_FLIP_VERTICAL;
    else if (horizontal && vertical)
        container->flip = (SDL_RendererFlip)(SDL_FLIP_HORIZONTAL | SDL_FLIP_VERTICAL);
}

void spriteShape(int id, int rows, int cols)
{
    SurfaceContainer *container = getContainer(id);
    if (!container)
        return;

    if (container->type == GROUP)
    {
        for (int i = 0; i < container->items.size(); i++)
            spriteShape(container->items[i], rows, cols);
        return;
    }

    if (container->type != TEXTURE)
        return;

    if (rows <= 0 || cols <= 0)
        return;

    int w = container->w / cols;
    int h = container->h / rows;
    container->src.w = w;
    container->src.h = h;
    container->index = 0;
    container->cols = cols;
    container->rows = rows;

    int x = container->dest.w - w;
    int y = container->dest.h - h;

    container->dest.x += x / 2;
    container->dest.y += y / 2;
    container->dest.w = w;
    container->dest.h = h;
}

void nextSpriteShape(int id)
{
    SurfaceContainer *container = getContainer(id);
    if (!container)
        return;

    if (container->type == GROUP)
    {
        for (int i = 0; i < container->items.size(); i++)
            nextSpriteShape(container->items[i]);
        return;
    }

    if (container->type != TEXTURE)
        return;

    int index = container->index + 1;
    int row = index / container->cols;

    if (row >= container->rows)
    {
        index = 0;
        row = 0;
    }

    int col = index - row * container->cols;

    container->index = index;
    container->src.x = col * container->src.w;
    container->src.y = row * container->src.h;
}

void previousSpriteShape(int id)
{
    SurfaceContainer *container = getContainer(id);
    if (!container)
        return;

    if (container->type == GROUP)
    {
        for (int i = 0; i < container->items.size(); i++)
            previousSpriteShape(container->items[i]);
        return;
    }

    if (container->type != TEXTURE)
        return;

    int index = container->index - 1;
    if (index < 0)
    {
        index = container->cols * container->rows - 1;
    }

    int row = index / container->cols;
    int col = index - row * container->cols;

    container->index = index;
    container->src.x = col * container->src.w;
    container->src.y = row * container->src.h;
}

void indexSpriteShape(int id, int index)
{
    SurfaceContainer *container = getContainer(id);
    if (!container)
        return;

    if (container->type == GROUP)
    {
        for (int i = 0; i < container->items.size(); i++)
            indexSpriteShape(container->items[i], index);
        return;
    }

    if (container->type != TEXTURE)
        return;

    if (index < 0)
        return;

    if (index > container->cols * container->rows - 1)
        return;

    int row = index / container->cols;
    int col = index - row * container->cols;

    container->index = index;
    container->src.x = col * container->src.w;
    container->src.y = row * container->src.h;
}

void tileSpriteShape(int id, int row, int col)
{
    SurfaceContainer *container = getContainer(id);
    if (!container)
        return;

    if (container->type == GROUP)
    {
        for (int i = 0; i < container->items.size(); i++)
            tileSpriteShape(container->items[i], row, col);
        return;
    }

    if (container->type != TEXTURE)
        return;

    if (row < 0 || row >= container->rows)
        return;

    if (col < 0 || col >= container->cols)
        return;

    int index = row * container->cols + col;

    container->index = index;
    container->src.x = col * container->src.w;
    container->src.y = row * container->src.h;
}

void viewShape(int id, int x, int y, int w, int h)
{
    SurfaceContainer *container = getContainer(id);
    if (!container)
        return;

    if (container->type == GROUP)
    {
        for (int i = 0; i < container->items.size(); i++)
            viewShape(container->items[i], x, y, w, h);
        return;
    }

    if (container->type != TEXTURE)
        return;

    if (x < 0)
        x = x % container->w;

    if (y < 0)
        y = y % container->h;

    if (x >= container->w)
        x = x % container->w;

    if (y >= container->h)
        y = y % container->h;

    if (w <= 0)
        w = container->w;

    if (h <= 0)
        h = container->h;

    if (x + w > container->w)
        w = container->w - x;

    if (y + h > container->h)
        h = container->h - y;

    container->src.x = x;
    container->src.y = y;
    container->src.w = w;
    container->src.h = h;
}

void setShape(int id, int x, int y, int w, int h)
{
    SurfaceContainer *container = getContainer(id);
    if (!container)
        return;

    if (container->type == GROUP)
    {
        for (int i = 0; i < container->items.size(); i++)
            setShape(container->items[i], x, y, w, h);
        return;
    }

    if (w < 0)
        w = container->w;

    if (h < 0)
        h = container->h;

    container->dest.x = x;
    container->dest.y = y;
    container->dest.w = w;
    container->dest.h = h;
}

bool getShape(int id, int *x, int *y, int *w, int *h)
{
    SurfaceContainer *container = getContainer(id);
    if (!container)
        return false;

    *x = container->dest.x;
    *y = container->dest.y;
    *w = container->dest.w;
    *h = container->dest.h;
    return true;
}

bool dimsShape(int id, int *w, int *h)
{
    SurfaceContainer *container = getContainer(id);
    if (!container)
        return false;

    *w = container->w;
    *h = container->h;
    return true;
}

void drawShape(int id)
{
    SurfaceContainer *container = getContainer(id);
    if (!container)
        return;

    if (container->type == GROUP)
    {
        for (int i = 0; i < container->items.size(); i++)
            drawShape(container->items[i]);
        return;
    }

    int W = container->dest.w;
    int H = container->dest.h;
    int w = container->dest.w * container->scale;
    int h = container->dest.h * container->scale;
    int dx = (W - w) / 2;
    int dy = (H - h) / 2;

    container->dest.x += dx;
    container->dest.y += dy;
    container->dest.w = w;
    container->dest.h = h;

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

    container->dest.x -= dx;
    container->dest.y -= dy;
    container->dest.w = W;
    container->dest.h = H;
}

bool getPixelsTexture(int id, unsigned int *length, unsigned char **pixels)
{
    SurfaceContainer *container = getContainer(id);
    if (!container)
        return false;

    if (container->type != TEXTURE)
        return false;

    *length = container->dataSize;
    *pixels = (unsigned char *)malloc(sizeof(unsigned char) * container->dataSize);
    memcpy(*pixels, container->data, container->dataSize);
    return true;
}

bool setPixelsTexture(int id, unsigned int length, unsigned char *pixels)
{
    SurfaceContainer *container = getContainer(id);
    if (!container)
        return false;

    if (container->type != TEXTURE)
        return false;

    if (container->dataSize != length)
        return false;

    memcpy(container->data, pixels, length);

    SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, container->surface);

    if (tex == NULL)
        return false;

    SDL_DestroyTexture(container->texture);
    container->texture = tex;

    SDL_SetTextureBlendMode(container->texture, SDL_BLENDMODE_BLEND);
    SDL_SetTextureAlphaMod(container->texture, 255);

    return true;
}

unsigned char *getPixelsTextureRaw(int id)
{
    SurfaceContainer *container = getContainer(id);
    if (!container)
        return NULL;

    if (container->type != TEXTURE)
        return NULL;

    return container->data;
}

void blitTexture(int id)
{
    SurfaceContainer *container = getContainer(id);
    if (!container)
        return;

    if (container->type != TEXTURE)
        return;

    SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, container->surface);

    if (tex == NULL)
        return;

    SDL_DestroyTexture(container->texture);
    container->texture = tex;

    SDL_SetTextureBlendMode(container->texture, SDL_BLENDMODE_BLEND);
    SDL_SetTextureAlphaMod(container->texture, 255);
}