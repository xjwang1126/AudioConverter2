#include "audioprocessorinternal.h"

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

AudioProcessorInternal::AudioProcessorInternal()
{
#if 0
    av_log_set_level(AV_LOG_DEBUG);
#endif
}

int AudioProcessorInternal::open(char *inData, int inDataSize, char *coverData, int coverDataSize, char *formatName, const int64_t startTime, const int64_t endTime, const int64_t fadeIn, const int64_t fadeOut, const float tempo, const float volume, const bool reverse, const TimeMode &timeMode, const AudioFormat &audioFormat, const std::map<MetadataType, string> &metadatas)
{
    int error = 0;

    startTime_ = startTime;
    endTime_ = endTime;
    timeMode_ = timeMode;

    cancel_ = false;

    do {
        mediaDemuxer_ = new MediaDemuxer();
        if (!mediaDemuxer_->open2(inData, inDataSize)) {
            error = 1;
            break;
        }

        FormatType formatType = FORMAT_NONE_TYPE;
        if (strcmp(formatName, "mp3") == 0) {
            formatType = FORMAT_MP3_TYPE;
        } else if (strcmp(formatName, "m4a") == 0) {
            formatType = FORMAT_M4A_TYPE;
        } else if (strcmp(formatName, "flac") == 0) {
            formatType = FORMAT_FLAC_TYPE;
        } else if (strcmp(formatName, "ogg") == 0) {
            formatType = FORMAT_OGG_TYPE;
        } else {
            break;
        }
        mediaMuxer_ = new MediaMuxer();
        if (!mediaMuxer_->open(formatType)) {
            error = 2;
            break;
        }

        if (!(mediaDemuxer_->getMediaType() == MEDIA_VIDEO_AUDIO_TYPE && mediaMuxer_->getMediaType() == MEDIA_VIDEO_AUDIO_TYPE)) {
            error = 3;
            break;
        }

        coverMediaDemuxer_ = new MediaDemuxer();
        noCover_ = [&coverData, &coverDataSize, this](){
            if (!coverData || !coverDataSize) {
                return true;
            }
            if (!coverMediaDemuxer_->open2(coverData, coverDataSize)) {
                return true;
            }
            return false;
        }();

        mediaMuxer_->setMuxerType(MediaMuxer::MUXER_FFMPEG_TYPE);

        std::map<MetadataType, string> inMetadatas = mediaDemuxer_->getMetadatas();
        std::map<MetadataType, string> outMetadatas = metadatas;
        outMetadatas.merge(inMetadatas);
        mediaMuxer_->setMetadatas(outMetadatas);

        noDecode_ = [this, &audioFormat, &startTime, &endTime, &fadeIn, &fadeOut, tempo, &volume, &reverse](){
            if (mediaDemuxer_->getAudioCodecId() != mediaMuxer_->getAudioCodecId()) {
                return false;
            }
            AVCodecParameters* codecPar = static_cast<AVCodecParameters*>(mediaDemuxer_->getAudioCodecPar());
            if (audioFormat.channels != -1 && audioFormat.channels != codecPar->channels) {
                return false;
            }
            if (audioFormat.sampleRate != -1 && audioFormat.sampleRate != codecPar->sample_rate) {
                return false;
            }
            if (audioFormat.bitrate != -1 && audioFormat.bitrate != codecPar->bit_rate) {
                return false;
            }
            if (startTime != -1 || endTime != -1) {
                return false;
            }
            if (fadeIn != 0 || fadeOut != 0) {
                return false;
            }
            if (tempo < 1.0 - 0.01 || tempo > 1.0 + 0.01) {
                return false;
            }
            if (volume < 0 || volume > 0) {
                return false;
            }
            if (reverse) {
                return false;
            }
            return true;
        }();

        if (noDecode_) {
            const int defaultAudioIndex = MediaLibrary::getDefaultAudioIndex(static_cast<AVFormatContext*>(mediaDemuxer_->getFormatContext()));
            if (defaultAudioIndex == -1) {
                error = 4;
                break;
            }
            mediaMuxer_->addStream(mediaDemuxer_, defaultAudioIndex);
        } else {
            audioDecoderDelegate_ = new AudioDecoderDelegateImpl(this);
            audioDecoder_ = new AudioDecoder(audioDecoderDelegate_);
            if (!audioDecoder_->open(CODEC_SOFTWARE_TYPE, mediaDemuxer_)) {
                error = 4;
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
                error = 5;
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
            std::vector<std::pair<int64_t, float> > volumeInfos;
            switch (timeMode_) {
            case TIME_CUT_MODE: {
                const int64_t duration = mediaDemuxer_->getDuration();
                const int64_t startTimeReal = startTime == -1 ? 0 : startTime;
                const int64_t endTimeReal = endTime == -1 ? duration : endTime;
                const int64_t totalTimeReal = endTimeReal - startTimeReal;
                const int64_t fadeInMin = std::min(fadeIn, totalTimeReal / 2);
                const int64_t fadeOutMin = std::min(fadeOut, totalTimeReal / 2);
                const int64_t fadeInStartTime = 0;
                const int64_t fadeInDuration = fadeInMin;
                const int64_t fadeOutStartTime = totalTimeReal - fadeOutMin;
                const int64_t fadeOutDuration = fadeOutMin + (fadeOutMin == 0 ? 0 : 100);
                if (fadeInDuration == 0) {
                    volumeInfos.push_back(std::make_pair(fadeInStartTime, 1.f));
                } else {
                    volumeInfos.push_back(std::make_pair(fadeInStartTime, 0.f));
                    volumeInfos.push_back(std::make_pair(fadeInStartTime + fadeInDuration, 1.f));
                }
                if (fadeOutDuration == 0) {
                    volumeInfos.push_back(std::make_pair(fadeOutStartTime + fadeOutDuration, 1.f));
                } else {
                    volumeInfos.push_back(std::make_pair(fadeOutStartTime, 1.f));
                    volumeInfos.push_back(std::make_pair(fadeOutStartTime + fadeOutDuration, 0.f));
                }
            }
                break;
            case TIME_TRIM_MODE: {
                const int64_t duration = mediaDemuxer_->getDuration();
                const int64_t endTimeReal = startTime == -1 ? 0 : startTime;
                const int64_t startTimeReal = endTime == -1 ? duration : endTime;
                const int64_t totalTime = duration - (startTimeReal - endTimeReal);
                const int64_t fadeInMin = std::min(fadeIn, duration - startTimeReal);
                const int64_t fadeOutMin = std::min(fadeOut, endTimeReal);
                const int64_t fadeInStartTime = endTimeReal;
                const int64_t fadeInDuration = fadeInMin;
                const int64_t fadeOutStartTime = endTimeReal - fadeOutMin;
                const int64_t fadeOutDuration = fadeOutMin;
                volumeInfos.push_back(std::make_pair(0, 1.f));
                if (fadeOutDuration == 0) {
                    volumeInfos.push_back(std::make_pair(fadeOutStartTime + fadeOutDuration, 1.f));
                } else {
                    volumeInfos.push_back(std::make_pair(fadeOutStartTime, 1.f));
                    volumeInfos.push_back(std::make_pair(fadeOutStartTime + fadeOutDuration, 0.f));
                }
                if (fadeInDuration == 0) {
                    volumeInfos.push_back(std::make_pair(fadeInStartTime, 1.f));
                } else {
                    volumeInfos.push_back(std::make_pair(fadeInStartTime, 0.f));
                    volumeInfos.push_back(std::make_pair(fadeInStartTime + fadeInDuration, 1.f));
                }
                volumeInfos.push_back(std::make_pair(totalTime, 1.f));
            }
                break;
            default:
                break;
            }
            string volumeDescription;
            if (volumeInfos.size() > 2) {
                int64_t fromTime, toTime;
                float fromVolume, toVolume;
                string headVolumeDescription;
                string tailVolumeDescription;
                headVolumeDescription += "volume='";
                tailVolumeDescription += ", 1";
                for (auto it = volumeInfos.begin(); it != volumeInfos.end(); it++) {
                    auto nextIt = it + 1;
                    if (nextIt == volumeInfos.end()) {
                        break;
                    }
                    fromTime = it->first;
                    fromVolume = it->second;
                    toTime = nextIt->first;
                    toVolume = nextIt->second;
                    if (headVolumeDescription != "volume='") {
                        headVolumeDescription += ",";
                    }
                    headVolumeDescription += "if(between(t," + std::to_string(fromTime * 1.f / SECOND_TO_MILLISECOND_UNIT) + "," + std::to_string(toTime * 1.f / SECOND_TO_MILLISECOND_UNIT) + "), " + std::to_string(fromVolume) + "+(" + std::to_string(toVolume - fromVolume) + ")*(t-" +  std::to_string(fromTime * 1.f / SECOND_TO_MILLISECOND_UNIT) + ")/" + std::to_string((toTime - fromTime) * 1.f / SECOND_TO_MILLISECOND_UNIT);
                    tailVolumeDescription += ")";
                }
                volumeDescription = headVolumeDescription + tailVolumeDescription + "':eval=frame";
            }
            if (volume < 0 || volume > 0) {
                volumeDescription = "volume=" + std::to_string(volume) + "dB";
            }
            filterDescription += volumeDescription + (volumeDescription.empty() ? "" : ",");
            string reverseDescription;
            if (reverse) {
                reverseDescription = "areverse";
            }
            filterDescription += reverseDescription + (reverseDescription.empty() ? "" : ",");
            string rubberbandDescription;
            rubberbandDescription = "atempo=" + std::to_string(tempo);
            filterDescription += rubberbandDescription + (rubberbandDescription.empty() ? "" : ",");
            filterDescription += "asetnsamples=n=" + std::to_string(samples) + ":p=0";
            if (!audioFilter_->open(filterDescription, inCodecInfos, outCodecInfos)) {
                error = 6;
                break;
            }
        }

        if (!noCover_) {
            const int defaultVideoIndex = MediaLibrary::getDefaultVideoIndex(static_cast<AVFormatContext*>(coverMediaDemuxer_->getFormatContext()));
            if (defaultVideoIndex == -1) {
                error = 7;
                break;
            }
            mediaMuxer_->addStream(coverMediaDemuxer_, defaultVideoIndex);
        }

        if (!mediaMuxer_->start()) {
            error = 8;
            break;
        }

        if (!noCover_) {
            coverMediaDemuxer_->seek(0);
        }

#if ADD_BLOCK_PROCESS
        switch (timeMode_) {
        case TIME_CUT_MODE:
        {
            const int64_t duration = mediaDemuxer_->getDuration();
            curStartTime = startTime_ == -1 ? 0 : startTime_;
            curEndTime = endTime_ == -1 ? duration : endTime_;
            totalTime = curEndTime - curStartTime;

            mediaDemuxer_->seek(curStartTime);
            requestTime = curStartTime;
        }
            break;
        case TIME_TRIM_MODE:
        {
            const int64_t duration = mediaDemuxer_->getDuration();
            curEndTime = startTime_ == -1 ? 0 : startTime_;
            curStartTime = endTime_ == -1 ? duration : endTime_;
            totalTime = (curEndTime - 0) + (duration - curStartTime);

            mediaDemuxer_->seek(0);
            requestTime = 0;
        }
            break;
        default:
            break;
        };

        packetCtx = nullptr;
#endif
    } while (false);

    return error;
}

void AudioProcessorInternal::process()
{
    do {
        PacketContext* packetCtx = nullptr;
        while (!noCover_) {
            if (cancel_) {
                break;
            }

            if (!packetCtx) {
                packetCtx = coverMediaDemuxer_->read();
            }
            if (!packetCtx) {
                break;
            }

            if (packetCtx->getPacketType() != PacketContext::PACKET_VIDEO_TYPE) {
                delete packetCtx;
                packetCtx = nullptr;
                continue;
            }

            mediaMuxer_->write(packetCtx);
            delete packetCtx;
            packetCtx = nullptr;
        }

        int64_t totalTime = 0;
        int64_t duration = 0;
        switch (timeMode_) {
        case TIME_CUT_MODE:
        {
            duration = mediaDemuxer_->getDuration();
            const int64_t curStartTime = startTime_ == -1 ? 0 : startTime_;
            const int64_t curEndTime = endTime_ == -1 ? duration : endTime_;
            totalTime = curEndTime - curStartTime;

            mediaDemuxer_->seek(curStartTime);
            int64_t requestTime = curStartTime;
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

                if (noDecode_) {
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
                        while (!audioDecoderBuffer_.empty()) {
                            frameCtx = audioDecoderBuffer_.front();
                            if (((requestTime >= frameCtx->getTime()) && (requestTime < (frameCtx->getTime() + frameCtx->getDuration()))) || (requestTime < frameCtx->getTime())) {
                                break;
                            } else {
                                audioDecoderBuffer_.pop();
                                delete frameCtx;
                                frameCtx = nullptr;
                            }
                        }
                    }
                    if (!frameCtx) {
                        continue;
                    }

                    Frame* frame = frameCtx->getFrame();
                    Frame* copyFrame = frame->copy();
                    assert(copyFrame);
                    const int64_t frameTime = requestTime - curStartTime;
                    copyFrame->setTime(frameTime);
                    audioFilter_->add(copyFrame);
                    delete copyFrame;
                    copyFrame = nullptr;

                    if (requestTime > curEndTime) {
                        break;
                    }

                    requestTime += frame->getDuration();

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
        }
            break;
        case TIME_TRIM_MODE:
        {
            duration = mediaDemuxer_->getDuration();
            const int64_t curEndTime = startTime_ == -1 ? 0 : startTime_;
            const int64_t curStartTime = endTime_ == -1 ? duration : endTime_;
            totalTime = (curEndTime - 0) + (duration - curStartTime);

            mediaDemuxer_->seek(0);
            int64_t requestTime = 0;
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

                if (noDecode_) {
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
                        while (!audioDecoderBuffer_.empty()) {
                            frameCtx = audioDecoderBuffer_.front();
                            if (((requestTime >= frameCtx->getTime()) && (requestTime < (frameCtx->getTime() + frameCtx->getDuration()))) || (requestTime < frameCtx->getTime())) {
                                break;
                            } else {
                                audioDecoderBuffer_.pop();
                                delete frameCtx;
                                frameCtx = nullptr;
                            }
                        }
                    }
                    if (!frameCtx) {
                        continue;
                    }

                    Frame* frame = frameCtx->getFrame();
                    Frame* copyFrame = frame->copy();
                    assert(copyFrame);
                    const int64_t frameTime = requestTime - 0;
                    copyFrame->setTime(frameTime);
                    audioFilter_->add(copyFrame);
                    delete copyFrame;
                    copyFrame = nullptr;

                    requestTime += frame->getDuration();

                    if (requestTime > curEndTime) {
                        break;
                    }

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

            const int64_t writePakcetTime = requestTime;

            mediaDemuxer_->seek(curStartTime);
            requestTime = curStartTime;
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

                if (noDecode_) {
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
                        while (!audioDecoderBuffer_.empty()) {
                            frameCtx = audioDecoderBuffer_.front();
                            if (((requestTime >= frameCtx->getTime()) && (requestTime < (frameCtx->getTime() + frameCtx->getDuration()))) || (requestTime < frameCtx->getTime())) {
                                break;
                            } else {
                                audioDecoderBuffer_.pop();
                                delete frameCtx;
                                frameCtx = nullptr;
                            }
                        }
                    }
                    if (!frameCtx) {
                        continue;
                    }

                    Frame* frame = frameCtx->getFrame();
                    Frame* copyFrame = frame->copy();
                    assert(copyFrame);
                    const int64_t frameTime = writePakcetTime + requestTime - curStartTime;
                    copyFrame->setTime(frameTime);
                    audioFilter_->add(copyFrame);
                    delete copyFrame;
                    copyFrame = nullptr;

                    requestTime += frame->getDuration();

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
        }
            break;
        default:
            break;
        }
        if (totalTime > duration) {
            break;
        }

        if (!noDecode_) {
            audioFilter_->add(nullptr);

            do {
                if (cancel_) {
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
                if (cancel_) {
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

        if (cancel_) {
            break;
        }
    } while (false);
}

void AudioProcessorInternal::process(bool *end, int *progress)
{
    do {
        if (end_) {
            break;
        }

        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        long long beginTime = ts.tv_sec * 1000LL + ts.tv_nsec / 1000000;
        bool processTimeMeet = false;

        while (!noCover_) {
            if (cancel_) {
                break;
            }

            if (!packetCtx) {
                packetCtx = coverMediaDemuxer_->read();
            }
            if (!packetCtx) {
                break;
            }

            if (packetCtx->getPacketType() != PacketContext::PACKET_VIDEO_TYPE) {
                delete packetCtx;
                packetCtx = nullptr;
                continue;
            }

            mediaMuxer_->write(packetCtx);
            delete packetCtx;
            packetCtx = nullptr;
        }

        switch (timeMode_) {
        case TIME_CUT_MODE:
        {
            while (true) {
                clock_gettime(CLOCK_REALTIME, &ts);
                long long endTime = ts.tv_sec * 1000LL + ts.tv_nsec / 1000000;
                int processTime = endTime - beginTime;
                if (processTime > 200) {
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

                if (noDecode_) {
                    requestTime = std::max(requestTime, packetCtx->getTime());

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
                        while (!audioDecoderBuffer_.empty()) {
                            frameCtx = audioDecoderBuffer_.front();
                            if (((requestTime >= frameCtx->getTime()) && (requestTime < (frameCtx->getTime() + frameCtx->getDuration()))) || (requestTime < frameCtx->getTime())) {
                                break;
                            } else {
                                audioDecoderBuffer_.pop();
                                delete frameCtx;
                                frameCtx = nullptr;
                            }
                        }
                    }
                    if (!frameCtx) {
                        continue;
                    }

                    Frame* frame = frameCtx->getFrame();
                    Frame* copyFrame = frame->copy();
                    assert(copyFrame);
                    const int64_t frameTime = requestTime - curStartTime;
                    copyFrame->setTime(frameTime);
                    audioFilter_->add(copyFrame);
                    delete copyFrame;
                    copyFrame = nullptr;

                    if (requestTime > curEndTime) {
                        break;
                    }

                    requestTime += frame->getDuration();

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
        }
            break;
        case TIME_TRIM_MODE:
        {
            while (true) {
                clock_gettime(CLOCK_REALTIME, &ts);
                long long endTime = ts.tv_sec * 1000LL + ts.tv_nsec / 1000000;
                int processTime = endTime - beginTime;
                if (processTime > 200) {
                    processTimeMeet = true;
                    break;
                }

                if (cancel_) {
                    break;
                }

                if (requestTime > curEndTime) {
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

                if (noDecode_) {
                    assert(0);
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
                        while (!audioDecoderBuffer_.empty()) {
                            frameCtx = audioDecoderBuffer_.front();
                            if (((requestTime >= frameCtx->getTime()) && (requestTime < (frameCtx->getTime() + frameCtx->getDuration()))) || (requestTime < frameCtx->getTime())) {
                                break;
                            } else {
                                audioDecoderBuffer_.pop();
                                delete frameCtx;
                                frameCtx = nullptr;
                            }
                        }
                    }
                    if (!frameCtx) {
                        continue;
                    }

                    Frame* frame = frameCtx->getFrame();
                    Frame* copyFrame = frame->copy();
                    assert(copyFrame);
                    const int64_t frameTime = requestTime - 0;
                    copyFrame->setTime(frameTime);
                    audioFilter_->add(copyFrame);
                    delete copyFrame;
                    copyFrame = nullptr;

                    requestTime += frame->getDuration();

                    if (requestTime > curEndTime) {
                        break;
                    }

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

            if (processTimeMeet) {
                break;
            }

            if (trimPakcetTime == -1) {
                trimPakcetTime = requestTime;

                mediaDemuxer_->seek(curStartTime);
                requestTime = curStartTime;
            }

            while (true) {
                clock_gettime(CLOCK_REALTIME, &ts);
                long long endTime = ts.tv_sec * 1000LL + ts.tv_nsec / 1000000;
                int processTime = endTime - beginTime;
                if (processTime > 200) {
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

                if (noDecode_) {
                    assert(0);
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
                        while (!audioDecoderBuffer_.empty()) {
                            frameCtx = audioDecoderBuffer_.front();
                            if (((requestTime >= frameCtx->getTime()) && (requestTime < (frameCtx->getTime() + frameCtx->getDuration()))) || (requestTime < frameCtx->getTime())) {
                                break;
                            } else {
                                audioDecoderBuffer_.pop();
                                delete frameCtx;
                                frameCtx = nullptr;
                            }
                        }
                    }
                    if (!frameCtx) {
                        continue;
                    }

                    Frame* frame = frameCtx->getFrame();
                    Frame* copyFrame = frame->copy();
                    assert(copyFrame);
                    const int64_t frameTime = trimPakcetTime + requestTime - curStartTime;
                    copyFrame->setTime(frameTime);
                    audioFilter_->add(copyFrame);
                    delete copyFrame;
                    copyFrame = nullptr;

                    requestTime += frame->getDuration();

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
        }
            break;
        default:
            break;
        }

        if (processTimeMeet) {
            break;
        }

        if (!noDecode_) {
            audioFilter_->add(nullptr);

            do {
                if (cancel_) {
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
                if (cancel_) {
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

        if (cancel_) {
            break;
        }

        end_ = true;
    } while (false);

    *end = end_;
    *progress = end_ ? 100 : std::max(std::min((int)((requestTime * 1.f / totalTime) * 100), 100), 0);
}

void AudioProcessorInternal::process(bool *end, int *progress, char **data, int *dataSize)
{
    process(end, progress);

    const BufferData& bufferData = mediaMuxer_->getBufferData();
    *data = (char*)(bufferData.buf + index_);
    *dataSize = bufferData.size - bufferData.room - index_;
    index_ = bufferData.size - bufferData.room;
}

void AudioProcessorInternal::cancel()
{
    cancel_ = true;
}

void AudioProcessorInternal::close()
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

    if (coverMediaDemuxer_) {
        coverMediaDemuxer_->close();

        delete coverMediaDemuxer_;
        coverMediaDemuxer_ = nullptr;
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

    if (audioFilter_) {
        audioFilter_->close();

        delete audioFilter_;
        audioFilter_ = nullptr;
    }
}

void AudioProcessorInternal::getFileData(char **data, int *dataSize)
{
    if (mediaMuxer_) {
        mediaMuxer_->finish();

        const BufferData& bufferData = mediaMuxer_->getBufferData();
        *data = (char*)bufferData.buf;
        *dataSize = bufferData.size - bufferData.room;
    } else {
        *data = nullptr;
        *dataSize = 0;
    }
}

void AudioProcessorInternal::getFileData(char **data, int *dataSize, bool *end, int *progress, int readDataSize)
{
    if (mediaMuxer_) {
        if (readDataSize == 0) {
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

void AudioProcessorInternal::decodeAudioFrame(FrameContext *frameCtx)
{
    std::lock_guard<std::mutex> guard(audioDecoderBufferMutex_);
    (void)guard;
    audioDecoderBuffer_.push(frameCtx);
}

void AudioProcessorInternal::encodeAudioPacket(PacketContext *packetCtx)
{
    std::lock_guard<std::mutex> guard(audioEncoderBufferMutex_);
    (void)guard;
    audioEncoderBuffer_.push(packetCtx);
}

AudioProcessorInternal::AudioDecoderDelegateImpl::AudioDecoderDelegateImpl(AudioProcessorInternal *internal)
    : internal_(internal)
{
}

AudioProcessorInternal::AudioDecoderDelegateImpl::~AudioDecoderDelegateImpl()
{
    internal_ = nullptr;
}

void AudioProcessorInternal::AudioDecoderDelegateImpl::output(FrameContext *frameCtx)
{
    if (internal_) {
        internal_->decodeAudioFrame(frameCtx);
    }
}

AudioProcessorInternal::AudioEncoderDelegateImpl::AudioEncoderDelegateImpl(AudioProcessorInternal *internal)
    : internal_(internal)
{
}

AudioProcessorInternal::AudioEncoderDelegateImpl::~AudioEncoderDelegateImpl()
{
    internal_ = nullptr;
}

void AudioProcessorInternal::AudioEncoderDelegateImpl::output(PacketContext *packetCtx)
{
    if (internal_) {
        internal_->encodeAudioPacket(packetCtx);
    }
}

}
