#ifndef _GAME_DRAWING_HPP_
#define _GAME_DRAWING_HPP_

#include <SDL.h>
#include <map>
#include <vector>

using namespace std;

enum SurfaceType
{
    RECT,
    RECT_FILL,
    TEXTURE,
    GROUP
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
    double angle, scale;
    int w, h;
    int index;
    int rows, cols;
    SDL_RendererFlip flip;
    SDL_Rect src, dest;
    SurfaceColor color;
    vector<int> items;

    bool hasAlpha;
    unsigned char *data;
    int dataSize;

} SurfaceContainer;

extern int surfacesCount;
extern map<int, SurfaceContainer> surfaces;
extern SDL_Renderer *renderer;

void addShape(int parent, int id);
int cloneShape(int id);
void drawShape(int id);
void alphaShape(int id, unsigned char value);
void centerShape(int id);
void leftShape(int id);
void rightShape(int id);
void topShape(int id);
void bottomShape(int id);
void translateShape(int id, int dx, int dy);
void moveShape(int id, int x, int y);
void rotateShape(int id, double angle);
void rotationShape(int id, double angle);
void scaleShape(int id, double factor);
void scaleToShape(int id, double factor);
void flipShape(int id, bool horizontal, bool vertical);
void spriteShape(int id, int rows, int cols);
void nextSpriteShape(int id);
void previousSpriteShape(int id);
void indexSpriteShape(int id, int index);
void tileSpriteShape(int id, int row, int col);
void viewShape(int id, int x, int y, int w, int h);
void setShape(int id, int x, int y, int w, int h);
bool getShape(int id, int *x, int *y, int *w, int *h);
bool dimsShape(int id, int *w, int *h);
bool getPixelsTexture(int id, unsigned int *length, unsigned char **pixels);
bool setPixelsTexture(int id, unsigned int length, unsigned char *pixels);

#endif
