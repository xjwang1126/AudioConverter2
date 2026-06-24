#ifndef FFMPEGFRAME_H
#define FFMPEGFRAME_H

#include "frame.h"

extern "C" {
#include <libavformat/avformat.h>
}

namespace MediaLibrary {

class FFmpegFrame : public Frame
{
public:
    FFmpegFrame();
    FFmpegFrame(const FFmpegFrame& ffmpegFrame);
    virtual ~FFmpegFrame();

    virtual Frame* copy() const override;
    virtual Frame* convert(const FrameType& frameType) const override;
    virtual Frame* resize(const MSize &size) const override;

    virtual void copy(Frame* frame) override;

    virtual void setFrame(void* frame) override { frame_ = static_cast<AVFrame*>(frame); }
    virtual void setTime(const int64_t time, const TimeBase& timeBase = {1, SECOND_TO_MILLISECOND_UNIT}) override;
    virtual void setDuration(const int64_t duration, const TimeBase& timeBase = {1, SECOND_TO_MILLISECOND_UNIT}) override;

    virtual void* getFrame() const override { return frame_; }
    virtual MSize getSize() const override;
    virtual int64_t getTime(const TimeBase& timeBase = {1, SECOND_TO_MILLISECOND_UNIT}) const override;
    virtual int64_t getDuration(const TimeBase& timeBase = {1, SECOND_TO_MILLISECOND_UNIT}) const override;

    static Frame* create(const MSize& size, const int pixelFormat);

private:
    AVFrame* frame_{nullptr};
};

}

#endif // FFMPEGFRAME_H
