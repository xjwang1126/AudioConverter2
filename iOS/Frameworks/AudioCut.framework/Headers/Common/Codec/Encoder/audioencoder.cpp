#include "audioencoder.h"
#include "Codec/codeccommon.h"
#include "Model/ffmpegframe.h"
#include "Model/packetcontext.h"
#include "Media/mediacommon.h"

#include <cassert>

namespace MediaLibrary {

AudioEncoder::AudioEncoder(EncoderDelegate *delegate)
    : Encoder(delegate)
{
}

AudioEncoder::~AudioEncoder()
{
}

bool AudioEncoder::open(const CodecType &codecType, MediaMuxer *mediaMuxer, const std::map<CodecKey, CodecValue> &codecInfos)
{
    codecType_ = codecType;
    mediaMuxer_ = mediaMuxer;
    codecInfos_ = codecInfos;

    switch (codecType_) {
    case CODEC_SOFTWARE_TYPE:
        return openSoftwareEncoder(codecInfos);
        break;
    case CODEC_HARDWARE_TYPE:
        break;
    default:
        break;
    }

    return false;
}

void AudioEncoder::close()
{
    switch (codecType_) {
    case CODEC_SOFTWARE_TYPE:
        closeSoftwareEncoder();
        break;
    case CODEC_HARDWARE_TYPE:
        break;
    default:
        break;
    }
}

bool AudioEncoder::encode(Frame *frame)
{
    switch (codecType_) {
    case CODEC_SOFTWARE_TYPE:
        return softwareEncoderEncode(frame);
        break;
    case CODEC_HARDWARE_TYPE:
        break;
    default:
        break;
    }

    return true;
}

bool AudioEncoder::openSoftwareEncoder(const std::map<CodecKey, CodecValue> &codecInfos)
{
    softwareEncoder_ = openAudioEncoder(static_cast<AVFormatContext*>(mediaMuxer_->getFormatCtx()), codecInfos);
    if (softwareEncoder_) {
        codecInfos_[CODEC_AUDIO_SAMPLE_SIZE_KEY] = static_cast<AVCodecContext*>(softwareEncoder_)->frame_size;
    } else {
        codecInfos_[CODEC_AUDIO_SAMPLE_SIZE_KEY] = 1024;
    }
    return softwareEncoder_ ? true : false;
}

void AudioEncoder::closeSoftwareEncoder()
{
    if (!softwareEncoder_) {
        return;
    }
    closeAudioEncoder(static_cast<AVCodecContext*>(softwareEncoder_));
    softwareEncoder_ = nullptr;
}

bool AudioEncoder::softwareEncoderEncode(Frame *frame)
{
    bool isEncodeEnd = false;
    int ret = 0;

    AVCodecContext* softwareEncoder = static_cast<AVCodecContext*>(softwareEncoder_);
    if (frame) {
        FFmpegFrame* ffmpegFrame = dynamic_cast<FFmpegFrame*>(frame);
        if (!ffmpegFrame) {
            assert(0);
        }
        AVFrame* avFrame = static_cast<AVFrame*>(ffmpegFrame->getFrame());
        ret = avcodec_send_frame(softwareEncoder, avFrame);
    } else {
        ret = avcodec_send_frame(softwareEncoder, nullptr);
    }
    while (ret >= 0) {
        AVPacket* packet = av_packet_alloc();
        if ((ret = avcodec_receive_packet(softwareEncoder, packet)) < 0) {
            if (ret == AVERROR_EOF) {
                isEncodeEnd = true;
            }
            av_packet_free(&packet);
            break;
        }

        if (delegate_) {
            PacketContext* packetCtx = new PacketContext();
            packetCtx->setPacketType(PacketContext::PACKET_AUDIO_TYPE);
            packetCtx->setPacket(packet);
            delegate_->output(packetCtx);
        } else {
            av_packet_free(&packet);
        }
    }
    return isEncodeEnd;
}

}
