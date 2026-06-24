#include "videodecoder.h"
#include "Codec/codeccommon.h"
#include "Model/ffmpegframe.h"
#include "Model/framecontext.h"
#include "Model/packetcontext.h"
#include "Media/mediademuxer.h"

namespace MediaLibrary {

VideoDecoder::VideoDecoder(DecoderDelegate *delegate)
    : Decoder(delegate)
{
}

VideoDecoder::~VideoDecoder()
{
}

bool VideoDecoder::open(const CodecType &decoderType, MediaDemuxer *mediaDemuxer, const std::map<CodecKey, CodecValue> &codecInfos)
{
    codecType_ = decoderType;
    mediaDemuxer_ = mediaDemuxer;

    switch (codecType_) {
    case CODEC_SOFTWARE_TYPE: {
        return openSoftwareDecoder();
    }
        break;
    case CODEC_HARDWARE_TYPE: {
        (void)codecInfos;
    }
        break;
    }

    return false;
}

void VideoDecoder::close()
{
    switch (codecType_) {
    case CODEC_SOFTWARE_TYPE: {
        closeSoftwareDecoder();
        softwareDecoder_ = nullptr;
    }
        break;
    case CODEC_HARDWARE_TYPE: {
    }
        break;
    }
}

bool VideoDecoder::decode(PacketContext *packetCtx)
{
    switch (codecType_) {
    case CODEC_SOFTWARE_TYPE: {
        return softwareDecoderDecode(packetCtx);
    }
        break;
    case CODEC_HARDWARE_TYPE: {
    }
        break;
    }

    return true;
}

void VideoDecoder::flush()
{
    switch (codecType_) {
    case CODEC_SOFTWARE_TYPE: {
        softwareDecoderFlush();
    }
        break;
    case CODEC_HARDWARE_TYPE: {
    }
        break;
    }
}

bool VideoDecoder::openSoftwareDecoder()
{
    AVCodecParameters* codecPar = static_cast<AVCodecParameters*>(mediaDemuxer_->getVideoCodecPar());
    softwareDecoder_ = openDecoder(codecPar);
    codecInfos_[CODEC_ID_KEY] = codecPar->codec_id;
    codecInfos_[CODEC_VIDEO_PIXEL_FORMAT_KEY] = codecPar->format;
    codecInfos_[CODEC_VIDEO_FPS_KEY] = mediaDemuxer_->getFPS();
    codecInfos_[CODEC_VIDEO_WIDTH_KEY] = static_cast<int>(mediaDemuxer_->getSize().getWidth());
    codecInfos_[CODEC_VIDEO_HEIGHT_KEY] = static_cast<int>(mediaDemuxer_->getSize().getHeight());
    return softwareDecoder_ ? true : false;
}

void VideoDecoder::closeSoftwareDecoder()
{
    if (softwareDecoder_) {
        closeDecoder(softwareDecoder_);
        softwareDecoder_ = nullptr;
    }
}

bool VideoDecoder::softwareDecoderDecode(PacketContext *packetCtx)
{
    AVPacket* packet = nullptr;
    if (packetCtx) {
        packet = packetCtx->getPacket();
    }
    int ret = avcodec_send_packet(softwareDecoder_, packet);
    bool addPacketFlag = (ret >= 0) || (ret == -1094995529);
    while (ret == AVERROR(EAGAIN) || ret >= 0 || ret == -1094995529) {
        AVFrame* frame = av_frame_alloc();
        if ((ret = avcodec_receive_frame(softwareDecoder_, frame)) < 0) {
            av_frame_free(&frame);
            break;
        }
        if (delegate_) {
            const int64_t frameTime = static_cast<int64_t>(roundf(frame->pts * SECOND_TO_MILLISECOND_UNIT / (AV_TIME_BASE * 1.f)));
            const int64_t nextFrameTime = static_cast<int64_t>(roundf((frame->pts + frame->pkt_duration) * SECOND_TO_MILLISECOND_UNIT / (AV_TIME_BASE * 1.f)));
            const int64_t frameDuration = nextFrameTime - frameTime;

            FFmpegFrame* ffmpegFrame = new FFmpegFrame;
            ffmpegFrame->setFrame(av_frame_clone(frame));
            ffmpegFrame->setTimeBase(TimeBase(1, SECOND_TO_MICROSECOND_UNIT));
            ffmpegFrame->setTime(frameTime);
            ffmpegFrame->setDuration(frameDuration);

            FrameContext* frameCtx = new FrameContext();
            frameCtx->setFrame(ffmpegFrame);
            delegate_->output(frameCtx);
        }

        av_frame_free(&frame);
    }

    return addPacketFlag;
}

void VideoDecoder::softwareDecoderFlush()
{
    if (softwareDecoder_) {
        avcodec_flush_buffers(softwareDecoder_);
    }
}

}
