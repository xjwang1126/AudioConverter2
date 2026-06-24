#include "videofilter.h"
#include "Model/ffmpegframe.h"

namespace MediaLibrary {

VideoFilter::VideoFilter()
{
}

bool VideoFilter::open(const std::string &description, const std::map<CodecKey, CodecValue> &inCodecInfos, const std::map<CodecKey, CodecValue> &outCodecInfos)
{
    AVPixelFormat inPixelFormat = AV_PIX_FMT_NONE;
    int inWidth = 0, inHeight = 0;
    for (auto inCodecInfo : inCodecInfos) {
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
    for (auto outCodecInfo : outCodecInfos) {
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
    (void)outWidth;(void)outHeight;

    AVFilterGraph* filterGraph = nullptr;
    AVFilterContext* bufferSrcCtx = nullptr;
    AVFilterContext* bufferSinkCtx = nullptr;
    AVFilter* buffersrc = const_cast<AVFilter*>(avfilter_get_by_name("buffer"));
    AVFilter* buffersink = const_cast<AVFilter*>(avfilter_get_by_name("buffersink"));
    AVFilterInOut* inputs = nullptr;
    AVFilterInOut* outputs = nullptr;
    int ret = 0;
    char args[512];

    do {
        inputs = avfilter_inout_alloc();
        outputs = avfilter_inout_alloc();
        filterGraph = avfilter_graph_alloc();
        if (!outputs || !inputs || !filterGraph) {
            ret = AVERROR(ENOMEM);
            break;
        }

        snprintf(args, sizeof(args),
                 "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=0/1",
                 inWidth, inHeight, inPixelFormat,
                 1, AV_TIME_BASE);

        if ((ret = avfilter_graph_create_filter(&bufferSrcCtx, buffersrc, "in", args, nullptr, filterGraph)) < 0) {
            break;
        }

        if ((ret = avfilter_graph_create_filter(&bufferSinkCtx, buffersink, "out", nullptr, nullptr, filterGraph)) < 0) {
            break;
        }

        if (outPixelFormat != AV_PIX_FMT_NONE) {
            enum AVPixelFormat pixelFormats[] = {outPixelFormat, AV_PIX_FMT_NONE};
            av_opt_set_int_list(bufferSinkCtx, "pix_fmts", pixelFormats, AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN);
            if (ret < 0) {
                ret = AVERROR_INVALIDDATA;
                break;
            }
        }

        outputs->name = av_strdup("in");
        outputs->filter_ctx = bufferSrcCtx;
        outputs->pad_idx = 0;
        outputs->next = nullptr;

        inputs->name = av_strdup("out");
        inputs->filter_ctx = bufferSinkCtx;
        inputs->pad_idx = 0;
        inputs->next = nullptr;

        if ((ret = avfilter_graph_parse_ptr(filterGraph, description.c_str(), &inputs, &outputs, nullptr)) < 0) {
            break;
        }

        if ((ret = avfilter_graph_config(filterGraph, nullptr)) < 0) {
            break;
        }
    } while(false);

    if (inputs) {
        avfilter_inout_free(&inputs);
        inputs = nullptr;
    }
    if (outputs) {
        avfilter_inout_free(&outputs);
        outputs = nullptr;
    }

    if (ret == 0) {
        filterGraph_ = filterGraph;
        bufferSrcCtx_ = bufferSrcCtx;
        bufferSinkCtx_ = bufferSinkCtx;

        return true;
    } else {
        avfilter_graph_free(&filterGraph);

        return false;
    }
}

void VideoFilter::close()
{
    if (filterGraph_) {
        avfilter_graph_free(&filterGraph_);
        filterGraph_ = nullptr;
        bufferSrcCtx_ = nullptr;
        bufferSinkCtx_ = nullptr;
    }
}

void VideoFilter::add(Frame *frame)
{
    AVFrame* avFrame = nullptr;
    if (frame) {
        FFmpegFrame* ffmpegFrame = dynamic_cast<FFmpegFrame*>(frame);
        avFrame = static_cast<AVFrame*>(ffmpegFrame->getFrame());
    }
    int ret = 0;
    if ((ret = av_buffersrc_add_frame_flags(bufferSrcCtx_, avFrame, AV_BUFFERSRC_FLAG_KEEP_REF)) < 0) {
    }
    (void)ret;
}

Frame *VideoFilter::get()
{
    AVFrame* avFrame = av_frame_alloc();
    int ret = 0;
    if ((ret = av_buffersink_get_frame(bufferSinkCtx_, avFrame)) < 0) {
        av_frame_free(&avFrame);
        return nullptr;
    }
    (void)ret;
    FFmpegFrame* ffmpegFrame = new FFmpegFrame;
    ffmpegFrame->setFrame(avFrame);
    ffmpegFrame->setTimeBase(TimeBase(1, SECOND_TO_MICROSECOND_UNIT));
    const AVRational& timeBase = bufferSinkCtx_->inputs[0]->time_base;
    ffmpegFrame->setTime(avFrame->pts, TimeBase(timeBase.num, timeBase.den));

    return ffmpegFrame;
}

}
