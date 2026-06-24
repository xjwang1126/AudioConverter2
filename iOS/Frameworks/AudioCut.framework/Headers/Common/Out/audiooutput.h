#ifndef AUDIOOUTPUT_H
#define AUDIOOUTPUT_H

#ifndef CLOSE_NATIVE_CODE
#ifndef CLOSE_SDL_CODE
#include <SDL3/SDL.h>
#endif
#endif

#include <map>

#include "Codec/codecinfo.h"

namespace MediaLibrary {

class Frame;

class AudioOutput
{
public:
    AudioOutput();

    bool open(const std::map<MediaLibrary::CodecKey, MediaLibrary::CodecValue>& codecInfos);
    void close();

    void play();
    void pause();
    void flush();

    void render(Frame* frame);

private:
#ifndef CLOSE_NATIVE_CODE
#ifndef CLOSE_SDL_CODE
    SDL_AudioStream* audioStream_{nullptr};
#endif
#endif
};

}

#endif // AUDIOOUTPUT_H
