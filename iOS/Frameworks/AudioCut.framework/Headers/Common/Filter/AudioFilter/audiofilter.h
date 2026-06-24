#ifndef AUDIOFILTER_H
#define AUDIOFILTER_H

#include "Codec/codecinfo.h"

#include <map>
#include <string>

extern "C" {
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavfilter/avfilter.h>
#include <libavutil/opt.h>
}

namespace MediaLibrary {

class Frame;

class AudioFilter
{
public:
    AudioFilter();

    bool open(const std::string& description, const std::map<CodecKey, CodecValue>& inCodecInfos, const std::map<CodecKey, CodecValue>& outCodecInfos);
    void close();

    void add(Frame* frame);
    Frame* get();

private:
    AVFilterGraph* filterGraph_{nullptr};
    AVFilterContext* bufferSrcCtx_{nullptr};
    AVFilterContext* bufferSinkCtx_{nullptr};
};

}

#endif // AUDIOFILTER_H
