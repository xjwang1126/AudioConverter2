#ifndef CODECINFO_H
#define CODECINFO_H

#include <variant>
#include <cstdint>

namespace MediaLibrary {

enum CodecType {
    CODEC_HARDWARE_TYPE,
    CODEC_SOFTWARE_TYPE,
};

enum CodecKey {
    CODEC_ID_KEY,
    CODEC_NAME_KEY,
    CODEC_VIDEO_WIDTH_KEY,
    CODEC_VIDEO_HEIGHT_KEY,
    CODEC_VIDEO_PIXEL_FORMAT_KEY,
    CODEC_VIDEO_FPS_KEY,
    CODEC_AUDIO_SAMPLE_FORMAT_EKY,
    CODEC_AUDIO_SAMPLE_RATE_KEY,
    CODEC_AUDIO_CHANNEL_LAYOUT_KEY,
    CODEC_AUDIO_CHANNELS_KEY,
    CODEC_AUDIO_SAMPLE_SIZE_KEY,
};

using CodecValue = std::variant<int, float, bool, std::string, uint64_t, int64_t>;

}

#endif // CODECINFO_H
