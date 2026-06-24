#ifndef AUDIOCONVERTER_H
#define AUDIOCONVERTER_H

#include "Codec/codecinfo.h"

#include <map>

extern "C" {
#include <libswresample/swresample.h>
}

namespace MediaLibrary {

class Frame;

class AudioConverter
{
public:
    AudioConverter();

    bool open(const std::map<CodecKey, CodecValue>& inCodecInfos, const std::map<CodecKey, CodecValue>& outCodecInfos);
    void close();

    Frame* convert(Frame* frame);

private:
    SwrContext* swrCtx_{nullptr};
    std::map<CodecKey, CodecValue> inCodecInfos_;
    std::map<CodecKey, CodecValue> outCodecInfos_;
};

}

#endif // AUDIOCONVERTER_H
