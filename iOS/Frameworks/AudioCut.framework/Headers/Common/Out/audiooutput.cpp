#include "audiooutput.h"

#include <Common/Model/frame.h>

extern "C" {
#include <libavformat/avformat.h>
}

#include <cassert>

namespace MediaLibrary {

AudioOutput::AudioOutput()
{
}

bool AudioOutput::open(const std::map<MediaLibrary::CodecKey, MediaLibrary::CodecValue> &codecInfos)
{
#ifndef CLOSE_NATIVE_CODE
#ifndef CLOSE_SDL_CODE
    int sampleRate = 0;
    AVSampleFormat sampleFormat = AV_SAMPLE_FMT_NONE;
    uint64_t channelLayout = 0;
    int channels = 0;
    for (auto codecInfo : codecInfos) {
        switch (codecInfo.first) {
        case CODEC_AUDIO_SAMPLE_FORMAT_EKY:
            sampleFormat = static_cast<AVSampleFormat>(std::get<int>(codecInfo.second));
            break;
        case CODEC_AUDIO_SAMPLE_RATE_KEY:
            sampleRate = std::get<int>(codecInfo.second);
            break;
        case CODEC_AUDIO_CHANNEL_LAYOUT_KEY:
            channelLayout = std::get<uint64_t>(codecInfo.second);
            break;
        case CODEC_AUDIO_CHANNELS_KEY:
            channels = std::get<int>(codecInfo.second);
            break;
        default:
            break;
        }
    }
    (void)channelLayout;

    SDL_AudioFormat audioFormat;
    switch (sampleFormat) {
    case AV_SAMPLE_FMT_S16:
        audioFormat = SDL_AUDIO_S16;
        break;
    default:
        assert(0);
        break;
    }

    SDL_AudioSpec audioSpec;
    SDL_zero(audioSpec);
    audioSpec.freq = sampleRate;
    audioSpec.format = audioFormat;
    audioSpec.channels = channels;

    audioStream_ = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &audioSpec, nullptr, nullptr);

    return audioStream_;
#else
    (void)codecInfos;
    return false;
#endif
#else
    (void)codecInfos;
    return false;
#endif
}

void AudioOutput::close()
{
#ifndef CLOSE_NATIVE_CODE
#ifndef CLOSE_SDL_CODE
    if (audioStream_) {
        SDL_DestroyAudioStream(audioStream_);
        audioStream_ = nullptr;
    }
#endif
#endif
}

void AudioOutput::play()
{
#ifndef CLOSE_NATIVE_CODE
#ifndef CLOSE_SDL_CODE
    SDL_ResumeAudioDevice(SDL_GetAudioStreamDevice(audioStream_));
#endif
#endif
}

void AudioOutput::pause()
{
#ifndef CLOSE_NATIVE_CODE
#ifndef CLOSE_SDL_CODE
    SDL_PauseAudioDevice(SDL_GetAudioStreamDevice(audioStream_));
#endif
#endif
}

void AudioOutput::flush()
{
#ifndef CLOSE_NATIVE_CODE
#ifndef CLOSE_SDL_CODE
    SDL_ClearAudioStream(audioStream_);
#endif
#endif
}

void AudioOutput::render(Frame *frame)
{
#ifndef CLOSE_NATIVE_CODE
#ifndef CLOSE_SDL_CODE
    AVFrame* audioFrame = static_cast<AVFrame*>(frame->getFrame());
    SDL_PutAudioStreamData(audioStream_, audioFrame->data[0], av_samples_get_buffer_size(nullptr, audioFrame->channels, audioFrame->nb_samples, (AVSampleFormat)audioFrame->format, 1));
#else
    (void)frame;
#endif
#else
    (void)frame;
#endif
}

}
