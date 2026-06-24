#include "audioconcatinternal.h"

#include "Common/Model/packetcontext.h"
#include "Common/Model/framecontext.h"
#include "Common/Model/ffmpegframe.h"
#include "Common/Media/mediacommon.h"
#include "Common/Media/mediademuxer.h"
#include "Common/Media/mediamuxer.h"
#include "Common/Codec/Decoder/audiodecoder.h"
#include "Common/Codec/Encoder/audioencoder.h"
#include "Common/Filter/AudioFilter/audiofilter.h"

#include <cassert>

namespace MediaLibrary {

AudioConcatInternal::AudioConcatInternal()
{
}

int AudioConcatInternal::open()
{
    int error = 0;

    cancel_ = false;

    do {
        FormatType formatType = FORMAT_MP3_TYPE;
        mediaMuxer_ = new MediaMuxer();
        if (!mediaMuxer_->open(formatType)) {
            error = 1;
            break;
        }

        if (!(mediaMuxer_->getMediaType() == MEDIA_VIDEO_AUDIO_TYPE)) {
            error = 2;
            break;
        }

        mediaMuxer_->setMuxerType(MediaMuxer::MUXER_FFMPEG_TYPE);

        audioEncoderDelegate_ = new AudioEncoderDelegateImpl(this);
        audioEncoder_ = new AudioEncoder(audioEncoderDelegate_);
        std::map<CodecKey, CodecValue> codecInfos;
        codecInfos[CODEC_AUDIO_SAMPLE_RATE_KEY] = 48000;
        codecInfos[CODEC_AUDIO_CHANNEL_LAYOUT_KEY] = static_cast<uint64_t>(av_get_default_channel_layout(1));
        codecInfos[CODEC_AUDIO_CHANNELS_KEY] = 1;
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
            error = 3;
            break;
        }

        if (!mediaMuxer_->start()) {
            error = 4;
            break;
        }
    } while (false);

    return error;
}

int AudioConcatInternal::append(char *inData, int inDataSize)
{
    int error = 0;

    cancel_ = false;

    do {
        mediaDemuxer_ = new MediaDemuxer();
        if (!mediaDemuxer_->open2(inData, inDataSize)) {
            error = 1;
            break;
        }

        if (!(mediaDemuxer_->getMediaType() == MEDIA_VIDEO_AUDIO_TYPE)) {
            error = 2;
            break;
        }

        audioDecoderDelegate_ = new AudioDecoderDelegateImpl(this);
        audioDecoder_ = new AudioDecoder(audioDecoderDelegate_);
        if (!audioDecoder_->open(CODEC_SOFTWARE_TYPE, mediaDemuxer_)) {
            error = 3;
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
            error = 4;
            break;
        }

        PacketContext* packetCtx = nullptr;
        while (true) {
            if (cancel_) {
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
            Frame* copyFrame = frame->copy();
            assert(copyFrame);
            const int64_t frameTime = frameCtx->getTime();
            copyFrame->setTime(frameTime);
            audioFilter_->add(copyFrame);
            delete copyFrame;
            copyFrame = nullptr;

            delete frameCtx;
            frameCtx = nullptr;

            Frame* filterFrame = audioFilter_->get();
            if (!filterFrame) {
                continue;
            }
            filterFrame->setTime(filterFrame->getTime() + offsetTime_);
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

        audioFilter_->add(nullptr);

        do {
            if (cancel_) {
                break;
            }

            Frame* filterFrame = audioFilter_->get();
            if (!filterFrame) {
                break;
            }
            filterFrame->setTime(filterFrame->getTime() + offsetTime_);
            if (audioEncoder_->encode(filterFrame)) {
            }
            delete filterFrame;
            filterFrame = nullptr;
        } while (true);

        if (cancel_) {
            break;
        }

        offsetTime_ += mediaDemuxer_->getDuration();
    } while (false);

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

    if (audioFilter_) {
        audioFilter_->close();

        delete audioFilter_;
        audioFilter_ = nullptr;
    }

    return error;
}

int AudioConcatInternal::append2(char *inData, int inDataSize)
{
    int error = 0;

    cancel_ = false;
    end_ = false;

    do {
        mediaDemuxer_ = new MediaDemuxer();
        if (!mediaDemuxer_->open2(inData, inDataSize)) {
            error = 1;
            break;
        }

        if (!(mediaDemuxer_->getMediaType() == MEDIA_VIDEO_AUDIO_TYPE)) {
            error = 2;
            break;
        }

        audioDecoderDelegate_ = new AudioDecoderDelegateImpl(this);
        audioDecoder_ = new AudioDecoder(audioDecoderDelegate_);
        if (!audioDecoder_->open(CODEC_SOFTWARE_TYPE, mediaDemuxer_)) {
            error = 3;
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
            error = 4;
            break;
        }
    } while (false);

    return error;
}

void AudioConcatInternal::process2(bool *end, int *progress)
{
    const std::chrono::steady_clock::time_point beginTime = std::chrono::steady_clock::now();
    bool processTimeMeet = false;
    int64_t requestTime = 0;

    do {
        do {
            PacketContext* packetCtx = nullptr;
            while (true) {
                const std::chrono::steady_clock::time_point endTime = std::chrono::steady_clock::now();
                std::chrono::duration<double, std::milli> processTime = endTime - beginTime;
                if (processTime.count() > 200) {
                    processTimeMeet = true;
                    break;
                }

                if (cancel_) {
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
                Frame* copyFrame = frame->copy();
                assert(copyFrame);
                const int64_t frameTime = frameCtx->getTime();
                copyFrame->setTime(frameTime);
                audioFilter_->add(copyFrame);
                delete copyFrame;
                copyFrame = nullptr;

                requestTime = frameTime;

                delete frameCtx;
                frameCtx = nullptr;

                Frame* filterFrame = audioFilter_->get();
                if (!filterFrame) {
                    continue;
                }
                filterFrame->setTime(filterFrame->getTime() + offsetTime_);
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

            if (processTimeMeet) {
                break;
            }

            audioFilter_->add(nullptr);

            do {
                if (cancel_) {
                    break;
                }

                Frame* filterFrame = audioFilter_->get();
                if (!filterFrame) {
                    break;
                }
                filterFrame->setTime(filterFrame->getTime() + offsetTime_);
                if (audioEncoder_->encode(filterFrame)) {
                }
                delete filterFrame;
                filterFrame = nullptr;
            } while (true);

            if (cancel_) {
                break;
            }

            offsetTime_ += mediaDemuxer_->getDuration();
        } while (false);

        if (processTimeMeet) {
            break;
        }

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

        if (audioFilter_) {
            audioFilter_->close();

            delete audioFilter_;
            audioFilter_ = nullptr;
        }

        end_ = true;
    } while (false);

    *end = end_;
    *progress = end_ ? 100 : std::max(std::min((int)((requestTime * 1.f / mediaDemuxer_->getDuration()) * 100), 100), 0);
}

void AudioConcatInternal::cancel()
{
    cancel_ = true;
}

void AudioConcatInternal::close()
{
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

void AudioConcatInternal::getFileData(char **data, int *dataSize)
{
    if (mediaMuxer_) {
        audioEncoder_->encode(nullptr);

        do {
            if (cancel_) {
                break;
            }

            PacketContext* packetCtx = nullptr;
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

        mediaMuxer_->finish();

        const BufferData& bufferData = mediaMuxer_->getBufferData();
        *data = (char*)bufferData.buf;
        *dataSize = bufferData.size - bufferData.room;
    } else {
        *data = nullptr;
        *dataSize = 0;
    }
}

void AudioConcatInternal::getFileData(char **data, int *dataSize, bool *end, int *progress, int readDataSize)
{
    if (mediaMuxer_) {
        if (readDataSize == 0) {
            audioEncoder_->encode(nullptr);

            do {
                if (cancel_) {
                    break;
                }

                PacketContext* packetCtx = nullptr;
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

            mediaMuxer_->finish();
        }

        const BufferData& bufferData = mediaMuxer_->getBufferData();
        *data = (char*)bufferData.buf + readDataSize;
        *dataSize = std::min((int)(bufferData.size - bufferData.room - readDataSize), 2 * 1024 * 1024);
        *end = readDataSize >= (int)(bufferData.size - bufferData.room);
        *progress = *end ? 100 : std::max(std::min((int)((readDataSize * 1.f / (int)(bufferData.size - bufferData.room)) * 100), 100), 0);
    } else {
        *data = nullptr;
        *dataSize = 0;
    }
}

void AudioConcatInternal::decodeAudioFrame(FrameContext *frameCtx)
{
    std::lock_guard<std::mutex> guard(audioDecoderBufferMutex_);
    (void)guard;
    audioDecoderBuffer_.push(frameCtx);
}

void AudioConcatInternal::encodeAudioPacket(PacketContext *packetCtx)
{
    std::lock_guard<std::mutex> guard(audioEncoderBufferMutex_);
    (void)guard;
    audioEncoderBuffer_.push(packetCtx);
}

AudioConcatInternal::AudioDecoderDelegateImpl::AudioDecoderDelegateImpl(AudioConcatInternal *internal)
    : internal_(internal)
{
}

AudioConcatInternal::AudioDecoderDelegateImpl::~AudioDecoderDelegateImpl()
{
    internal_ = nullptr;
}

void AudioConcatInternal::AudioDecoderDelegateImpl::output(FrameContext *frameCtx)
{
    if (internal_) {
        internal_->decodeAudioFrame(frameCtx);
    }
}

AudioConcatInternal::AudioEncoderDelegateImpl::AudioEncoderDelegateImpl(AudioConcatInternal *internal)
    : internal_(internal)
{
}

AudioConcatInternal::AudioEncoderDelegateImpl::~AudioEncoderDelegateImpl()
{
    internal_ = nullptr;
}

void AudioConcatInternal::AudioEncoderDelegateImpl::output(PacketContext *packetCtx)
{
    if (internal_) {
        internal_->encodeAudioPacket(packetCtx);
    }
}

}
