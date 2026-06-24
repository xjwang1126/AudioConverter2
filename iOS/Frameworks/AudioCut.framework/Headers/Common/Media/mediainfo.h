#ifndef MEDIAINFO_H
#define MEDIAINFO_H

#include <cstdlib>
#include <cstdint>
#include <cstdio>

namespace MediaLibrary {

enum MediaType {
    MEDIA_NONE_TYPE = 0,

    MEDIA_STILL_PICTURE_TYPE,

    MEDIA_MOTION_PICTURE_TYPE,

    MEDIA_VIDEO_AUDIO_TYPE,
};

enum FormatType {
    FORMAT_NONE_TYPE = 0,

    FORMAT_MP4_TYPE,

    FORMAT_MP3_TYPE,

    FORMAT_FLAC_TYPE,

    FORMAT_OGG_TYPE,

    FORMAT_WAV_TYPE,

    FORMAT_M4A_TYPE,

    FORMAT_AAC_TYPE,

    FORMAT_GIF_TYPE,

    FORMAT_JPG_TYPE,

    FORMAT_PNG_TYPE,

    FORMAT_BMP_TYPE,

    FORMAT_WEBP_TYPE,
};

struct BufferData {
    uint8_t* buf;
    int size;
    uint8_t* ptr;
    size_t room;

    FILE* file{nullptr};
    int64_t fileSize{0};
};

struct BufferData2 {
    uint8_t *ptr;
    int size;
    int pos;
};

}

#endif // MEDIAINFO_H
