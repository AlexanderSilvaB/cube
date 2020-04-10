#include <SDL.h>
#include <cube/cubeext.h>
#include <map>
#include <stdio.h>

using namespace std;

extern "C"
{
    EXPORTED cube_native_var *get()
    {
        cube_native_var *res = NATIVE_LIST();

        SDL_Event e;
        while (SDL_PollEvent(&e) != 0)
        {
            int i = SDL_QUIT;
            cube_native_var *ev = NATIVE_DICT();
            ADD_NATIVE_DICT(ev, COPY_STR("type"), NATIVE_NUMBER(e.type));

            switch (e.type)
            {
                case SDL_AUDIODEVICEADDED:
                case SDL_AUDIODEVICEREMOVED: {
                    ADD_NATIVE_DICT(ev, COPY_STR("timestamp"), NATIVE_NUMBER(e.adevice.timestamp));
                    ADD_NATIVE_DICT(ev, COPY_STR("which"), NATIVE_NUMBER(e.adevice.which));
                    ADD_NATIVE_DICT(ev, COPY_STR("iscapture"), NATIVE_NUMBER(e.adevice.iscapture));
                    break;
                }

                case SDL_CONTROLLERAXISMOTION: {
                    ADD_NATIVE_DICT(ev, COPY_STR("timestamp"), NATIVE_NUMBER(e.caxis.timestamp));
                    ADD_NATIVE_DICT(ev, COPY_STR("which"), NATIVE_NUMBER(e.caxis.which));
                    ADD_NATIVE_DICT(ev, COPY_STR("axis"), NATIVE_NUMBER(e.caxis.axis));
                    ADD_NATIVE_DICT(ev, COPY_STR("value"), NATIVE_NUMBER(e.caxis.value));
                    break;
                }

                case SDL_CONTROLLERBUTTONDOWN:
                case SDL_CONTROLLERBUTTONUP: {
                    ADD_NATIVE_DICT(ev, COPY_STR("timestamp"), NATIVE_NUMBER(e.cbutton.timestamp));
                    ADD_NATIVE_DICT(ev, COPY_STR("which"), NATIVE_NUMBER(e.cbutton.which));
                    ADD_NATIVE_DICT(ev, COPY_STR("button"), NATIVE_NUMBER(e.cbutton.button));
                    ADD_NATIVE_DICT(ev, COPY_STR("state"), NATIVE_NUMBER(e.cbutton.state));
                    break;
                }

                case SDL_CONTROLLERDEVICEADDED:
                case SDL_CONTROLLERDEVICEREMOVED:
                case SDL_CONTROLLERDEVICEREMAPPED: {
                    ADD_NATIVE_DICT(ev, COPY_STR("timestamp"), NATIVE_NUMBER(e.cdevice.timestamp));
                    ADD_NATIVE_DICT(ev, COPY_STR("which"), NATIVE_NUMBER(e.cdevice.which));
                    ADD_NATIVE_DICT(ev, COPY_STR("ctype"), NATIVE_NUMBER(e.cdevice.type));
                    break;
                }

                case SDL_DOLLARGESTURE:
                case SDL_DOLLARRECORD: {
                    ADD_NATIVE_DICT(ev, COPY_STR("timestamp"), NATIVE_NUMBER(e.dgesture.timestamp));
                    ADD_NATIVE_DICT(ev, COPY_STR("touchId"), NATIVE_NUMBER(e.dgesture.touchId));
                    ADD_NATIVE_DICT(ev, COPY_STR("gestureId"), NATIVE_NUMBER(e.dgesture.gestureId));
                    ADD_NATIVE_DICT(ev, COPY_STR("numFingers"), NATIVE_NUMBER(e.dgesture.numFingers));
                    ADD_NATIVE_DICT(ev, COPY_STR("error"), NATIVE_NUMBER(e.dgesture.error));
                    ADD_NATIVE_DICT(ev, COPY_STR("x"), NATIVE_NUMBER(e.dgesture.x));
                    ADD_NATIVE_DICT(ev, COPY_STR("y"), NATIVE_NUMBER(e.dgesture.y));
                    break;
                }

                case SDL_DROPFILE:
                case SDL_DROPTEXT:
                case SDL_DROPBEGIN:
                case SDL_DROPCOMPLETE: {
                    ADD_NATIVE_DICT(ev, COPY_STR("timestamp"), NATIVE_NUMBER(e.drop.timestamp));
                    ADD_NATIVE_DICT(ev, COPY_STR("windowID"), NATIVE_NUMBER(e.drop.windowID));
                    ADD_NATIVE_DICT(ev, COPY_STR("file"), NATIVE_STRING_COPY(e.drop.file));
                    break;
                }

                case SDL_FINGERMOTION:
                case SDL_FINGERDOWN:
                case SDL_FINGERUP: {
                    ADD_NATIVE_DICT(ev, COPY_STR("timestamp"), NATIVE_NUMBER(e.tfinger.timestamp));
                    ADD_NATIVE_DICT(ev, COPY_STR("touchId"), NATIVE_NUMBER(e.tfinger.touchId));
                    ADD_NATIVE_DICT(ev, COPY_STR("fingerId"), NATIVE_NUMBER(e.tfinger.fingerId));
                    ADD_NATIVE_DICT(ev, COPY_STR("pressure"), NATIVE_NUMBER(e.tfinger.pressure));
                    ADD_NATIVE_DICT(ev, COPY_STR("x"), NATIVE_NUMBER(e.tfinger.x));
                    ADD_NATIVE_DICT(ev, COPY_STR("y"), NATIVE_NUMBER(e.tfinger.y));
                    ADD_NATIVE_DICT(ev, COPY_STR("dx"), NATIVE_NUMBER(e.tfinger.dx));
                    ADD_NATIVE_DICT(ev, COPY_STR("dy"), NATIVE_NUMBER(e.tfinger.dy));
                    break;
                }

                case SDL_KEYDOWN:
                case SDL_KEYUP: {
                    ADD_NATIVE_DICT(ev, COPY_STR("timestamp"), NATIVE_NUMBER(e.key.timestamp));
                    ADD_NATIVE_DICT(ev, COPY_STR("windowID"), NATIVE_NUMBER(e.key.windowID));
                    ADD_NATIVE_DICT(ev, COPY_STR("state"), NATIVE_NUMBER(e.key.state));
                    ADD_NATIVE_DICT(ev, COPY_STR("repeat"), NATIVE_NUMBER(e.key.repeat));
                    ADD_NATIVE_DICT(ev, COPY_STR("scancode"), NATIVE_NUMBER(e.key.keysym.scancode));
                    ADD_NATIVE_DICT(ev, COPY_STR("mod"), NATIVE_NUMBER(e.key.keysym.mod));
                    break;
                }

                case SDL_JOYAXISMOTION: {
                    ADD_NATIVE_DICT(ev, COPY_STR("timestamp"), NATIVE_NUMBER(e.jaxis.timestamp));
                    ADD_NATIVE_DICT(ev, COPY_STR("which"), NATIVE_NUMBER(e.jaxis.which));
                    ADD_NATIVE_DICT(ev, COPY_STR("axis"), NATIVE_NUMBER(e.jaxis.axis));
                    ADD_NATIVE_DICT(ev, COPY_STR("value"), NATIVE_NUMBER(e.jaxis.value));
                    break;
                }

                case SDL_JOYBALLMOTION: {
                    ADD_NATIVE_DICT(ev, COPY_STR("timestamp"), NATIVE_NUMBER(e.jball.timestamp));
                    ADD_NATIVE_DICT(ev, COPY_STR("which"), NATIVE_NUMBER(e.jball.which));
                    ADD_NATIVE_DICT(ev, COPY_STR("ball"), NATIVE_NUMBER(e.jball.ball));
                    ADD_NATIVE_DICT(ev, COPY_STR("xrel"), NATIVE_NUMBER(e.jball.xrel));
                    ADD_NATIVE_DICT(ev, COPY_STR("yrel"), NATIVE_NUMBER(e.jball.yrel));
                    break;
                }

                case SDL_JOYHATMOTION: {
                    ADD_NATIVE_DICT(ev, COPY_STR("timestamp"), NATIVE_NUMBER(e.jhat.timestamp));
                    ADD_NATIVE_DICT(ev, COPY_STR("which"), NATIVE_NUMBER(e.jhat.which));
                    ADD_NATIVE_DICT(ev, COPY_STR("hat"), NATIVE_NUMBER(e.jhat.hat));
                    ADD_NATIVE_DICT(ev, COPY_STR("value"), NATIVE_NUMBER(e.jhat.value));
                    break;
                }

                case SDL_JOYBUTTONDOWN:
                case SDL_JOYBUTTONUP: {
                    ADD_NATIVE_DICT(ev, COPY_STR("timestamp"), NATIVE_NUMBER(e.jbutton.timestamp));
                    ADD_NATIVE_DICT(ev, COPY_STR("which"), NATIVE_NUMBER(e.jbutton.which));
                    ADD_NATIVE_DICT(ev, COPY_STR("button"), NATIVE_NUMBER(e.jbutton.button));
                    ADD_NATIVE_DICT(ev, COPY_STR("state"), NATIVE_NUMBER(e.jbutton.state));
                    break;
                }

                case SDL_JOYDEVICEADDED:
                case SDL_JOYDEVICEREMOVED: {
                    ADD_NATIVE_DICT(ev, COPY_STR("timestamp"), NATIVE_NUMBER(e.jdevice.timestamp));
                    ADD_NATIVE_DICT(ev, COPY_STR("which"), NATIVE_NUMBER(e.jdevice.which));
                    break;
                }

                case SDL_MOUSEMOTION: {
                    ADD_NATIVE_DICT(ev, COPY_STR("timestamp"), NATIVE_NUMBER(e.motion.timestamp));
                    ADD_NATIVE_DICT(ev, COPY_STR("which"), NATIVE_NUMBER(e.motion.which));
                    ADD_NATIVE_DICT(ev, COPY_STR("windowID"), NATIVE_NUMBER(e.motion.windowID));
                    ADD_NATIVE_DICT(ev, COPY_STR("state"), NATIVE_NUMBER(e.motion.state));
                    ADD_NATIVE_DICT(ev, COPY_STR("x"), NATIVE_NUMBER(e.motion.x));
                    ADD_NATIVE_DICT(ev, COPY_STR("y"), NATIVE_NUMBER(e.motion.y));
                    ADD_NATIVE_DICT(ev, COPY_STR("xrel"), NATIVE_NUMBER(e.motion.xrel));
                    ADD_NATIVE_DICT(ev, COPY_STR("yrel"), NATIVE_NUMBER(e.motion.yrel));
                    break;
                }

                case SDL_MOUSEBUTTONDOWN:
                case SDL_MOUSEBUTTONUP: {
                    ADD_NATIVE_DICT(ev, COPY_STR("timestamp"), NATIVE_NUMBER(e.button.timestamp));
                    ADD_NATIVE_DICT(ev, COPY_STR("which"), NATIVE_NUMBER(e.button.which));
                    ADD_NATIVE_DICT(ev, COPY_STR("windowID"), NATIVE_NUMBER(e.button.windowID));
                    ADD_NATIVE_DICT(ev, COPY_STR("state"), NATIVE_NUMBER(e.button.state));
                    ADD_NATIVE_DICT(ev, COPY_STR("x"), NATIVE_NUMBER(e.button.x));
                    ADD_NATIVE_DICT(ev, COPY_STR("y"), NATIVE_NUMBER(e.button.y));
                    ADD_NATIVE_DICT(ev, COPY_STR("button"), NATIVE_NUMBER(e.button.button));
                    ADD_NATIVE_DICT(ev, COPY_STR("clicks"), NATIVE_NUMBER(e.button.clicks));
                    break;
                }

                case SDL_MOUSEWHEEL: {
                    ADD_NATIVE_DICT(ev, COPY_STR("timestamp"), NATIVE_NUMBER(e.wheel.timestamp));
                    ADD_NATIVE_DICT(ev, COPY_STR("which"), NATIVE_NUMBER(e.wheel.which));
                    ADD_NATIVE_DICT(ev, COPY_STR("windowID"), NATIVE_NUMBER(e.wheel.windowID));
                    ADD_NATIVE_DICT(ev, COPY_STR("direction"), NATIVE_NUMBER(e.wheel.direction));
                    ADD_NATIVE_DICT(ev, COPY_STR("x"), NATIVE_NUMBER(e.wheel.x));
                    ADD_NATIVE_DICT(ev, COPY_STR("y"), NATIVE_NUMBER(e.wheel.y));
                    break;
                }

                case SDL_MULTIGESTURE: {
                    ADD_NATIVE_DICT(ev, COPY_STR("timestamp"), NATIVE_NUMBER(e.mgesture.timestamp));
                    ADD_NATIVE_DICT(ev, COPY_STR("numFingers"), NATIVE_NUMBER(e.mgesture.numFingers));
                    ADD_NATIVE_DICT(ev, COPY_STR("touchId"), NATIVE_NUMBER(e.mgesture.touchId));
                    ADD_NATIVE_DICT(ev, COPY_STR("dTheta"), NATIVE_NUMBER(e.mgesture.dTheta));
                    ADD_NATIVE_DICT(ev, COPY_STR("dDist"), NATIVE_NUMBER(e.mgesture.dDist));
                    ADD_NATIVE_DICT(ev, COPY_STR("x"), NATIVE_NUMBER(e.mgesture.x));
                    ADD_NATIVE_DICT(ev, COPY_STR("y"), NATIVE_NUMBER(e.mgesture.y));
                    break;
                }

                case SDL_QUIT: {
                    ADD_NATIVE_DICT(ev, COPY_STR("timestamp"), NATIVE_NUMBER(e.quit.timestamp));
                    break;
                }

                case SDL_SYSWMEVENT: {
                    ADD_NATIVE_DICT(ev, COPY_STR("timestamp"), NATIVE_NUMBER(e.syswm.timestamp));
                    break;
                }

                case SDL_TEXTEDITING: {
                    ADD_NATIVE_DICT(ev, COPY_STR("timestamp"), NATIVE_NUMBER(e.edit.timestamp));
                    ADD_NATIVE_DICT(ev, COPY_STR("windowID"), NATIVE_NUMBER(e.edit.windowID));
                    ADD_NATIVE_DICT(ev, COPY_STR("start"), NATIVE_NUMBER(e.edit.start));
                    ADD_NATIVE_DICT(ev, COPY_STR("length"), NATIVE_NUMBER(e.edit.length));
                    ADD_NATIVE_DICT(ev, COPY_STR("text"), NATIVE_STRING_COPY(e.edit.text));
                    break;
                }

                case SDL_TEXTINPUT: {
                    ADD_NATIVE_DICT(ev, COPY_STR("timestamp"), NATIVE_NUMBER(e.text.timestamp));
                    ADD_NATIVE_DICT(ev, COPY_STR("windowID"), NATIVE_NUMBER(e.text.windowID));
                    ADD_NATIVE_DICT(ev, COPY_STR("text"), NATIVE_STRING_COPY(e.text.text));
                    break;
                }

                case SDL_USEREVENT: {
                    ADD_NATIVE_DICT(ev, COPY_STR("timestamp"), NATIVE_NUMBER(e.user.timestamp));
                    ADD_NATIVE_DICT(ev, COPY_STR("windowID"), NATIVE_NUMBER(e.user.windowID));
                    ADD_NATIVE_DICT(ev, COPY_STR("code"), NATIVE_NUMBER(e.user.code));
                    ADD_NATIVE_DICT(ev, COPY_STR("data1"), NATIVE_NUMBER(*((int *)&e.user.data1)));
                    ADD_NATIVE_DICT(ev, COPY_STR("data2"), NATIVE_NUMBER(*((int *)&e.user.data2)));
                    break;
                }

                case SDL_WINDOWEVENT: {
                    ADD_NATIVE_DICT(ev, COPY_STR("timestamp"), NATIVE_NUMBER(e.window.timestamp));
                    ADD_NATIVE_DICT(ev, COPY_STR("windowID"), NATIVE_NUMBER(e.window.windowID));
                    ADD_NATIVE_DICT(ev, COPY_STR("event"), NATIVE_NUMBER(e.window.event));
                    ADD_NATIVE_DICT(ev, COPY_STR("data1"), NATIVE_NUMBER(e.window.data1));
                    ADD_NATIVE_DICT(ev, COPY_STR("data2"), NATIVE_NUMBER(e.window.data2));
                    break;
                }

                default: {
                }
            }

            ADD_NATIVE_LIST(res, ev);
        }

        return res;
    }
}