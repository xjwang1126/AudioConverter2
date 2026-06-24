#ifndef VIDEOCONVERTER_H
#define VIDEOCONVERTER_H

#include "Codec/codecinfo.h"

#include <map>

extern "C" {
#include <libswscale/swscale.h>
}

namespace MediaLibrary {

class Frame;

class VideoConverter
{
public:
    VideoConverter();

    bool open(const std::map<CodecKey, CodecValue>& inCodecInfos, const std::map<CodecKey, CodecValue>& outCodecInfos);
    void close();

    Frame* convert(Frame* frame);

private:
    SwsContext* swsCtx_{nullptr};
    std::map<CodecKey, CodecValue> inCodecInfos_;
    std::map<CodecKey, CodecValue> outCodecInfos_;
};

}

#endif // VIDEOCONVERTER_H
