#include "videoconverter.h"
#include "Model/ffmpegframe.h"

extern "C" {
#include <libavformat/avformat.h>

}

namespace MediaLibrary {

VideoConverter::VideoConverter()
{
}

bool VideoConverter::open(const std::map<CodecKey, CodecValue> &inCodecInfos, const std::map<CodecKey, CodecValue> &outCodecInfos)
{
    inCodecInfos_ = inCodecInfos;
    outCodecInfos_ = outCodecInfos;

    AVPixelFormat inPixelFormat = AV_PIX_FMT_NONE;
    int inWidth = 0, inHeight = 0;
    for (auto inCodecInfo : inCodecInfos_) {
        switch (inCodecInfo.first) {
        case CODEC_VIDEO_PIXEL_FORMAT_KEY:
            inPixelFormat = static_cast<AVPixelFormat>(std::get<int>(inCodecInfo.second));
            break;
        case CODEC_VIDEO_WIDTH_KEY:
            inWidth = std::get<int>(inCodecInfo.second);
            break;
        case CODEC_VIDEO_HEIGHT_KEY:
            inHeight = std::get<int>(inCodecInfo.second);
            break;
        default:
            break;
        }
    }

    AVPixelFormat outPixelFormat = AV_PIX_FMT_NONE;
    int outWidth = 0, outHeight = 0;
    for (auto outCodecInfo : outCodecInfos_) {
        switch (outCodecInfo.first) {
        case CODEC_VIDEO_PIXEL_FORMAT_KEY:
            outPixelFormat = static_cast<AVPixelFormat>(std::get<int>(outCodecInfo.second));
            break;
        case CODEC_VIDEO_WIDTH_KEY:
            outWidth = std::get<int>(outCodecInfo.second);
            break;
        case CODEC_VIDEO_HEIGHT_KEY:
            outHeight = std::get<int>(outCodecInfo.second);
            break;
        default:
            break;
        }
    }

    swsCtx_ = sws_getContext(inWidth,
                            inHeight,
                            inPixelFormat,
                            outWidth,
                            outHeight,
                            outPixelFormat,
                            SWS_FAST_BILINEAR,
                            nullptr,
                            nullptr,
                            nullptr);
    return swsCtx_;
}

void VideoConverter::close()
{
    if (swsCtx_) {
        sws_freeContext(swsCtx_);
        swsCtx_ = nullptr;
    }
}

Frame *VideoConverter::convert(Frame *frame)
{
    if (!frame) {
        return nullptr;
    }
    FFmpegFrame* ffmpegFrame = dynamic_cast<FFmpegFrame*>(frame);
    AVFrame* avFrame = static_cast<AVFrame*>(ffmpegFrame->getFrame());

    AVPixelFormat outPixelFormat = AV_PIX_FMT_NONE;
    int outWidth = 0, outHeight = 0;
    for (auto outCodecInfo : outCodecInfos_) {
        switch (outCodecInfo.first) {
        case CODEC_VIDEO_PIXEL_FORMAT_KEY:
            outPixelFormat = static_cast<AVPixelFormat>(std::get<int>(outCodecInfo.second));
            break;
        case CODEC_VIDEO_WIDTH_KEY:
            outWidth = std::get<int>(outCodecInfo.second);
            break;
        case CODEC_VIDEO_HEIGHT_KEY:
            outHeight = std::get<int>(outCodecInfo.second);
            break;
        default:
            break;
        }
    }

    AVFrame* convertAVFrame = av_frame_alloc();
    convertAVFrame->format = outPixelFormat;
    convertAVFrame->width = outWidth;
    convertAVFrame->height = outHeight;
    convertAVFrame->pts = avFrame->pts;
    convertAVFrame->pkt_dts = avFrame->pkt_dts;
    convertAVFrame->best_effort_timestamp = avFrame->best_effort_timestamp;
    convertAVFrame->pkt_duration = avFrame->pkt_duration;
    if (av_frame_get_buffer(convertAVFrame, 0) != 0) {
        return nullptr;
    }
    convertAVFrame->linesize[0] = convertAVFrame->width * 4;
    sws_scale(swsCtx_, static_cast<const unsigned char* const*>(avFrame->data), avFrame->linesize, 0,
              avFrame->height, convertAVFrame->data, convertAVFrame->linesize);

    FFmpegFrame* convertFFmpegFrame = new FFmpegFrame;
    convertFFmpegFrame->setFrame(convertAVFrame);
    convertFFmpegFrame->setTimeBase(TimeBase(1, SECOND_TO_MICROSECOND_UNIT));
    convertFFmpegFrame->setTime(frame->getTime());
    convertFFmpegFrame->setDuration(frame->getDuration());

    return convertFFmpegFrame;
}

}
