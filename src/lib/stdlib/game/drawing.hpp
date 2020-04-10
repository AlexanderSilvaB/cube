#ifndef _GAME_DRAWING_HPP_
#define _GAME_DRAWING_HPP_

#include <SDL.h>
#include <map>

using namespace std;

enum SurfaceType
{
    RECT,
    RECT_FILL,
    TEXTURE
};

typedef union {
    struct
    {
        unsigned char r, g, b, a;
    } values;
    unsigned int rgba;
} SurfaceColor;

// The surfaces container
typedef struct
{
    SurfaceType type;
    SDL_Surface *surface;
    SDL_Texture *texture;
    double angle;
    SDL_RendererFlip flip;
    SDL_Rect src, dest;
    SurfaceColor color;
} SurfaceContainer;

extern int surfacesCount;
extern map<int, SurfaceContainer> surfaces;
extern SDL_Renderer *renderer;

void drawShape(int id);
void alphaShape(int id, unsigned char value);
void translateShape(int id, int dx, int dy);
void moveShape(int id, int x, int y);
void rotateShape(int id, double angle);
void rotationShape(int id, double angle);
void flipShape(int id, bool horizontal, bool vertical);

#endif
