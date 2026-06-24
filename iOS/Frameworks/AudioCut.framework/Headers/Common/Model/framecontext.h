#ifndef FRAMECONTEXT_H
#define FRAMECONTEXT_H

#include "frameinfo.h"

#include <cstdint>

namespace MediaLibrary {

class Frame;

class FrameContext
{
public:
    FrameContext();
    virtual ~FrameContext();

    void setFrame(Frame* frame) { frame_ = frame; }

    Frame* getFrame() const { return frame_; }

    int64_t getTime(const TimeBase& timeBase = {1, SECOND_TO_MILLISECOND_UNIT});
    int64_t getDuration(const TimeBase& timeBase = {1, SECOND_TO_MILLISECOND_UNIT});

private:
    Frame* frame_{nullptr};
};

}

#endif // FRAMECONTEXT_H
