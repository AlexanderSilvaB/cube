#include "sciplot/sciplot.hpp"
#include <cube/cubeext.h>
#include <vector>

using namespace sciplot;
using namespace std;

vector<plot *> plots;
vector<plotspecs *> draws;

extern "C"
{
    // Cube
    EXPORTED void cube_release()
    {
        draws.clear();
        for (int i = 0; i < plots.size(); i++)
            delete plots[i];
    }

    // Plot
    EXPORTED int create_plot()
    {
        plots.push_back(new plot());
        return plots.size() - 1;
    }

    EXPORTED void save_plot(int index, char *fileName)
    {
        if (index < 0 || index > plots.size())
            return;
        plots[index]->save(fileName);
    }

    EXPORTED void show_plot(int index)
    {
        if (index < 0 || index > plots.size())
            return;
        plots[index]->show();
    }

    EXPORTED void pallete_plot(int index, char *name)
    {
        if (index < 0 || index > plots.size())
            return;
        plots[index]->pallete(name);
    }

    EXPORTED void size_plot(int index, unsigned int width, unsigned int height)
    {
        if (index < 0 || index > plots.size())
            return;
        plots[index]->size(width, height);
    }

    EXPORTED void xlabel_plot(int index, char *name)
    {
        if (index < 0 || index > plots.size())
            return;
        plots[index]->xlabel(name);
    }

    EXPORTED void ylabel_plot(int index, char *name)
    {
        if (index < 0 || index > plots.size())
            return;
        plots[index]->ylabel(name);
    }

    EXPORTED void xrange_plot(int index, double min, double max)
    {
        if (index < 0 || index > plots.size())
            return;
        plots[index]->xrange(min, max);
    }

    EXPORTED void yrange_plot(int index, double min, double max)
    {
        if (index < 0 || index > plots.size())
            return;
        plots[index]->yrange(min, max);
    }

    EXPORTED void grid_plot(int index, bool enabled)
    {
        if (index < 0 || index > plots.size())
            return;

        plots[index]->grid().show(enabled);
    }

    EXPORTED void border_plot(int index, bool left, bool top, bool right, bool bottom)
    {
        if (index < 0 || index > plots.size())
            return;

        plots[index]->border().clear();
        if (left)
            plots[index]->border().left();
        if (top)
            plots[index]->border().top();
        if (right)
            plots[index]->border().right();
        if (bottom)
            plots[index]->border().bottom();
    }

    EXPORTED void tics_plot(int index)
    {
        if (index < 0 || index > plots.size())
            return;

        plots[index]->tics();
    }

    EXPORTED void legend_plot(int index)
    {
        if (index < 0 || index > plots.size())
            return;

        plots[index]->legend();
    }

    EXPORTED void samples_plot(int index, unsigned int samples)
    {
        if (index < 0 || index > plots.size())
            return;
        plots[index]->samples(samples);
    }

    EXPORTED int draw_plot(int index, cube_native_var *_x, cube_native_var *_y)
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

        plotspecs &draw = plots[index]->draw(x, y);
        draws.push_back(&draw);
        return draws.size() - 1;
    }

    // Draws
    EXPORTED void title_draw(int index, char *title)
    {
        if (index < 0 || index > draws.size())
            return;

        draws[index]->title(title);
    }

    EXPORTED void linewidth_draw(int index, unsigned int size)
    {
        if (index < 0 || index > draws.size())
            return;

        draws[index]->linewidth(size);
    }

    EXPORTED void linestyle_draw(int index, unsigned int style)
    {
        if (index < 0 || index > draws.size())
            return;

        draws[index]->linestyle(style);
    }

    EXPORTED void linetype_draw(int index, unsigned int type)
    {
        if (index < 0 || index > draws.size())
            return;

        draws[index]->linetype(type);
    }

    EXPORTED void linecolor_draw(int index, char *color)
    {
        if (index < 0 || index > draws.size())
            return;

        draws[index]->linecolor(color);
    }

    EXPORTED void dashtype_draw(int index, unsigned int type)
    {
        if (index < 0 || index > draws.size())
            return;

        draws[index]->dashtype(type);
    }
}