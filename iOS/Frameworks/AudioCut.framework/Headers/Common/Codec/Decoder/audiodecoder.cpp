#include "audiodecoder.h"
#include "Codec/codeccommon.h"
#include "Model/ffmpegframe.h"
#include "Model/framecontext.h"
#include "Model/packetcontext.h"
#include "Media/mediademuxer.h"

namespace MediaLibrary {

AudioDecoder::AudioDecoder(DecoderDelegate *delegate)
    : Decoder(delegate)
{
}

AudioDecoder::~AudioDecoder()
{
}

bool AudioDecoder::open(const CodecType &decoderType, MediaDemuxer *mediaDemuxer, const std::map<CodecKey, CodecValue> &codecInfos)
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

void AudioDecoder::close()
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

bool AudioDecoder::decode(PacketContext *packetCtx)
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

void AudioDecoder::flush()
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

bool AudioDecoder::openSoftwareDecoder()
{
    AVCodecParameters* codecPar = static_cast<AVCodecParameters*>(mediaDemuxer_->getAudioCodecPar());
    softwareDecoder_ = openDecoder(codecPar);
    codecInfos_[CODEC_AUDIO_SAMPLE_FORMAT_EKY] = codecPar->format;
    codecInfos_[CODEC_AUDIO_SAMPLE_RATE_KEY] = codecPar->sample_rate;
    codecInfos_[CODEC_AUDIO_CHANNEL_LAYOUT_KEY] = codecPar->channel_layout == 0 ? av_get_default_channel_layout(codecPar->channels) : codecPar->channel_layout;
    codecInfos_[CODEC_AUDIO_CHANNELS_KEY] = codecPar->channels;
    return softwareDecoder_ ? true : false;
}

void AudioDecoder::closeSoftwareDecoder()
{
    if (softwareDecoder_) {
        closeDecoder(softwareDecoder_);
        softwareDecoder_ = nullptr;
    }
}

bool AudioDecoder::softwareDecoderDecode(PacketContext *packetCtx)
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

void AudioDecoder::softwareDecoderFlush()
{
    if (softwareDecoder_) {
        avcodec_flush_buffers(softwareDecoder_);
    }
}

}
