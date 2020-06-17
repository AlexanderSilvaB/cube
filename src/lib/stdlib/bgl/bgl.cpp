#include <basicgl/Manager.hpp>
#include <cube/cubeext.h>

using namespace BasicGL;

void animation(ElementsList elements, WindowPtr window, float ellasedTime)
{
}

void keyboard(Keyboard keyboard, WindowPtr window)
{
}

void mouse(Mouse mouse, WindowPtr window)
{
}

extern "C"
{
    EXPORTED void cube_init()
    {
        char *argv[0];
        int argc = 0;
        Manager::Init(argc, argv);
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
    }
}