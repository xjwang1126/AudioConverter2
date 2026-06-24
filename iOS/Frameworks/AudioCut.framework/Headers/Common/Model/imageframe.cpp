#include "imageframe.h"
#include "ffmpegframe.h"

#include <cstring>
#include <cassert>

extern "C" {
#include <libavutil/imgutils.h>
}

namespace MediaLibrary {

ImageFrame::ImageFrame()
    : Frame(Frame::FRAME_IMAGE_TYPE)
{
}

ImageFrame::ImageFrame(const ImageFrame &imageFrame)
    : Frame(imageFrame)
{
    if (imageFrame.frame_) {
        const int frameDataSize = imageFrame.size_.getWidth() * imageFrame.size_.getHeight() * 4;
        frame_ = new unsigned char[frameDataSize];
        memcpy(frame_, imageFrame.frame_, static_cast<size_t>(frameDataSize));
    }
    size_ = imageFrame.size_;
    time_ = imageFrame.time_;
    duration_ = imageFrame.duration_;
}

ImageFrame::~ImageFrame()
{
    if (frame_) {
        delete[] frame_;
        frame_ = nullptr;
    }
    size_ = MSize();
    time_ = -1;
    duration_ = 0;
}

Frame *ImageFrame::copy() const
{
    return new ImageFrame(*this);
}

Frame *ImageFrame::convert(const FrameType &frameType) const
{
    switch (frameType) {
    case FRAME_FFMPEG_TYPE:
    {
        if (!frame_) {
            break;
        }

        const MSize size = size_;
        const int width = size.getWidth();
        const int height = size.getHeight();
        AVFrame* frame = av_frame_alloc();
        frame->width = width;
        frame->height = height;
        frame->format = AV_PIX_FMT_RGBA;
        av_frame_get_buffer(frame, 0);

        uint8_t* data[4];
        int linesize[4];
        data[0] = frame_;
        linesize[0] = width * 4;
        av_image_copy(frame->data, frame->linesize, const_cast<const uint8_t**>(static_cast<uint8_t**>(data)), linesize, static_cast<AVPixelFormat>(frame->format), frame->width, frame->height);

        FFmpegFrame* ffmpegFrame = new FFmpegFrame;
        ffmpegFrame->setFrame(frame);
        ffmpegFrame->setTimeBase(TimeBase(1, SECOND_TO_MICROSECOND_UNIT));
        ffmpegFrame->setTime(getTime());
        ffmpegFrame->setDuration(getDuration());

        return ffmpegFrame;
    }
        break;
    default:
        break;
    }

    return nullptr;
}

Frame *ImageFrame::resize(const MSize &size) const
{
    Frame* convertFrame = ImageFrame::convert(FRAME_FFMPEG_TYPE);
    Frame* resizeFrame = convertFrame->resize(size);
    Frame* imageFrame = resizeFrame->convert(FRAME_IMAGE_TYPE);
    delete convertFrame;
    convertFrame = nullptr;
    delete resizeFrame;
    resizeFrame = nullptr;
    return imageFrame;
}

void ImageFrame::copy(Frame *frame)
{
    (void)frame;
}

void ImageFrame::setTime(const int64_t time, const TimeBase &timeBase)
{
    time_ = (time * timeBase.num * timeBase_.den) / (timeBase.den * timeBase_.num);
}

void ImageFrame::setDuration(const int64_t duration, const TimeBase &timeBase)
{
    duration_ = (duration * timeBase.num * timeBase_.den) / (timeBase.den * timeBase_.num);
}

int64_t ImageFrame::getTime(const TimeBase &timeBase) const
{
    return (time_ * timeBase_.num * timeBase.den) / (timeBase_.den * timeBase.num);
}

int64_t ImageFrame::getDuration(const TimeBase &timeBase) const
{
    return (duration_ * timeBase_.num * timeBase.den) / (timeBase_.den * timeBase.num);
}

}
