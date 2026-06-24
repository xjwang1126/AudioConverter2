#ifndef EMPTYFRAME_H
#define EMPTYFRAME_H

#include "frame.h"

namespace MediaLibrary {

class EmptyFrame : public Frame
{
public:
    EmptyFrame();
    EmptyFrame(const EmptyFrame& emptyFrame);
    virtual ~EmptyFrame();

    virtual Frame* copy() const override;

    virtual void setSize(const MSize& size) override { size_ = size; }
    virtual void setTime(const int64_t time, const TimeBase& timeBase = {1, SECOND_TO_MILLISECOND_UNIT}) override;
    virtual void setDuration(const int64_t duration, const TimeBase& timeBase = {1, SECOND_TO_MILLISECOND_UNIT}) override;

    virtual MSize getSize() const override { return size_; }
    virtual int64_t getTime(const TimeBase& timeBase = {1, SECOND_TO_MILLISECOND_UNIT}) const override;
    virtual int64_t getDuration(const TimeBase& timeBase = {1, SECOND_TO_MILLISECOND_UNIT}) const override;

private:
    virtual Frame* convert(const FrameType& frameType) const override;
    virtual Frame* resize(const MSize& size) const override;

    virtual void copy(Frame* frame) override;

    void setFrame(void* frame) override { (void)frame; }

    void* getFrame() const override { return nullptr; }

private:
    MSize size_;
    int64_t time_{-1};
    int64_t duration_{0};
};

}

#endif // EMPTYFRAME_H
