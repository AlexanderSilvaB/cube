#include <basicgl/Manager.hpp>
#include <cube/cubeext.h>

using namespace BasicGL;

cube_native_var *animation_func = NULL;
cube_native_var *keyboard_func = NULL;
cube_native_var *mouse_func = NULL;

void animation(ElementsList elements, WindowPtr window, float ellasedTime)
{
    if (animation_func != NULL)
    {
        cube_native_var *args = NATIVE_LIST();
        ADD_NATIVE_LIST(args, NATIVE_NUMBER(window->index));
        ADD_NATIVE_LIST(args, NATIVE_NUMBER(ellasedTime));

        CALL_NATIVE_FUNC(animation_func, args);
    }
}

void keyboard(Keyboard keyboard, WindowPtr window)
{
    if (keyboard_func != NULL)
    {
        cube_native_var *args = NATIVE_LIST();
        ADD_NATIVE_LIST(args, NATIVE_NUMBER(window->index));

        cube_native_var *kb = NATIVE_DICT();
        ADD_NATIVE_DICT(kb, COPY_STR("alt"), NATIVE_BOOL(keyboard.alt));
        ADD_NATIVE_DICT(kb, COPY_STR("ctrl"), NATIVE_BOOL(keyboard.ctrl));
        ADD_NATIVE_DICT(kb, COPY_STR("shift"), NATIVE_BOOL(keyboard.shift));
        ADD_NATIVE_DICT(kb, COPY_STR("key"), NATIVE_NUMBER(keyboard.key));
        ADD_NATIVE_DICT(kb, COPY_STR("x"), NATIVE_NUMBER(keyboard.x));
        ADD_NATIVE_DICT(kb, COPY_STR("y"), NATIVE_NUMBER(keyboard.y));
        ADD_NATIVE_DICT(kb, COPY_STR("windowX"), NATIVE_NUMBER(keyboard.windowX));
        ADD_NATIVE_DICT(kb, COPY_STR("windowY"), NATIVE_NUMBER(keyboard.windowY));

        ADD_NATIVE_LIST(args, kb);

        CALL_NATIVE_FUNC(keyboard_func, args);
    }
}

void mouse(Mouse mouse, WindowPtr window)
{
    if (mouse_func != NULL)
    {
        cube_native_var *args = NATIVE_LIST();
        ADD_NATIVE_LIST(args, NATIVE_NUMBER(window->index));

        cube_native_var *ms = NATIVE_DICT();
        ADD_NATIVE_DICT(ms, COPY_STR("pressed"), NATIVE_BOOL(mouse.pressed));
        ADD_NATIVE_DICT(ms, COPY_STR("released"), NATIVE_BOOL(mouse.released));
        ADD_NATIVE_DICT(ms, COPY_STR("move"), NATIVE_BOOL(mouse.move));
        ADD_NATIVE_DICT(ms, COPY_STR("leave"), NATIVE_BOOL(mouse.leave));
        ADD_NATIVE_DICT(ms, COPY_STR("entered"), NATIVE_BOOL(mouse.entered));
        ADD_NATIVE_DICT(ms, COPY_STR("left"), NATIVE_BOOL(mouse.left));
        ADD_NATIVE_DICT(ms, COPY_STR("right"), NATIVE_BOOL(mouse.right));
        ADD_NATIVE_DICT(ms, COPY_STR("middle"), NATIVE_BOOL(mouse.middle));
        ADD_NATIVE_DICT(ms, COPY_STR("scroll"), NATIVE_NUMBER(mouse.scroll));
        ADD_NATIVE_DICT(ms, COPY_STR("dx"), NATIVE_NUMBER(mouse.dx));
        ADD_NATIVE_DICT(ms, COPY_STR("dy"), NATIVE_NUMBER(mouse.dy));
        ADD_NATIVE_DICT(ms, COPY_STR("x"), NATIVE_NUMBER(mouse.x));
        ADD_NATIVE_DICT(ms, COPY_STR("y"), NATIVE_NUMBER(mouse.y));
        ADD_NATIVE_DICT(ms, COPY_STR("windowX"), NATIVE_NUMBER(mouse.windowX));
        ADD_NATIVE_DICT(ms, COPY_STR("windowY"), NATIVE_NUMBER(mouse.windowY));

        ADD_NATIVE_LIST(args, ms);

        CALL_NATIVE_FUNC(mouse_func, args);
    }
}

extern "C"
{
    EXPORTED void cube_init()
    {
        char *argv[0];
        int argc = 0;
        Manager::Init(argc, argv);

        animation_func = NULL;
        keyboard_func = NULL;
        mouse_func = NULL;
    }

    EXPORTED void cube_release()
    {
        Manager::Destroy();
    }

    EXPORTED int create_window(const char *name, bool is3d, int fps, int width, int height, int x, int y)
    {
        int id = Manager::Create(name, is3d ? Modes::MODE_3D : Modes::MODE_2D, fps, width, height, x, y);
        Manager::SetAnimationFunction(animation);
        Manager::SetKeyboardFunction(keyboard);
        Manager::SetMouseFunction(mouse);
        return id;
    }

    EXPORTED void select_window(int id)
    {
        Manager::SelectWindow(id);
    }

    EXPORTED void set_cartesian(bool cartesian)
    {
        Manager::SetCartesian(cartesian);
    }

    EXPORTED bool is_open()
    {
        return Manager::IsOpen();
    }

    EXPORTED bool is_fullscreen()
    {
        return Manager::IsFullscreen();
    }

    EXPORTED float window_width()
    {
        return Manager::WindowWidth();
    }

    EXPORTED float window_height()
    {
        return Manager::WindowHeight();
    }

    EXPORTED void set_background_float(float r, float g, float b)
    {
        Manager::SetBackground(r, g, b);
    }

    EXPORTED void set_background_int(int r, int g, int b)
    {
        Manager::SetBackground(r, g, b);
    }

    EXPORTED void set_fullscreen(bool fullscreen)
    {
        Manager::SetFullscreen(fullscreen);
    }

    EXPORTED void toogle_fullscreen()
    {
        Manager::ToggleFullscreen();
    }

    EXPORTED bool capture_bgl(const char *fileName)
    {
        return Manager::Capture(fileName);
    }

    EXPORTED bool start_recording(const char *fileName, int fps)
    {
        return Manager::StartRecording(fileName, fps);
    }

    EXPORTED void stop_recording()
    {
        Manager::StopRecording();
    }

    EXPORTED void pause_bgl(float secs)
    {
        Manager::Pause(secs);
    }

    EXPORTED void show_bgl()
    {
        Manager::Show();
    }

    EXPORTED int create_element(const int type, const char *name)
    {
        ElementPtr element = Manager::CreateElement((Elements)type, name);
        if (element == NULL)
            return -1;
        return Manager::index(element);
    }

    EXPORTED int create_plot(int rows, int cols, int index, const char *name)
    {
        Plot *plot = Manager::CreatePlot(rows, cols, index, name);
        if (plot == NULL)
            return -1;
        return Manager::index(plot);
    }

    EXPORTED int find_element(const char *name)
    {
        ElementPtr element = Manager::find(name);
        if (element == NULL)
            return -1;
        return Manager::index(element);
    }

    EXPORTED void set_animation_callback(cube_native_var *func)
    {
        animation_func = SAVE_FUNC(func);
    }

    EXPORTED void set_keyboard_callback(cube_native_var *func)
    {
        keyboard_func = SAVE_FUNC(func);
    }

    EXPORTED void set_mouse_callback(cube_native_var *func)
    {
        mouse_func = SAVE_FUNC(func);
    }

    EXPORTED void rotate_element(int index, float x, float y, float z)
    {
        ElementPtr element = Manager::get(index);
        if (element == NULL)
            return;
        element->rotate(x, y, z);
    }

    EXPORTED void rotate_to_element(int index, float x, float y, float z)
    {
        ElementPtr element = Manager::get(index);
        if (element == NULL)
            return;
        element->rotateTo(x, y, z);
    }

    EXPORTED void translate_element(int index, float x, float y, float z)
    {
        ElementPtr element = Manager::get(index);
        if (element == NULL)
            return;
        element->translate(x, y, z);
    }

    EXPORTED void move_element(int index, float x, float y, float z)
    {
        ElementPtr element = Manager::get(index);
        if (element == NULL)
            return;
        element->moveTo(x, y, z);
    }

    EXPORTED void flip_element(int index, bool x, bool y, bool z)
    {
        ElementPtr element = Manager::get(index);
        if (element == NULL)
            return;
        element->flip(x, y, z);
    }

    EXPORTED void scale_element(int index, float x, float y, float z)
    {
        ElementPtr element = Manager::get(index);
        if (element == NULL)
            return;
        element->scale(x, y, z);
    }

    EXPORTED void scale_to_element(int index, float x, float y, float z)
    {
        ElementPtr element = Manager::get(index);
        if (element == NULL)
            return;
        element->scaleTo(x, y, z);
    }

    EXPORTED void color_element_float(int index, float r, float g, float b, float a)
    {
        ElementPtr element = Manager::get(index);
        if (element == NULL)
            return;
        element->rgb(r, g, b, a);
    }

    EXPORTED void color_element_int(int index, int r, int g, int b, int a)
    {
        ElementPtr element = Manager::get(index);
        if (element == NULL)
            return;
        element->rgb((unsigned char)r, g, b, a);
    }

    EXPORTED void wireframe_element(int index, bool wireframe)
    {
        ElementPtr element = Manager::get(index);
        if (element == NULL)
            return;
        element->setWireframe(wireframe);
    }

    EXPORTED void set_text_element(int index, const char *text)
    {
        ElementPtr element = Manager::get(index);
        if (element == NULL)
            return;
        element->setText(text);
    }

    EXPORTED void point_2d_element(int index, float x, float y, int pIndex)
    {
        ElementPtr element = Manager::get(index);
        if (element == NULL)
            return;
        element->point(x, y, pIndex);
    }

    EXPORTED void point_3d_element(int index, float x, float y, float z, int pIndex)
    {
        ElementPtr element = Manager::get(index);
        if (element == NULL)
            return;
        element->point(x, y, z, pIndex);
    }

    EXPORTED void line_2d_element(int index, float x1, float y1, float x2, float y2, int pIndex)
    {
        ElementPtr element = Manager::get(index);
        if (element == NULL)
            return;
        element->line(x1, y1, x2, y2, pIndex);
    }

    EXPORTED void line_3d_element(int index, float x1, float y1, float z1, float x2, float y2, float z2, int pIndex)
    {
        ElementPtr element = Manager::get(index);
        if (element == NULL)
            return;
        element->line(x1, y1, z1, x2, y2, z2, pIndex);
    }

    EXPORTED void triangle_2d_element(int index, float x1, float y1, float x2, float y2, float x3, float y3, int pIndex)
    {
        ElementPtr element = Manager::get(index);
        if (element == NULL)
            return;
        element->triangle(x1, y1, x2, y2, x3, y3, pIndex);
    }

    EXPORTED void triangle_3d_element(int index, float x1, float y1, float z1, float x2, float y2, float z2, float x3,
                                      float y3, float z3, int pIndex)
    {
        ElementPtr element = Manager::get(index);
        if (element == NULL)
            return;
        element->triangle(x1, y1, z1, x2, y2, z2, x3, y3, z3, pIndex);
    }

    EXPORTED void rectangle_2d_element(int index, float x1, float y1, float x2, float y2, int pIndex)
    {
        ElementPtr element = Manager::get(index);
        if (element == NULL)
            return;
        element->rectangle(x1, y1, x2, y2, pIndex);
    }

    EXPORTED void rectangle_3d_element(int index, float x1, float y1, float z1, float x2, float y2, float z2,
                                       int pIndex)
    {
        ElementPtr element = Manager::get(index);
        if (element == NULL)
            return;
        element->rectangle(x1, y1, z1, x2, y2, z2, pIndex);
    }

    EXPORTED void circle_2d_element(int index, float x, float y, float r)
    {
        ElementPtr element = Manager::get(index);
        if (element == NULL)
            return;
        element->circle(x, y, r);
    }

    EXPORTED void circle_3d_element(int index, float x, float y, float z, float r)
    {
        ElementPtr element = Manager::get(index);
        if (element == NULL)
            return;
        element->circle(x, y, z, r);
    }

    EXPORTED void glow_element(int index, float start, float scale)
    {
        ElementPtr element = Manager::get(index);
        if (element == NULL)
            return;
        element->glow(start, scale);
    }

    EXPORTED void glow_at_element(int index, int pIndex, int pos)
    {
        ElementPtr element = Manager::get(index);
        if (element == NULL)
            return;
        element->glowAt(pIndex, pos);
    }

    EXPORTED void map_element(int index)
    {
        ElementPtr element = Manager::get(index);
        if (element == NULL)
            return;
        element->map();
    }

    EXPORTED void reshape_element(int index, int n, bool byElement)
    {
        ElementPtr element = Manager::get(index);
        if (element == NULL)
            return;
        element->reshape(n, byElement);
    }

    EXPORTED float text_width_element(int index)
    {
        ElementPtr element = Manager::get(index);
        if (element == NULL)
            return 0;
        return element->getTextWidth();
    }

    EXPORTED float text_height_element(int index)
    {
        ElementPtr element = Manager::get(index);
        if (element == NULL)
            return 0;
        return element->getTextHeight();
    }

    EXPORTED void text_align_element(int index, int align)
    {
        ElementPtr element = Manager::get(index);
        if (element == NULL)
            return;
        element->textAlign(align);
    }

    EXPORTED int new_point_element(int index, float x, float y, float z)
    {
        ElementPtr element = Manager::get(index);
        if (element == NULL)
            return -1;
        return element->newPoint(x, y, z);
    }

    EXPORTED int add_element(int index, const int type)
    {
        ElementPtr element = Manager::get(index);
        if (element == NULL)
            return -1;

        ElementPtr newElement = element->create((Elements)type);
        if (newElement == NULL)
            return -1;
        return Manager::index(newElement);
    }

    EXPORTED float time_window()
    {
        return Manager::CurrentWindow()->timeSinceBegin;
    }
}