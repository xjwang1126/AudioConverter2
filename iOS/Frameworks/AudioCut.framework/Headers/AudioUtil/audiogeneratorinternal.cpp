#include "audiogeneratorinternal.h"

#include "Common/Model/packetcontext.h"
#include "Common/Model/framecontext.h"
#include "Common/Model/ffmpegframe.h"
#include "Common/Media/mediacommon.h"
#include "Common/Media/mediademuxer.h"
#include "Common/Media/mediamuxer.h"
#include "Common/Codec/Decoder/audiodecoder.h"
#include "Common/Codec/Encoder/audioencoder.h"
#include "Common/Filter/AudioFilter/audiofilter.h"

namespace MediaLibrary {

AudioGeneratorInternal::AudioGeneratorInternal()
{
}

bool AudioGeneratorInternal::open(ReadCallback readCallback, SeekCallback demuxerSeekCallback, WriteCallback writeCallback, SeekCallback muxerSeekCallback, const string &formatName)
{
    readCallback_ = std::move(readCallback);
    demuxerSeekCallback_ = std::move(demuxerSeekCallback);
    wirteCallback_ = std::move(writeCallback);
    muxerSeekCallback_ = std::move(muxerSeekCallback);

    stop_ = false;

    do {
        mediaDemuxer_ = new MediaDemuxer();
        if (!mediaDemuxer_->open(this, readPacket, demuxerSeek, MediaDemuxer::CHECK_BASIC_TYPE)) {
            break;
        }

        FormatType formatType = FORMAT_NONE_TYPE;
        if (formatName == "mp3") {
            formatType = FORMAT_MP3_TYPE;
        } else if (formatName == "m4a") {
            formatType = FORMAT_M4A_TYPE;
        } else if (formatName == "flac") {
            formatType = FORMAT_FLAC_TYPE;
        } else if (formatName == "ogg") {
            formatType = FORMAT_OGG_TYPE;
        } else {
            break;
        }
        mediaMuxer_ = new MediaMuxer();
        if (!mediaMuxer_->open(formatType, this, writePacket, muxerSeek)) {
            break;
        }

        mediaMuxer_->setMuxerType(MediaMuxer::MUXER_FFMPEG_TYPE);

        noCodec_ = [this](){
            if (mediaDemuxer_->getAudioCodecId() != mediaMuxer_->getAudioCodecId()) {
                return false;
            }
            return true;
        }();

        if (noCodec_) {
            const int defaultAudioIndex = MediaLibrary::getDefaultAudioIndex(static_cast<AVFormatContext*>(mediaDemuxer_->getFormatContext()));
            if (defaultAudioIndex == -1) {
                break;
            }
            mediaMuxer_->addStream(mediaDemuxer_, defaultAudioIndex);
        } else {
            audioDecoderDelegate_ = new AudioDecoderDelegateImpl(this);
            audioDecoder_ = new AudioDecoder(audioDecoderDelegate_);
            if (!audioDecoder_->open(CODEC_SOFTWARE_TYPE, mediaDemuxer_)) {
                break;
            }

            audioEncoderDelegate_ = new AudioEncoderDelegateImpl(this);
            audioEncoder_ = new AudioEncoder(audioEncoderDelegate_);
            std::map<CodecKey, CodecValue> codecInfos = audioDecoder_->getCodecInfos();
            switch (mediaMuxer_->getFormatType()) {
            case FORMAT_MP3_TYPE: {
                codecInfos[CODEC_ID_KEY] = AV_CODEC_ID_MP3;
                codecInfos[CODEC_AUDIO_SAMPLE_FORMAT_EKY] = AV_SAMPLE_FMT_FLTP;
            }
                break;
            case FORMAT_FLAC_TYPE: {
                codecInfos[CODEC_ID_KEY] = AV_CODEC_ID_FLAC;
                codecInfos[CODEC_AUDIO_SAMPLE_FORMAT_EKY] = AV_SAMPLE_FMT_S16;
            }
                break;
            case FORMAT_OGG_TYPE: {
                codecInfos[CODEC_ID_KEY] = AV_CODEC_ID_VORBIS;
                codecInfos[CODEC_AUDIO_SAMPLE_FORMAT_EKY] = AV_SAMPLE_FMT_FLTP;
            }
                break;
            case FORMAT_WAV_TYPE: {
                codecInfos[CODEC_ID_KEY] = AV_CODEC_ID_PCM_S16LE;
                codecInfos[CODEC_AUDIO_SAMPLE_FORMAT_EKY] = AV_SAMPLE_FMT_S16;
            }
                break;
            case FORMAT_M4A_TYPE: {
                codecInfos[CODEC_ID_KEY] = AV_CODEC_ID_AAC;
                codecInfos[CODEC_AUDIO_SAMPLE_FORMAT_EKY] = AV_SAMPLE_FMT_FLTP;
            }
                break;
            default:
                break;
            }
            if (!audioEncoder_->open(CODEC_SOFTWARE_TYPE, mediaMuxer_, codecInfos)) {
                break;
            }

            audioFilter_ = new AudioFilter;
            string filterDescription;
            const std::map<CodecKey, CodecValue>& inCodecInfos = audioDecoder_->getCodecInfos();
            const std::map<CodecKey, CodecValue>& outCodecInfos = audioEncoder_->getCodecInfos();
            int samples = 1024;
            auto it = outCodecInfos.find(CODEC_AUDIO_SAMPLE_SIZE_KEY);
            if (it != outCodecInfos.cend()) {
                samples = std::get<int>(it->second);
            }
            if (samples == 0) {
                samples = 1024;
            }
            filterDescription += "asetnsamples=n=" + std::to_string(samples) + ":p=0";
            if (!audioFilter_->open(filterDescription, inCodecInfos, outCodecInfos)) {
                break;
            }
        }

        if (!mediaMuxer_->start()) {
            break;
        }

        return true;
    } while (false);

    return false;
}

void AudioGeneratorInternal::start()
{
    PacketContext* packetCtx = nullptr;
    while (true) {
        if (stop_) {
            break;
        }

        if (!packetCtx) {
            packetCtx = mediaDemuxer_->read();
        }
        if (!packetCtx) {
            break;
        }

        if (packetCtx->getPacketType() != PacketContext::PACKET_AUDIO_TYPE) {
            delete packetCtx;
            packetCtx = nullptr;
            continue;
        }

        if (noCodec_) {
            mediaMuxer_->write(packetCtx);
            delete packetCtx;
            packetCtx = nullptr;
        } else {
            if (audioDecoder_->decode(packetCtx)) {
                delete packetCtx;
                packetCtx = nullptr;
            }

            FrameContext* frameCtx = nullptr;
            {
                std::lock_guard<std::mutex> lock(audioDecoderBufferMutex_);
                (void)lock;
                if (!audioDecoderBuffer_.empty()) {
                    frameCtx = audioDecoderBuffer_.front();
                    audioDecoderBuffer_.pop();
                }
            }
            if (!frameCtx) {
                continue;
            }

            Frame* frame = frameCtx->getFrame();
            audioFilter_->add(frame);
            delete frameCtx;
            frameCtx = nullptr;

            Frame* filterFrame = audioFilter_->get();
            if (!filterFrame) {
                continue;
            }
            if (audioEncoder_->encode(filterFrame)) {
            }
            delete filterFrame;
            filterFrame = nullptr;

            packetCtx = nullptr;
            {
                std::lock_guard<std::mutex> lock(audioEncoderBufferMutex_);
                (void)lock;
                if (!audioEncoderBuffer_.empty()) {
                    packetCtx = audioEncoderBuffer_.front();
                    audioEncoderBuffer_.pop();
                }
            }
            if (!packetCtx) {
                continue;
            }
            mediaMuxer_->write(packetCtx);
            delete packetCtx;
            packetCtx = nullptr;
        }
    }

    if (!noCodec_) {
        audioDecoder_->decode(nullptr);

        do {
            if (stop_) {
                break;
            }

            FrameContext* frameCtx = nullptr;
            {
                std::lock_guard<std::mutex> lock(audioDecoderBufferMutex_);
                (void)lock;
                if (!audioDecoderBuffer_.empty()) {
                    frameCtx = audioDecoderBuffer_.front();
                    audioDecoderBuffer_.pop();
                }
            }
            if (!frameCtx) {
                break;
            }
            Frame* frame = frameCtx->getFrame();
            audioFilter_->add(frame);
            delete frameCtx;
            frameCtx = nullptr;
        } while (true);

        audioFilter_->add(nullptr);

        do {
            if (stop_) {
                break;
            }

            Frame* filterFrame = audioFilter_->get();
            if (!filterFrame) {
                break;
            }
            if (audioEncoder_->encode(filterFrame)) {
            }
            delete filterFrame;
            filterFrame = nullptr;
        } while (true);

        audioEncoder_->encode(nullptr);

        do {
            if (stop_) {
                break;
            }

            packetCtx = nullptr;
            {
                std::lock_guard<std::mutex> lock(audioEncoderBufferMutex_);
                (void)lock;
                if (!audioEncoderBuffer_.empty()) {
                    packetCtx = audioEncoderBuffer_.front();
                    audioEncoderBuffer_.pop();
                }
            }
            if (!packetCtx) {
                break;
            }
            mediaMuxer_->write(packetCtx);
            delete packetCtx;
            packetCtx = nullptr;
        } while (true);
    }
}

void AudioGeneratorInternal::stop()
{
    stop_ = true;
}

void AudioGeneratorInternal::close()
{
    if (audioDecoder_) {
        audioDecoder_->close();

        delete audioDecoder_;
        audioDecoder_ = nullptr;
    }

    if (audioDecoderDelegate_) {
        delete audioDecoderDelegate_;
        audioDecoderDelegate_ = nullptr;
    }

    if (mediaDemuxer_) {
        mediaDemuxer_->close();

        delete mediaDemuxer_;
        mediaDemuxer_ = nullptr;
    }

    if (audioEncoder_) {
        audioEncoder_->close();

        delete audioEncoder_;
        audioEncoder_ = nullptr;
    }

    if (audioEncoderDelegate_) {
        delete audioEncoderDelegate_;
        audioEncoderDelegate_ = nullptr;
    }

    if (mediaMuxer_) {
        mediaMuxer_->finish();
        mediaMuxer_->close();

        delete mediaMuxer_;
        mediaMuxer_ = nullptr;
    }
}

void AudioGeneratorInternal::decodeAudioFrame(FrameContext *frameCtx)
{
    std::lock_guard<std::mutex> guard(audioDecoderBufferMutex_);
    (void)guard;
    audioDecoderBuffer_.push(frameCtx);
}

void AudioGeneratorInternal::encodeAudioPacket(PacketContext *packetCtx)
{
    std::lock_guard<std::mutex> guard(audioEncoderBufferMutex_);
    (void)guard;
    audioEncoderBuffer_.push(packetCtx);
}

int AudioGeneratorInternal::readPacket(void* opaque, uint8_t* buffer, int bufferSize) {
    AudioGeneratorInternal* audioGeneratorInternal = static_cast<AudioGeneratorInternal*>(opaque);

    if (audioGeneratorInternal->readCallback_) {
        return audioGeneratorInternal->readCallback_(buffer, bufferSize);
    } else {
        return AVERROR_EOF;
    }
}

int64_t AudioGeneratorInternal::demuxerSeek(void* opaque, int64_t offset, int whence) {
    AudioGeneratorInternal* audioGeneratorInternal = static_cast<AudioGeneratorInternal*>(opaque);

    if (audioGeneratorInternal->demuxerSeekCallback_) {
        return audioGeneratorInternal->demuxerSeekCallback_(offset, whence);
    } else {
        return AVERROR(ESPIPE);
    }
}

int AudioGeneratorInternal::writePacket(void *opaque, uint8_t *buffer, int bufferSize)
{
    AudioGeneratorInternal* audioGeneratorInternal = static_cast<AudioGeneratorInternal*>(opaque);

    if (audioGeneratorInternal->wirteCallback_) {
        return audioGeneratorInternal->wirteCallback_(buffer, bufferSize);
    } else {
        return AVERROR_EOF;
    }
}

int64_t AudioGeneratorInternal::muxerSeek(void *opaque, int64_t offset, int whence)
{
    AudioGeneratorInternal* audioGeneratorInternal = static_cast<AudioGeneratorInternal*>(opaque);

    if (audioGeneratorInternal->muxerSeekCallback_) {
        return audioGeneratorInternal->muxerSeekCallback_(offset, whence);
    } else {
        return AVERROR(ESPIPE);
    }
}

AudioGeneratorInternal::AudioDecoderDelegateImpl::AudioDecoderDelegateImpl(AudioGeneratorInternal *internal)
    : internal_(internal)
{
}

AudioGeneratorInternal::AudioDecoderDelegateImpl::~AudioDecoderDelegateImpl()
{
    internal_ = nullptr;
}

void AudioGeneratorInternal::AudioDecoderDelegateImpl::output(FrameContext *frameCtx)
{
    if (internal_) {
        internal_->decodeAudioFrame(frameCtx);
    }
}

AudioGeneratorInternal::AudioEncoderDelegateImpl::AudioEncoderDelegateImpl(AudioGeneratorInternal *internal)
    : internal_(internal)
{
}

AudioGeneratorInternal::AudioEncoderDelegateImpl::~AudioEncoderDelegateImpl()
{
    internal_ = nullptr;
}

void AudioGeneratorInternal::AudioEncoderDelegateImpl::output(PacketContext *packetCtx)
{
    if (internal_) {
        internal_->encodeAudioPacket(packetCtx);
    }
}

}
