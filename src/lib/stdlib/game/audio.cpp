#include "audio.h"
#include <SDL.h>
#include <cube/cubeext.h>
#include <map>

using namespace std;

static int audioId = 0;
static map<int, Audio *> audios;

extern "C"
{
    EXPORTED cube_native_var *open(cube_native_var *_path)
    {
        char *path = AS_NATIVE_STRING(_path);

        Audio *audio = createAudio(path, 0, SDL_MIX_MAXVOLUME);
        if (!audio)
        {
            delete audio;
            return NATIVE_NULL();
        }

        audios[audioId] = audio;
        cube_native_var *res = NATIVE_NUMBER(audioId);
        audioId++;
        return res;
    }

    EXPORTED void close(cube_native_var *id)
    {
        map<int, Audio *>::iterator it = audios.find(AS_NATIVE_NUMBER(id));
        if (it == audios.end())
            return;

        freeAudio(it->second);
        audios.erase(it);
    }

    EXPORTED void play(cube_native_var *id)
    {
        map<int, Audio *>::iterator it = audios.find(AS_NATIVE_NUMBER(id));
        if (it == audios.end())
            return;

        if (it->second->loop)
            playSoundFromMemory(it->second, SDL_MIX_MAXVOLUME);
        else
            playMusicFromMemory(it->second, SDL_MIX_MAXVOLUME);
    }

    EXPORTED void pause(cube_native_var *id)
    {
        map<int, Audio *>::iterator it = audios.find(AS_NATIVE_NUMBER(id));
        if (it == audios.end())
            return;
    }

    EXPORTED void stop(cube_native_var *id)
    {
        map<int, Audio *>::iterator it = audios.find(AS_NATIVE_NUMBER(id));
        if (it == audios.end())
            return;
    }

    EXPORTED void repeat(cube_native_var *id, cube_native_var *enable)
    {
        map<int, Audio *>::iterator it = audios.find(AS_NATIVE_NUMBER(id));
        if (it == audios.end())
            return;

        it->second->loop = AS_NATIVE_BOOL(enable);
    }
}