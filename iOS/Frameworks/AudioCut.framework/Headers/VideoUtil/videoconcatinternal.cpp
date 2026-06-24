#include "videoconcatinternal.h"

#include "Common/Model/packetcontext.h"
#include "Common/Model/framecontext.h"
#include "Common/Model/ffmpegframe.h"
#include "Common/Media/mediacommon.h"
#include "Common/Media/mediademuxer.h"
#include "Common/Media/mediamuxer.h"
#include "Common/Codec/Decoder/audiodecoder.h"
#include "Common/Codec/Decoder/videodecoder.h"
#include "Common/Codec/Encoder/audioencoder.h"
#include "Common/Codec/Encoder/videoencoder.h"
#include "Common/Filter/AudioFilter/audiofilter.h"
#include "Common/Filter/VideoFilter/videofilter.h"

#include <cassert>

namespace MediaLibrary {

VideoConcatInternal::VideoConcatInternal()
{
}

int VideoConcatInternal::open()
{
    int error = 0;

    cancel_ = false;

    do {
        FormatType formatType = FORMAT_MP4_TYPE;
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

        videoEncoderDelegate_ = new VideoEncoderDelegateImpl(this);
        videoEncoder_ = new VideoEncoder(videoEncoderDelegate_);

        std::map<CodecKey, CodecValue> audioCodecInfos;
        audioCodecInfos[CODEC_AUDIO_SAMPLE_RATE_KEY] = 48000;
        audioCodecInfos[CODEC_AUDIO_CHANNEL_LAYOUT_KEY] = static_cast<uint64_t>(av_get_default_channel_layout(1));
        audioCodecInfos[CODEC_AUDIO_CHANNELS_KEY] = 1;

        std::map<CodecKey, CodecValue> videoCodecInfos;
        videoCodecInfos[CODEC_VIDEO_FPS_KEY] = 30.f;
        videoCodecInfos[CODEC_VIDEO_WIDTH_KEY] = 540;
        videoCodecInfos[CODEC_VIDEO_HEIGHT_KEY] = 960;

        switch (mediaMuxer_->getFormatType()) {
        case FORMAT_MP3_TYPE: {
            audioCodecInfos[CODEC_ID_KEY] = AV_CODEC_ID_MP3;
            audioCodecInfos[CODEC_AUDIO_SAMPLE_FORMAT_EKY] = AV_SAMPLE_FMT_FLTP;
        }
        break;
        case FORMAT_FLAC_TYPE: {
            audioCodecInfos[CODEC_ID_KEY] = AV_CODEC_ID_FLAC;
            audioCodecInfos[CODEC_AUDIO_SAMPLE_FORMAT_EKY] = AV_SAMPLE_FMT_S16;
        }
        break;
        case FORMAT_OGG_TYPE: {
            audioCodecInfos[CODEC_ID_KEY] = AV_CODEC_ID_VORBIS;
            audioCodecInfos[CODEC_AUDIO_SAMPLE_FORMAT_EKY] = AV_SAMPLE_FMT_FLTP;
        }
        break;
        case FORMAT_WAV_TYPE: {
            audioCodecInfos[CODEC_ID_KEY] = AV_CODEC_ID_PCM_S16LE;
            audioCodecInfos[CODEC_AUDIO_SAMPLE_FORMAT_EKY] = AV_SAMPLE_FMT_S16;
        }
        break;
        case FORMAT_M4A_TYPE: {
            audioCodecInfos[CODEC_ID_KEY] = AV_CODEC_ID_AAC;
            audioCodecInfos[CODEC_AUDIO_SAMPLE_FORMAT_EKY] = AV_SAMPLE_FMT_FLTP;
        }
        break;
        case FORMAT_MP4_TYPE: {
            audioCodecInfos[CODEC_ID_KEY] = AV_CODEC_ID_AAC;
            audioCodecInfos[CODEC_AUDIO_SAMPLE_FORMAT_EKY] = AV_SAMPLE_FMT_FLTP;

            videoCodecInfos[CODEC_ID_KEY] = AV_CODEC_ID_H264;
            videoCodecInfos[CODEC_VIDEO_PIXEL_FORMAT_KEY] = AV_PIX_FMT_YUV420P;
            videoCodecInfos[CODEC_VIDEO_WIDTH_KEY] = static_cast<int>(1080);
            videoCodecInfos[CODEC_VIDEO_HEIGHT_KEY] = static_cast<int>(1920);
            videoCodecInfos[CODEC_VIDEO_FPS_KEY] = 30.f;
        }
        break;
        default:
            break;
        }
        if (!audioEncoder_->open(CODEC_SOFTWARE_TYPE, mediaMuxer_, audioCodecInfos)) {
            error = 3;
            break;
        }
        if (!videoEncoder_->open(CODEC_SOFTWARE_TYPE, mediaMuxer_, videoCodecInfos)) {
            error = 4;
            break;
        }

        if (!mediaMuxer_->start()) {
            error = 5;
            break;
        }
    } while (false);

    return error;
}

int VideoConcatInternal::append(char *inData, int inDataSize)
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
        string audioFilterDescription;
        const std::map<CodecKey, CodecValue>& inAudioCodecInfos = audioDecoder_->getCodecInfos();
        const std::map<CodecKey, CodecValue>& outAudioCodecInfos = audioEncoder_->getCodecInfos();
        int samples = 1024;
        auto it = outAudioCodecInfos.find(CODEC_AUDIO_SAMPLE_SIZE_KEY);
        if (it != outAudioCodecInfos.cend()) {
            samples = std::get<int>(it->second);
        }
        if (samples == 0) {
            samples = 1024;
        }
        audioFilterDescription += "asetnsamples=n=" + std::to_string(samples) + ":p=0";
        if (!audioFilter_->open(audioFilterDescription, inAudioCodecInfos, outAudioCodecInfos)) {
            error = 4;
            break;
        }

        videoDecoderDelegate_ = new VideoDecoderDelegateImpl(this);
        videoDecoder_ = new VideoDecoder(videoDecoderDelegate_);
        if (!videoDecoder_->open(CODEC_SOFTWARE_TYPE, mediaDemuxer_)) {
            error = 5;
            break;
        }

        videoFilter_ = new VideoFilter;
        string videoFilterDescription;
        videoFilterDescription += "scale=" + std::to_string(static_cast<int>(1080)) + ":" + std::to_string(static_cast<int>(1920));
        const std::map<CodecKey, CodecValue>& inVideoCodecInfos = videoDecoder_->getCodecInfos();
        const std::map<CodecKey, CodecValue>& outVideoCodecInfos = videoEncoder_->getCodecInfos();
        if (!videoFilter_->open(videoFilterDescription, inVideoCodecInfos, outVideoCodecInfos)) {
            error = 6;
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

            switch (packetCtx->getPacketType()) {
            case PacketContext::PACKET_AUDIO_TYPE: {
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
                break;
            case PacketContext::PACKET_VIDEO_TYPE: {
                if (videoDecoder_->decode(packetCtx)) {
                    delete packetCtx;
                    packetCtx = nullptr;
                }

                FrameContext* frameCtx = nullptr;
                {
                    std::lock_guard<std::mutex> lock(videoDecoderBufferMutex_);
                    (void)lock;
                    if (!videoDecoderBuffer_.empty()) {
                        frameCtx = videoDecoderBuffer_.front();
                        videoDecoderBuffer_.pop();
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
                videoFilter_->add(copyFrame);
                delete copyFrame;
                copyFrame = nullptr;

                delete frameCtx;
                frameCtx = nullptr;

                Frame* filterFrame = videoFilter_->get();
                if (!filterFrame) {
                    continue;
                }
                filterFrame->setTime(filterFrame->getTime() + offsetTime_);
                if (videoEncoder_->encode(filterFrame)) {
                }
                delete filterFrame;
                filterFrame = nullptr;

                packetCtx = nullptr;
                {
                    std::lock_guard<std::mutex> lock(videoEncoderBufferMutex_);
                    (void)lock;
                    if (!videoEncoderBuffer_.empty()) {
                        packetCtx = videoEncoderBuffer_.front();
                        videoEncoderBuffer_.pop();
                    }
                }
                if (!packetCtx) {
                    continue;
                }
                mediaMuxer_->write(packetCtx);
                delete packetCtx;
                packetCtx = nullptr;
            }
                break;
            default:
                break;
            }
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

        videoFilter_->add(nullptr);

        do {
            if (cancel_) {
                break;
            }

            Frame* filterFrame = videoFilter_->get();
            if (!filterFrame) {
                break;
            }
            filterFrame->setTime(filterFrame->getTime() + offsetTime_);
            if (videoEncoder_->encode(filterFrame)) {
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

    if (videoDecoder_) {
        videoDecoder_->close();

        delete videoDecoder_;
        videoDecoder_ = nullptr;
    }

    if (videoDecoderDelegate_) {
        delete videoDecoderDelegate_;
        videoDecoderDelegate_ = nullptr;
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

    if (videoFilter_) {
        videoFilter_->close();

        delete videoFilter_;
        videoFilter_ = nullptr;
    }

    return error;
}

int VideoConcatInternal::append2(char *inData, int inDataSize)
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
        string audioFilterDescription;
        const std::map<CodecKey, CodecValue>& inAudioCodecInfos = audioDecoder_->getCodecInfos();
        const std::map<CodecKey, CodecValue>& outAudioCodecInfos = audioEncoder_->getCodecInfos();
        int samples = 1024;
        auto it = outAudioCodecInfos.find(CODEC_AUDIO_SAMPLE_SIZE_KEY);
        if (it != outAudioCodecInfos.cend()) {
            samples = std::get<int>(it->second);
        }
        if (samples == 0) {
            samples = 1024;
        }
        audioFilterDescription += "asetnsamples=n=" + std::to_string(samples) + ":p=0";
        if (!audioFilter_->open(audioFilterDescription, inAudioCodecInfos, outAudioCodecInfos)) {
            error = 4;
            break;
        }

        videoDecoderDelegate_ = new VideoDecoderDelegateImpl(this);
        videoDecoder_ = new VideoDecoder(videoDecoderDelegate_);
        if (!videoDecoder_->open(CODEC_SOFTWARE_TYPE, mediaDemuxer_)) {
            error = 5;
            break;
        }

        videoFilter_ = new VideoFilter;
        string videoFilterDescription;
        videoFilterDescription += "scale=" + std::to_string(static_cast<int>(1080)) + ":" + std::to_string(static_cast<int>(1920));
        const std::map<CodecKey, CodecValue>& inVideoCodecInfos = videoDecoder_->getCodecInfos();
        const std::map<CodecKey, CodecValue>& outVideoCodecInfos = videoEncoder_->getCodecInfos();
        if (!videoFilter_->open(videoFilterDescription, inVideoCodecInfos, outVideoCodecInfos)) {
            error = 6;
            break;
        }
    } while (false);

    return error;
}

void VideoConcatInternal::process2(bool *end, int *progress)
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

                switch (packetCtx->getPacketType()) {
                case PacketContext::PACKET_AUDIO_TYPE: {
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
                break;
                case PacketContext::PACKET_VIDEO_TYPE: {
                    if (videoDecoder_->decode(packetCtx)) {
                        delete packetCtx;
                        packetCtx = nullptr;
                    }

                    FrameContext* frameCtx = nullptr;
                    {
                        std::lock_guard<std::mutex> lock(videoDecoderBufferMutex_);
                        (void)lock;
                        if (!videoDecoderBuffer_.empty()) {
                            frameCtx = videoDecoderBuffer_.front();
                            videoDecoderBuffer_.pop();
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
                    videoFilter_->add(copyFrame);
                    delete copyFrame;
                    copyFrame = nullptr;

                    delete frameCtx;
                    frameCtx = nullptr;

                    Frame* filterFrame = videoFilter_->get();
                    if (!filterFrame) {
                        continue;
                    }
                    filterFrame->setTime(filterFrame->getTime() + offsetTime_);
                    if (videoEncoder_->encode(filterFrame)) {
                    }
                    delete filterFrame;
                    filterFrame = nullptr;

                    packetCtx = nullptr;
                    {
                        std::lock_guard<std::mutex> lock(videoEncoderBufferMutex_);
                        (void)lock;
                        if (!videoEncoderBuffer_.empty()) {
                            packetCtx = videoEncoderBuffer_.front();
                            videoEncoderBuffer_.pop();
                        }
                    }
                    if (!packetCtx) {
                        continue;
                    }
                    mediaMuxer_->write(packetCtx);
                    delete packetCtx;
                    packetCtx = nullptr;
                }
                break;
                default:
                    break;
                }
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

            videoFilter_->add(nullptr);

            do {
                if (cancel_) {
                    break;
                }

                Frame* filterFrame = videoFilter_->get();
                if (!filterFrame) {
                    break;
                }
                filterFrame->setTime(filterFrame->getTime() + offsetTime_);
                if (videoEncoder_->encode(filterFrame)) {
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

        if (videoDecoder_) {
            videoDecoder_->close();

            delete videoDecoder_;
            videoDecoder_ = nullptr;
        }

        if (videoDecoderDelegate_) {
            delete videoDecoderDelegate_;
            videoDecoderDelegate_ = nullptr;
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

        if (videoFilter_) {
            videoFilter_->close();

            delete videoFilter_;
            videoFilter_ = nullptr;
        }

        end_ = true;
    } while (false);

    *end = end_;
    *progress = end_ ? 100 : std::max(std::min((int)((requestTime * 1.f / mediaDemuxer_->getDuration()) * 100), 100), 0);
}

void VideoConcatInternal::cancel()
{
    cancel_ = true;
}

void VideoConcatInternal::close()
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

    if (videoEncoder_) {
        videoEncoder_->close();

        delete videoEncoder_;
        videoEncoder_ = nullptr;
    }

    if (videoEncoderDelegate_) {
        delete videoEncoderDelegate_;
        videoEncoderDelegate_ = nullptr;
    }

    if (mediaMuxer_) {
        mediaMuxer_->finish();
        mediaMuxer_->close();

        delete mediaMuxer_;
        mediaMuxer_ = nullptr;
    }
}

void VideoConcatInternal::getFileData(char **data, int *dataSize)
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

        videoEncoder_->encode(nullptr);

        do {
            if (cancel_) {
                break;
            }

            PacketContext* packetCtx = nullptr;
            {
                std::lock_guard<std::mutex> lock(videoEncoderBufferMutex_);
                (void)lock;
                if (!videoEncoderBuffer_.empty()) {
                    packetCtx = videoEncoderBuffer_.front();
                    videoEncoderBuffer_.pop();
                }
            }
            if (!packetCtx) {
                break;
            }
            mediaMuxer_->write(packetCtx);
            delete packetCtx;
            packetCtx = nullptr;
        } while (true);

        if (audioEncoder_) {
            audioEncoder_->close();

            delete audioEncoder_;
            audioEncoder_ = nullptr;
        }

        if (audioEncoderDelegate_) {
            delete audioEncoderDelegate_;
            audioEncoderDelegate_ = nullptr;
        }

        if (videoEncoder_) {
            videoEncoder_->close();

            delete videoEncoder_;
            videoEncoder_ = nullptr;
        }

        if (videoEncoderDelegate_) {
            delete videoEncoderDelegate_;
            videoEncoderDelegate_ = nullptr;
        }

        mediaMuxer_->finish();

        const BufferData& bufferData = mediaMuxer_->getBufferData();
        *data = (char*)bufferData.buf;
        *dataSize = bufferData.size - bufferData.room;
    } else {
        *data = nullptr;
        *dataSize = 0;
    }
}

void VideoConcatInternal::getFileData(char **data, int *dataSize, bool *end, int *progress, int readDataSize)
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

            videoEncoder_->encode(nullptr);

            do {
                if (cancel_) {
                    break;
                }

                PacketContext* packetCtx = nullptr;
                {
                    std::lock_guard<std::mutex> lock(videoEncoderBufferMutex_);
                    (void)lock;
                    if (!videoEncoderBuffer_.empty()) {
                        packetCtx = videoEncoderBuffer_.front();
                        videoEncoderBuffer_.pop();
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

void VideoConcatInternal::decodeAudioFrame(FrameContext *frameCtx)
{
    std::lock_guard<std::mutex> guard(audioDecoderBufferMutex_);
    (void)guard;
    audioDecoderBuffer_.push(frameCtx);
}

void VideoConcatInternal::encodeAudioPacket(PacketContext *packetCtx)
{
    std::lock_guard<std::mutex> guard(audioEncoderBufferMutex_);
    (void)guard;
    audioEncoderBuffer_.push(packetCtx);
}

void VideoConcatInternal::decodeVideoFrame(FrameContext *frameCtx)
{
    std::lock_guard<std::mutex> guard(videoDecoderBufferMutex_);
    (void)guard;
    videoDecoderBuffer_.push(frameCtx);
}

void VideoConcatInternal::encodeVideoPacket(PacketContext *packetCtx)
{
    std::lock_guard<std::mutex> guard(videoEncoderBufferMutex_);
    (void)guard;
    videoEncoderBuffer_.push(packetCtx);
}

VideoConcatInternal::AudioDecoderDelegateImpl::AudioDecoderDelegateImpl(VideoConcatInternal *internal)
    : internal_(internal)
{
}

VideoConcatInternal::AudioDecoderDelegateImpl::~AudioDecoderDelegateImpl()
{
    internal_ = nullptr;
}

void VideoConcatInternal::AudioDecoderDelegateImpl::output(FrameContext *frameCtx)
{
    if (internal_) {
        internal_->decodeAudioFrame(frameCtx);
    }
}

VideoConcatInternal::AudioEncoderDelegateImpl::AudioEncoderDelegateImpl(VideoConcatInternal *internal)
    : internal_(internal)
{
}

VideoConcatInternal::AudioEncoderDelegateImpl::~AudioEncoderDelegateImpl()
{
    internal_ = nullptr;
}

void VideoConcatInternal::AudioEncoderDelegateImpl::output(PacketContext *packetCtx)
{
    if (internal_) {
        internal_->encodeAudioPacket(packetCtx);
    }
}

VideoConcatInternal::VideoDecoderDelegateImpl::VideoDecoderDelegateImpl(VideoConcatInternal *internal)
    : internal_(internal)
{
}

VideoConcatInternal::VideoDecoderDelegateImpl::~VideoDecoderDelegateImpl()
{
    internal_ = nullptr;
}

void VideoConcatInternal::VideoDecoderDelegateImpl::output(FrameContext *frameCtx)
{
    if (internal_) {
        internal_->decodeVideoFrame(frameCtx);
    }
}

VideoConcatInternal::VideoEncoderDelegateImpl::VideoEncoderDelegateImpl(VideoConcatInternal *internal)
    : internal_(internal)
{
}

VideoConcatInternal::VideoEncoderDelegateImpl::~VideoEncoderDelegateImpl()
{
    internal_ = nullptr;
}

void VideoConcatInternal::VideoEncoderDelegateImpl::output(PacketContext *packetCtx)
{
    if (internal_) {
        internal_->encodeVideoPacket(packetCtx);
    }
}

}
