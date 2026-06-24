#ifndef IMAGEFRAME_H
#define IMAGEFRAME_H

#include "frame.h"

namespace MediaLibrary {

class ImageFrame : public Frame
{
public:
    ImageFrame();
    ImageFrame(const ImageFrame& imageFrame);
    virtual ~ImageFrame();

    virtual Frame* copy() const override;
    virtual Frame* convert(const FrameType& frameType) const override;
    virtual Frame* resize(const MSize& size) const override;

    virtual void copy(Frame* frame) override;

    virtual void setFrame(void* frame) override { frame_ = static_cast<uint8_t*>(frame); }
    virtual void setSize(const MSize& size) override { size_ = size; }
    virtual void setTime(const int64_t time, const TimeBase& timeBase = {1, SECOND_TO_MILLISECOND_UNIT}) override;
    virtual void setDuration(const int64_t duration, const TimeBase& timeBase = {1, SECOND_TO_MILLISECOND_UNIT}) override;

    virtual void* getFrame() const override { return frame_; }
    virtual MSize getSize() const override { return size_; }
    virtual int64_t getTime(const TimeBase& timeBase = {1, SECOND_TO_MILLISECOND_UNIT}) const override;
    virtual int64_t getDuration(const TimeBase& timeBase = {1, SECOND_TO_MILLISECOND_UNIT}) const override;

private:
    uint8_t* frame_{nullptr};
    MSize size_;
    int64_t time_{-1};
    int64_t duration_{0};
};

}

#endif // IMAGEFRAME_H
