#include "ffmpegframe.h"
#include "imageframe.h"

#include <cassert>

extern "C" {
#include <libswscale/swscale.h>
#include <libavutil/pixfmt.h>
#include <libavutil/pixdesc.h>
}

namespace MediaLibrary {

FFmpegFrame::FFmpegFrame()
    : Frame(FRAME_FFMPEG_TYPE)
{
}

FFmpegFrame::FFmpegFrame(const FFmpegFrame &ffmpegFrame)
    : Frame(ffmpegFrame)
{
    frame_ = ffmpegFrame.frame_ ? av_frame_clone(ffmpegFrame.frame_) : nullptr;
}

FFmpegFrame::~FFmpegFrame()
{
    if (frame_) {
        av_frame_free(&frame_);
        frame_ = nullptr;
    }
}

Frame *FFmpegFrame::copy() const
{
    return new FFmpegFrame(*this);
}

Frame *FFmpegFrame::convert(const FrameType &frameType) const
{
    switch (frameType) {
    case FRAME_IMAGE_TYPE:
    {
        SwsContext* swsCtx = nullptr;
        AVFrame* convertAVFrame = nullptr;
        uint8_t* frame = nullptr;
        ImageFrame* imageFrame = nullptr;

        do {
            AVFrame* avFrame = static_cast<AVFrame*>(getFrame());
            if (!avFrame) {
                break;
            }

            const int width = avFrame->width;
            const int height = avFrame->height;
            swsCtx = sws_getContext(width,
                                    height,
                                    static_cast<AVPixelFormat>(avFrame->format),
                                    width,
                                    height,
                                    AV_PIX_FMT_RGBA,
                                    SWS_BICUBIC, nullptr, nullptr, nullptr);
            if (!swsCtx) {
                break;
            }
            convertAVFrame = av_frame_alloc();
            convertAVFrame->format = AV_PIX_FMT_RGBA;
            convertAVFrame->width = width;
            convertAVFrame->height = height;
            if (av_frame_get_buffer(convertAVFrame, 0) != 0) {
                break;
            }
            convertAVFrame->linesize[0] = convertAVFrame->width * 4;
            sws_scale(swsCtx, static_cast<const unsigned char* const*>(avFrame->data), avFrame->linesize, 0,
                      height, convertAVFrame->data, convertAVFrame->linesize);
            size_t frameDataSize = width * height * 4;
            frame = new uint8_t[frameDataSize];
            memcpy(frame, convertAVFrame->data[0], frameDataSize);

            imageFrame = new ImageFrame;
            imageFrame->setFrame(frame);
            imageFrame->setSize(MSize(width, height));
            imageFrame->setTimeBase(TimeBase(1, SECOND_TO_MICROSECOND_UNIT));
            imageFrame->setTime(getTime());
            imageFrame->setDuration(getDuration());
        } while (false);

        if (convertAVFrame) {
            av_frame_free(&convertAVFrame);
            convertAVFrame = nullptr;
        }

        if (swsCtx) {
            sws_freeContext(swsCtx);
            swsCtx = nullptr;
        }

        return imageFrame;
    }
        break;
    default:
        break;
    }

    return nullptr;
}

Frame *FFmpegFrame::resize(const MSize &size) const
{
    FFmpegFrame* ffmpegFrame = nullptr;
    SwsContext* swsCtx = nullptr;

    do {
        AVFrame* avFrame = static_cast<AVFrame*>(getFrame());
        if (!avFrame) {
            break;
        }

        MSize outSize;
        scaleSizeByMin(getSize(), size, &outSize);

        const int width = avFrame->width;
        const int height = avFrame->height;
        swsCtx = sws_getContext(width,
                                height,
                                static_cast<AVPixelFormat>(avFrame->format),
                                outSize.getWidth(),
                                outSize.getHeight(),
                                AV_PIX_FMT_RGBA,
                                SWS_BICUBIC, nullptr, nullptr, nullptr);
        if (!swsCtx) {
            break;
        }
        AVFrame* resizeAVFrame = av_frame_alloc();
        resizeAVFrame->format = AV_PIX_FMT_RGBA;
        resizeAVFrame->width = outSize.getWidth();
        resizeAVFrame->height = outSize.getHeight();
        if (av_frame_get_buffer(resizeAVFrame, 0) != 0) {
            av_frame_free(&resizeAVFrame);
            resizeAVFrame = nullptr;
            break;
        }
        resizeAVFrame->linesize[0] = resizeAVFrame->width * 4;
        sws_scale(swsCtx, static_cast<const unsigned char* const*>(avFrame->data), avFrame->linesize, 0,
                  height, resizeAVFrame->data, resizeAVFrame->linesize);

        ffmpegFrame = new FFmpegFrame;
        ffmpegFrame->setFrame(resizeAVFrame);
        ffmpegFrame->setSize(outSize);
        ffmpegFrame->setTimeBase(TimeBase(1, SECOND_TO_MICROSECOND_UNIT));
        ffmpegFrame->setTime(getTime());
        ffmpegFrame->setDuration(getDuration());
    } while (false);

    if (swsCtx) {
        sws_freeContext(swsCtx);
        swsCtx = nullptr;
    }

    return ffmpegFrame;
}

void FFmpegFrame::copy(Frame *frame)
{
    assert(getFrameType() == frame->getFrameType());
    AVFrame* srcFrame = static_cast<AVFrame*>(frame->getFrame());
    AVFrame* dstFrame = frame_;
    assert(dstFrame->format == srcFrame->format);

    if (srcFrame->format == AV_PIX_FMT_PAL8) {
        memcpy(dstFrame->data[1], srcFrame->data[1], AVPALETTE_SIZE);
    }

    const AVPixFmtDescriptor *pixDesc = av_pix_fmt_desc_get(static_cast<AVPixelFormat>(srcFrame->format));
    const int bytesPerPixel = av_get_bits_per_pixel(pixDesc) / 8;
    const int dY = (dstFrame->height - srcFrame->height) / 2;
    const int dX = (dstFrame->width - srcFrame->width) / 2;
    for (int y = 0; y < (srcFrame->height - (dY >= 0 ? 0 : abs(dY) * 2)); y++) {
        uint8_t *dst = dstFrame->data[0] + ((dY >= 0 ? dY : 0) + y) * dstFrame->linesize[0] + (dX >= 0 ? dX : 0) * bytesPerPixel;
        uint8_t *src = srcFrame->data[0] + ((dY >= 0 ? 0 : abs(dY)) + y) * srcFrame->linesize[0] + (dX >= 0 ? 0 : abs(dX)) * bytesPerPixel;
        memcpy(dst, src, (srcFrame->width - (dX >= 0 ? 0 : abs(dX) * 2)) * bytesPerPixel);
    }
}

void FFmpegFrame::setTime(const int64_t time, const TimeBase &timeBase)
{
    frame_->pts = (time * timeBase.num * timeBase_.den) / (timeBase.den * timeBase_.num);
}

void FFmpegFrame::setDuration(const int64_t duration, const TimeBase &timeBase)
{
    frame_->pkt_duration = (duration * timeBase.num * timeBase_.den) / (timeBase.den * timeBase_.num);
}

MSize FFmpegFrame::getSize() const
{
    return MSize(frame_->width, frame_->height);
}

int64_t FFmpegFrame::getTime(const TimeBase &timeBase) const
{
    return (frame_->pts * timeBase_.num * timeBase.den) / (timeBase_.den * timeBase.num);
}

int64_t FFmpegFrame::getDuration(const TimeBase &timeBase) const
{
    return (frame_->pkt_duration * timeBase_.num * timeBase.den) / (timeBase_.den * timeBase.num);
}

Frame *FFmpegFrame::create(const MSize &size, const int pixelFormat)
{
    FFmpegFrame* ffmpegFrame = nullptr;

    do {
        AVFrame* frame = av_frame_alloc();
        frame->format = pixelFormat;
        frame->width = size.getWidth();
        frame->height = size.getHeight();
        if (av_frame_get_buffer(frame, 0) != 0) {
            av_frame_free(&frame);
            frame = nullptr;
            break;
        }

        ffmpegFrame = new FFmpegFrame;
        ffmpegFrame->setFrame(frame);
        ffmpegFrame->setSize(size);
        ffmpegFrame->setTimeBase(TimeBase(1, SECOND_TO_MICROSECOND_UNIT));
    } while (false);

    return ffmpegFrame;
}

}
