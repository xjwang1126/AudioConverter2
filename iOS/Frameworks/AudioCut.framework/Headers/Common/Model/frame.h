#ifndef FRAME_H
#define FRAME_H

#include "frameinfo.h"
#include "Format/msize.h"

#include <cstdint>

namespace MediaLibrary {

class Frame
{
public:
    enum FrameType {
        FRAME_NONE_TYPE = 0,
        FRAME_IMAGE_TYPE,
        FRAME_FFMPEG_TYPE,
        FRAME_TEXTURE_TYPE,
        FRAME_EMPTY_TYPE,
    };

public:
    Frame(const FrameType& frameType);
    Frame(const Frame& frame);
    virtual ~Frame();

    const FrameType& getFrameType() const { return frameType_; }

    void setTimeBase(const TimeBase& timeBase) { timeBase_ = timeBase; }

    virtual Frame* copy() const = 0;
    virtual Frame* convert(const FrameType& frameType) const = 0;
    virtual Frame* resize(const MSize& size) const = 0;

    virtual void copy(Frame* frame) = 0;

    virtual void setFrame(void* frame) = 0;
    virtual void setSize(const MSize& size) { (void)size; }
    virtual void setTime(const int64_t time, const TimeBase& timeBase = {1, SECOND_TO_MILLISECOND_UNIT}) = 0;
    virtual void setDuration(const int64_t duration, const TimeBase& timeBase = {1, SECOND_TO_MILLISECOND_UNIT}) = 0;

    virtual void* getFrame() const = 0;
    virtual MSize getSize() const { return MSize(); }
    virtual int64_t getTime(const TimeBase& timeBase = {1, SECOND_TO_MILLISECOND_UNIT}) const = 0;
    virtual int64_t getDuration(const TimeBase& timeBase = {1, SECOND_TO_MILLISECOND_UNIT}) const = 0;

protected:
    const FrameType frameType_;
    TimeBase timeBase_;
};

}

#endif // FRAME_H
