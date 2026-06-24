#include "audiofilter.h"
#include "Model/ffmpegframe.h"

namespace MediaLibrary {

AudioFilter::AudioFilter()
{
}

bool AudioFilter::open(const std::string &description, const std::map<CodecKey, CodecValue> &inCodecInfos, const std::map<CodecKey, CodecValue> &outCodecInfos)
{
    int inSampleRate = 0;
    AVSampleFormat inSampleFormat = AV_SAMPLE_FMT_NONE;
    uint64_t inChannelLayout = 0;
    int inChannels = 0;
    for (auto inCodecInfo : inCodecInfos) {
        switch (inCodecInfo.first) {
        case CODEC_AUDIO_SAMPLE_FORMAT_EKY:
            inSampleFormat = static_cast<AVSampleFormat>(std::get<int>(inCodecInfo.second));
            break;
        case CODEC_AUDIO_SAMPLE_RATE_KEY:
            inSampleRate = std::get<int>(inCodecInfo.second);
            break;
        case CODEC_AUDIO_CHANNEL_LAYOUT_KEY:
            inChannelLayout = std::get<uint64_t>(inCodecInfo.second);
            break;
        case CODEC_AUDIO_CHANNELS_KEY:
            inChannels = std::get<int>(inCodecInfo.second);
            break;
        default:
            break;
        }
    }
    (void)inChannels;

    int outSampleRate = 0;
    AVSampleFormat outSampleFormat = AV_SAMPLE_FMT_NONE;
    uint64_t outChannelLayout = 0;
    int outChannels = 0;
    for (auto outCodecInfo : outCodecInfos) {
        switch (outCodecInfo.first) {
        case CODEC_AUDIO_SAMPLE_FORMAT_EKY:
            outSampleFormat = static_cast<AVSampleFormat>(std::get<int>(outCodecInfo.second));
            break;
        case CODEC_AUDIO_SAMPLE_RATE_KEY:
            outSampleRate = std::get<int>(outCodecInfo.second);
            break;
        case CODEC_AUDIO_CHANNEL_LAYOUT_KEY:
            outChannelLayout = std::get<uint64_t>(outCodecInfo.second);
            break;
        case CODEC_AUDIO_CHANNELS_KEY:
            outChannels = std::get<int>(outCodecInfo.second);
            break;
        default:
            break;
        }
    }
    (void)outChannels;

    AVFilterGraph* filterGraph = nullptr;
    AVFilterContext* bufferSrcCtx = nullptr;
    AVFilterContext* bufferSinkCtx = nullptr;
    AVFilter* buffersrc = const_cast<AVFilter*>(avfilter_get_by_name("abuffer"));
    AVFilter* buffersink = const_cast<AVFilter*>(avfilter_get_by_name("abuffersink"));
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
                "time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=0x%" PRIx64,
                1, AV_TIME_BASE, inSampleRate,
                av_get_sample_fmt_name(inSampleFormat),
                inChannelLayout);

        if ((ret = avfilter_graph_create_filter(&bufferSrcCtx, buffersrc, "in", args, nullptr, filterGraph)) < 0) {
            break;
        }

        if ((ret = avfilter_graph_create_filter(&bufferSinkCtx, buffersink, "out", nullptr, nullptr, filterGraph)) < 0) {
            break;
        }

        const enum AVSampleFormat outSampleFmts[] = {outSampleFormat, AV_SAMPLE_FMT_NONE};
        if ((ret = av_opt_set_int_list(bufferSinkCtx, "sample_fmts", outSampleFmts, -1, AV_OPT_SEARCH_CHILDREN)) < 0) {
            break;
        }
        const int64_t outChannelLayouts[] = {static_cast<int64_t>(outChannelLayout), -1};
        if ((ret = av_opt_set_int_list(bufferSinkCtx, "channel_layouts", outChannelLayouts, -1, AV_OPT_SEARCH_CHILDREN)) < 0) {
            break;
        }
        const int outSampleRates[] = {outSampleRate, -1};
        if ((ret = av_opt_set_int_list(bufferSinkCtx, "sample_rates", outSampleRates, -1, AV_OPT_SEARCH_CHILDREN)) < 0) {
            break;
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

void AudioFilter::close()
{
    if (filterGraph_) {
        avfilter_graph_free(&filterGraph_);
        filterGraph_ = nullptr;
        bufferSrcCtx_ = nullptr;
        bufferSinkCtx_ = nullptr;
    }
}

void AudioFilter::add(Frame *frame)
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

Frame *AudioFilter::get()
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
