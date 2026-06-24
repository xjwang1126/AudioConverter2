#include "audioconverter.h"
#include "Model/ffmpegframe.h"

namespace MediaLibrary {

AudioConverter::AudioConverter()
{
}

bool AudioConverter::open(const std::map<CodecKey, CodecValue> &inCodecInfos, const std::map<CodecKey, CodecValue> &outCodecInfos)
{
    inCodecInfos_ = inCodecInfos;
    outCodecInfos_ = outCodecInfos;

    int inSampleRate = 0;
    AVSampleFormat inSampleFormat = AV_SAMPLE_FMT_NONE;
    uint64_t inChannelLayout = 0;
    int inChannels = 0;
    for (auto inCodecInfo : inCodecInfos_) {
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
    for (auto outCodecInfo : outCodecInfos_) {
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

    swrCtx_ = swr_alloc_set_opts(
        nullptr,
        outChannelLayout,
        outSampleFormat,
        outSampleRate,
        inChannelLayout,
        inSampleFormat,
        inSampleRate,
        0,
        nullptr
    );
    if (!swrCtx_) {
        return false;
    }
    if (swr_init(swrCtx_) < 0) {
        return false;
    }
    return true;
}

void AudioConverter::close()
{
    if (swrCtx_) {
        swr_free(&swrCtx_);
        swrCtx_ = nullptr;
    }
}

Frame *AudioConverter::convert(Frame *frame)
{
    if (!frame) {
        return nullptr;
    }
    FFmpegFrame* ffmpegFrame = dynamic_cast<FFmpegFrame*>(frame);
    AVFrame* avFrame = static_cast<AVFrame*>(ffmpegFrame->getFrame());

    int outSampleRate = 0;
    AVSampleFormat outSampleFormat = AV_SAMPLE_FMT_NONE;
    uint64_t outChannelLayout = 0;
    int outChannels = 0;
    for (auto outCodecInfo : outCodecInfos_) {
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

    AVFrame* convertAVFrame = av_frame_alloc();
    convertAVFrame->format = outSampleFormat;
    convertAVFrame->channel_layout = outChannelLayout;
    convertAVFrame->sample_rate = outSampleRate;
    convertAVFrame->nb_samples = avFrame->nb_samples;
    convertAVFrame->pts = avFrame->pts;
    convertAVFrame->pkt_dts = avFrame->pkt_dts;
    convertAVFrame->best_effort_timestamp = avFrame->best_effort_timestamp;
    convertAVFrame->pkt_duration = avFrame->pkt_duration;
    if (av_frame_get_buffer(convertAVFrame, 0) != 0) {
        return nullptr;
    }
    if (swr_convert_frame(swrCtx_, convertAVFrame, avFrame) != 0) {
        return nullptr;
    }

    FFmpegFrame* convertFFmpegFrame = new FFmpegFrame;
    convertFFmpegFrame->setFrame(convertAVFrame);
    convertFFmpegFrame->setTimeBase(TimeBase(1, SECOND_TO_MICROSECOND_UNIT));
    convertFFmpegFrame->setTime(frame->getTime());
    convertFFmpegFrame->setDuration(frame->getDuration());

    return convertFFmpegFrame;
}

}
