#include "emptyframe.h"

namespace MediaLibrary {

EmptyFrame::EmptyFrame()
    : Frame(Frame::FRAME_EMPTY_TYPE)
{
}

EmptyFrame::EmptyFrame(const EmptyFrame &emptyFrame)
    : Frame(emptyFrame)
{
    size_ = emptyFrame.size_;
    time_ = emptyFrame.time_;
    duration_ = emptyFrame.duration_;
}

EmptyFrame::~EmptyFrame()
{
    size_ = MSize();
    time_ = -1;
    duration_ = 0;
}

Frame *EmptyFrame::copy() const
{
    return new EmptyFrame(*this);
}

void EmptyFrame::setTime(const int64_t time, const TimeBase &timeBase)
{
    time_ = (time * timeBase.num * timeBase_.den) / (timeBase.den * timeBase_.num);
}

void EmptyFrame::setDuration(const int64_t duration, const TimeBase &timeBase)
{
    duration_ = (duration * timeBase.num * timeBase_.den) / (timeBase.den * timeBase_.num);
}

int64_t EmptyFrame::getTime(const TimeBase &timeBase) const
{
    return (time_ * timeBase_.num * timeBase.den) / (timeBase_.den * timeBase.num);
}

int64_t EmptyFrame::getDuration(const TimeBase &timeBase) const
{
    return (duration_ * timeBase_.num * timeBase.den) / (timeBase_.den * timeBase.num);
}

Frame *EmptyFrame::convert(const Frame::FrameType &frameType) const
{
    (void)frameType;
    return nullptr;
}

Frame *EmptyFrame::resize(const MSize &size) const
{
    (void)size;
    return nullptr;
}

void EmptyFrame::copy(Frame *frame)
{
    (void)frame;
}

}
