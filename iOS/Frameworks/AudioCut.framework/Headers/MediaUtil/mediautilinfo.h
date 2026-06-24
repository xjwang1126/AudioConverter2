#ifndef MEDIAUTILINFO_H
#define MEDIAUTILINFO_H

#include <string>
#include <cstdint>

#ifndef USE_CUSTOM_COMPONENT
#define USE_CUSTOM_COMPONENT 1
#endif

namespace MediaLibrary {

struct VideoFormat {
    int width = -1;

    int height = -1;

    float fps = -1.;

    int keyframeInteval = -1;
};

struct AudioFormat {
    int sampleRate = -1;

    int bitrate = -1;

    int channels = -1;
};

struct MediaInfo {
    std::string format;

    int duration = 0;

    int streams = 0;
};

enum TaskType {
    TASK_VERTICAL_FLIP_TYPE,

    TASK_HORIZONTAL_FLIP_TYPE,

    TASK_ROTATE_TYPE,
};

}

#endif // MEDIAUTILINFO_H
