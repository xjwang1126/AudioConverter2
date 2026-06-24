#include "videoencoder.h"
#include "Codec/codeccommon.h"
#include "Model/ffmpegframe.h"
#include "Model/packetcontext.h"
#include "Media/mediacommon.h"

#include <cassert>

namespace MediaLibrary {

VideoEncoder::VideoEncoder(EncoderDelegate *delegate)
    : Encoder(delegate)
{
}

VideoEncoder::~VideoEncoder()
{
}

bool VideoEncoder::open(const CodecType &codecType, MediaMuxer *mediaMuxer, const std::map<CodecKey, CodecValue> &codecInfos)
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

void VideoEncoder::close()
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

bool VideoEncoder::encode(Frame *frame)
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

bool VideoEncoder::openSoftwareEncoder(const std::map<CodecKey, CodecValue> &codecInfos)
{
    softwareEncoder_ = openVideoEncoder(static_cast<AVFormatContext*>(mediaMuxer_->getFormatCtx()), codecInfos);
    return softwareEncoder_ ? true : false;
}

void VideoEncoder::closeSoftwareEncoder()
{
    if (!softwareEncoder_) {
        return;
    }
    closeVideoEncoder(static_cast<AVCodecContext*>(softwareEncoder_));
    softwareEncoder_ = nullptr;
}

bool VideoEncoder::softwareEncoderEncode(Frame *frame)
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
            packetCtx->setPacketType(PacketContext::PACKET_VIDEO_TYPE);
            packetCtx->setPacket(packet);
            delegate_->output(packetCtx);
        } else {
            av_packet_free(&packet);
        }
    }
    return isEncodeEnd;
}

}
