#include "codeccommon.h"

extern "C" {
#include <libavutil/opt.h>
}

#include <string>

namespace MediaLibrary {

AVCodecContext *openDecoder(const AVCodecParameters *codecPar)
{
    AVCodecContext* decoder = nullptr;

    if (!codecPar) {
        return decoder;
    }

    int ret = 0;
    do {
        AVCodec* codec = avcodec_find_decoder(codecPar->codec_id);
        if (!codec) {
            codec = avcodec_find_decoder(codecPar->codec_id);
        }
        if (!codec) {
            ret = AVERROR(ENOMEM);
            break;
        }

        decoder = avcodec_alloc_context3(codec);
        if (!decoder) {
            ret = AVERROR(ENOMEM);
            break;
        }

        if ((ret = avcodec_parameters_to_context(decoder, codecPar)) < 0) {
            break;
        }

        if ((ret = avcodec_open2(decoder, codec, nullptr)) < 0) {
            break;
        }
    } while(false);

    if (ret < 0) {
        closeDecoder(decoder);
        decoder = nullptr;
    }

    return decoder;
}

void closeDecoder(AVCodecContext *decoder)
{
    if (!decoder) {
        return;
    }

    avcodec_flush_buffers(decoder);
    avcodec_free_context(&decoder);
}

AVCodecContext *openVideoEncoder(AVFormatContext *formatCtx, const std::map<CodecKey, CodecValue> &codecInfos)
{
    AVCodecID codecId = AV_CODEC_ID_NONE;
    std::string codecName;
    AVPixelFormat pixelFormat = AV_PIX_FMT_NONE;
    int width = 0, height = 0;
    float fps = 0.f;
    for (auto codecInfo : codecInfos) {
        switch (codecInfo.first) {
        case CODEC_ID_KEY:
            codecId = static_cast<AVCodecID>(std::get<int>(codecInfo.second));
            break;
        case CODEC_NAME_KEY:
            codecName = std::get<std::string>(codecInfo.second);
            break;
        case CODEC_VIDEO_PIXEL_FORMAT_KEY:
            pixelFormat = static_cast<AVPixelFormat>(std::get<int>(codecInfo.second));
            break;
        case CODEC_VIDEO_WIDTH_KEY:
            width = std::get<int>(codecInfo.second);
            break;
        case CODEC_VIDEO_HEIGHT_KEY:
            height = std::get<int>(codecInfo.second);
            break;
        case CODEC_VIDEO_FPS_KEY:
            fps = std::get<float>(codecInfo.second);
            break;
        default:
            break;
        }
    }
    if (!formatCtx || codecId == AV_CODEC_ID_NONE || width == 0 || height == 0 || fps < 0.0f) {
        return nullptr;
    }

    AVCodecContext* encoder = nullptr;
    int ret = 0;
    do {
        AVStream* stream = avformat_new_stream(formatCtx, nullptr);
        if (!stream) {
            ret = -1;
            break;
        }

        AVCodec* codec = avcodec_find_encoder(codecId);
        if (!codec) {
            ret = -1;
            break;
        }

        encoder = avcodec_alloc_context3(codec);
        if (!encoder) {
            ret = -1;
            break;
        }

        encoder->width = width;
        encoder->height = height;
        encoder->pix_fmt = pixelFormat;
        encoder->time_base = {1000, static_cast<int>(fps * 1000)};

        if (formatCtx->oformat->flags & AVFMT_GLOBALHEADER) {
            encoder->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        }

        if ((ret = avcodec_open2(encoder, codec, nullptr)) < 0) {
            break;
        }

        if ((ret = avcodec_parameters_from_context(stream->codecpar, encoder)) < 0) {
            break;
        }

        stream->time_base = encoder->time_base;
    } while (false);

    if (ret < 0) {
        closeVideoEncoder(encoder);
        encoder = nullptr;
    }
    return encoder;
}

void closeVideoEncoder(AVCodecContext *videoEncoder)
{
    if (!videoEncoder) {
        return;
    }

    avcodec_close(videoEncoder);
    avcodec_free_context(&videoEncoder);
}

AVCodecContext *openAudioEncoder(AVFormatContext *formatCtx, const std::map<CodecKey, CodecValue> &codecInfos)
{
    AVCodecID codecId = AV_CODEC_ID_NONE;
    std::string codecName;
    AVSampleFormat sampleFormat = AV_SAMPLE_FMT_NONE;
    int sampleRate;
    uint64_t channelLayout;
    int channels;
    for (auto codecInfo : codecInfos) {
        switch (codecInfo.first) {
        case CODEC_ID_KEY:
            codecId = static_cast<AVCodecID>(std::get<int>(codecInfo.second));
            break;
        case CODEC_NAME_KEY:
            codecName = std::get<std::string>(codecInfo.second);
            break;
        case CODEC_AUDIO_SAMPLE_FORMAT_EKY:
            sampleFormat = static_cast<AVSampleFormat>(std::get<int>(codecInfo.second));
            break;
        case CODEC_AUDIO_SAMPLE_RATE_KEY:
            sampleRate = std::get<int>(codecInfo.second);
            break;
        case CODEC_AUDIO_CHANNEL_LAYOUT_KEY:
            channelLayout = std::get<uint64_t>(codecInfo.second);
            break;
        case CODEC_AUDIO_CHANNELS_KEY:
            channels = std::get<int>(codecInfo.second);
            break;
        default:
            break;
        }
    }
    if (!formatCtx || codecId == AV_CODEC_ID_NONE || sampleFormat == AV_SAMPLE_FMT_NONE) {
        return nullptr;
    }

    AVCodecContext* encoder = nullptr;
    int ret = 0;
    do {
        AVStream* stream = avformat_new_stream(formatCtx, nullptr);
        if (!stream) {
            ret = -1;
            break;
        }

        AVCodec* codec = avcodec_find_encoder(codecId);
        if (!codec) {
            ret = -1;
            break;
        }

        encoder = avcodec_alloc_context3(codec);
        if (!encoder) {
            ret = -1;
            break;
        }

        encoder->sample_fmt = sampleFormat;
        encoder->sample_rate = sampleRate;
        encoder->channel_layout = channelLayout;
        encoder->channels = channels;

        if (formatCtx->oformat->flags & AVFMT_GLOBALHEADER) {
            encoder->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        }

        if ((ret = avcodec_open2(encoder, codec, nullptr)) < 0) {
            break;
        }

        if ((ret = avcodec_parameters_from_context(stream->codecpar, encoder)) < 0) {
            break;
        }
    } while (false);

    if (ret < 0) {
        closeVideoEncoder(encoder);
        encoder = nullptr;
    }
    return encoder;
}

void closeAudioEncoder(AVCodecContext *audioEncoder)
{
    if (!audioEncoder) {
        return;
    }

    avcodec_close(audioEncoder);
    avcodec_free_context(&audioEncoder);
}

}
