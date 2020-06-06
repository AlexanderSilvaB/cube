#include "sciplot/sciplot.hpp"
#include <cube/cubeext.h>
#include <vector>

using namespace sciplot;
using namespace std;

vector<plot *> plots;

extern "C"
{
    EXPORTED void cube_release()
    {
        for (int i = 0; i < plots.size(); i++)
            delete plots[i];
    }

    EXPORTED int create()
    {
        plots.push_back(new plot());
        return plots.size() - 1;
    }

    EXPORTED void save(int index, char *fileName)
    {
        if (index < 0 || index > plots.size())
            return;
        plots[index]->save(fileName);
    }

    EXPORTED void show(int index)
    {
        if (index < 0 || index > plots.size())
            return;
        plots[index]->show();
    }

    EXPORTED void pallete(int index, char *name)
    {
        if (index < 0 || index > plots.size())
            return;
        plots[index]->pallete(name);
    }

    EXPORTED void size(int index, unsigned int width, unsigned int height)
    {
        if (index < 0 || index > plots.size())
            return;
        plots[index]->size(width, height);
    }

    EXPORTED void xlabel(int index, char *name)
    {
        if (index < 0 || index > plots.size())
            return;
        plots[index]->xlabel(name);
    }

    EXPORTED void ylabel(int index, char *name)
    {
        if (index < 0 || index > plots.size())
            return;
        plots[index]->ylabel(name);
    }

    EXPORTED void xrange(int index, double min, double max)
    {
        if (index < 0 || index > plots.size())
            return;
        plots[index]->xrange(min, max);
    }

    EXPORTED void yrange(int index, double min, double max)
    {
        if (index < 0 || index > plots.size())
            return;
        plots[index]->yrange(min, max);
    }

    EXPORTED int grid(int index)
    {
        if (index < 0 || index > plots.size())
            return -1;

        int ret = *((int *)&(plots[index]->grid()));
        return ret;
    }

    EXPORTED int border(int index)
    {
        if (index < 0 || index > plots.size())
            return -1;

        int ret = *((int *)&(plots[index]->border()));
        return ret;
    }

    EXPORTED int tics(int index)
    {
        if (index < 0 || index > plots.size())
            return -1;

        int ret = *((int *)&(plots[index]->tics()));
        return ret;
    }

    EXPORTED int legend(int index)
    {
        if (index < 0 || index > plots.size())
            return -1;

        int ret = *((int *)&(plots[index]->legend()));
        return ret;
    }

    EXPORTED int draw(int index, cube_native_var *_x, cube_native_var *_y)
    {
        if (index < 0 || index > plots.size())
            return -1;

        if (_x->size != _y->size)
            return -1;

        vec x(_x->size), y(_x->size);
        for (int i = 0; i < _x->size; i++)
        {
            x[i] = AS_NATIVE_NUMBER(_x->list[i]);
            y[i] = AS_NATIVE_NUMBER(_y->list[i]);
        }

        int ret = *((int *)&(plots[index]->draw(x, y)));
        return ret;
    }
}