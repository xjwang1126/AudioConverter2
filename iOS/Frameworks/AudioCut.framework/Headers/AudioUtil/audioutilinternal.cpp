#include "audioutilinternal.h"
#include "MediaUtil/mediautildelegate.h"

#include "Common/Model/packetcontext.h"
#include "Common/Model/framecontext.h"
#include "Common/Model/ffmpegframe.h"
#include "Common/Model/imageframe.h"
#include "Common/Media/mediacommon.h"
#include "Common/Media/mediademuxer.h"
#include "Common/Media/mediamuxer.h"
#include "Common/Codec/Decoder/audiodecoder.h"
#include "Common/Codec/Encoder/audioencoder.h"
#include "Common/Codec/Decoder/videodecoder.h"
#include "Common/Filter/AudioFilter/audiofilter.h"
#include "Common/Util/file.h"

#include <cassert>
#include <vector>
#include <iostream>

extern "C" {
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
}

namespace MediaLibrary {

AudioUtilInternal::AudioUtilInternal()
    : MediaUtilInternal()
{
#if 0
    av_log_set_level(AV_LOG_DEBUG);
#endif
}

int AudioUtilInternal::editAudio(const string &inFilePath, const string &coverFilePath, const string &outFilePath, const int64_t startTime, const int64_t endTime, const int64_t fadeIn, const int64_t fadeOut, const float tempo, const TimeMode &timeMode, const AudioFormat &audioFormat, const std::map<MetadataType, string> &metadatas, const MediaUtilDelegate *delegate)
{
    bool result = false;
    int error = 0;

    cancel_ = false;

    MediaDemuxer mediaDemuxer;
    AudioDecoder* audioDecoder{nullptr};
    DecoderDelegate* audioDecoderDelegate{nullptr};
    AudioFilter* audioFilter{nullptr};
    AudioEncoder* audioEncoder{nullptr};
    EncoderDelegate* audioEncoderDelegate{nullptr};
    MediaMuxer mediaMuxer;
    MediaDemuxer coverMediaDemuxer;

    do {
        if (!File::exist(inFilePath)) {
            error = 1;
            break;
        }
        if (File::exist(outFilePath)) {
            error = 2;
            break;
        }

        if (!mediaDemuxer.open(inFilePath)) {
            error = 3;
            break;
        }
        if (!mediaMuxer.open(outFilePath)) {
            error = 4;
            break;
        }

        if (!(mediaDemuxer.getMediaType() == MEDIA_VIDEO_AUDIO_TYPE && mediaMuxer.getMediaType() == MEDIA_VIDEO_AUDIO_TYPE)) {
            error = 5;
            break;
        }

        const bool noCover = [&coverFilePath, &coverMediaDemuxer](){
            if (!File::exist(coverFilePath)) {
                return true;
            }
            if (!coverMediaDemuxer.open(coverFilePath)) {
                return true;
            }
            return false;
        }();

        mediaMuxer.setMuxerType(MediaMuxer::MUXER_FFMPEG_TYPE);

        std::map<MetadataType, string> inMetadatas = mediaDemuxer.getMetadatas();
        std::map<MetadataType, string> outMetadatas = metadatas;
        outMetadatas.merge(inMetadatas);
        mediaMuxer.setMetadatas(outMetadatas);

        const bool noDecode = [&mediaDemuxer, &mediaMuxer, &audioFormat, &startTime, &endTime, &fadeIn, &fadeOut, &tempo](){
            if (mediaDemuxer.getAudioCodecId() != mediaMuxer.getAudioCodecId()) {
                return false;
            }
            AVCodecParameters* codecPar = static_cast<AVCodecParameters*>(mediaDemuxer.getAudioCodecPar());
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
            return true;
        }();

        if (noDecode) {
            const int defaultAudioIndex = MediaLibrary::getDefaultAudioIndex(static_cast<AVFormatContext*>(mediaDemuxer.getFormatContext()));
            if (defaultAudioIndex == -1) {
                error = 6;
                break;
            }
            mediaMuxer.addStream(&mediaDemuxer, defaultAudioIndex);
        } else {
            audioDecoderDelegate = new AudioDecoderDelegateImpl(this);
            audioDecoder = new AudioDecoder(audioDecoderDelegate);
            if (!audioDecoder->open(CODEC_SOFTWARE_TYPE, &mediaDemuxer)) {
                error = 6;
                break;
            }

            audioEncoderDelegate = new AudioEncoderDelegateImpl(this);
            audioEncoder = new AudioEncoder(audioEncoderDelegate);
            std::map<CodecKey, CodecValue> codecInfos = audioDecoder->getCodecInfos();
            switch (mediaMuxer.getFormatType()) {
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
            if (!audioEncoder->open(CODEC_SOFTWARE_TYPE, &mediaMuxer, codecInfos)) {
                error = 7;
                break;
            }

            audioFilter = new AudioFilter;
            string filterDescription;
            const std::map<CodecKey, CodecValue>& inCodecInfos = audioDecoder->getCodecInfos();
            const std::map<CodecKey, CodecValue>& outCodecInfos = audioEncoder->getCodecInfos();
            int samples = 1024;
            auto it = outCodecInfos.find(CODEC_AUDIO_SAMPLE_SIZE_KEY);
            if (it != outCodecInfos.cend()) {
                samples = std::get<int>(it->second);
            }
            if (samples == 0) {
                samples = 1024;
            }
            std::vector<std::pair<int64_t, float> > volumeInfos;
            switch (timeMode) {
            case TIME_CUT_MODE: {
                const int64_t duration = mediaDemuxer.getDuration();
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
                const int64_t duration = mediaDemuxer.getDuration();
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
            filterDescription += volumeDescription + (volumeDescription.empty() ? "" : ",");
            string rubberbandDescription;
            rubberbandDescription = "atempo=" + std::to_string(tempo);
            filterDescription += rubberbandDescription + (rubberbandDescription.empty() ? "" : ",");
            filterDescription += "asetnsamples=n=" + std::to_string(samples) + ":p=0";
            if (!audioFilter->open(filterDescription, inCodecInfos, outCodecInfos)) {
                error = 8;
                break;
            }
        }

        if (!noCover) {
            const int defaultVideoIndex = MediaLibrary::getDefaultVideoIndex(static_cast<AVFormatContext*>(coverMediaDemuxer.getFormatContext()));
            if (defaultVideoIndex == -1) {
                error = 9;
                break;
            }
            mediaMuxer.addStream(&coverMediaDemuxer, defaultVideoIndex);
        }

        if (!mediaMuxer.start()) {
            error = 10;
            break;
        }

        if (!noCover) {
            coverMediaDemuxer.seek(0);
        }

        PacketContext* packetCtx = nullptr;
        while (!noCover) {
            if (cancel_) {
                break;
            }

            if (!packetCtx) {
                packetCtx = coverMediaDemuxer.read();
            }
            if (!packetCtx) {
                break;
            }

            if (packetCtx->getPacketType() != PacketContext::PACKET_VIDEO_TYPE) {
                delete packetCtx;
                packetCtx = nullptr;
                continue;
            }

            mediaMuxer.write(packetCtx);
            delete packetCtx;
            packetCtx = nullptr;
        }

        int64_t totalTime = 0;
        int64_t duration = 0;
        switch (timeMode) {
        case TIME_CUT_MODE:
        {
            duration = mediaDemuxer.getDuration();
            const int64_t curStartTime = startTime == -1 ? 0 : startTime;
            const int64_t curEndTime = endTime == -1 ? duration : endTime;
            totalTime = curEndTime - curStartTime;
            if (totalTime > duration) {
                break;
            }

            mediaDemuxer.seek(curStartTime);
            int64_t requestTime = curStartTime;
            while (true) {
                if (cancel_) {
                    break;
                }

                if (!packetCtx) {
                    packetCtx = mediaDemuxer.read();
                }
                if (!packetCtx) {
                    break;
                }

                if (packetCtx->getPacketType() != PacketContext::PACKET_AUDIO_TYPE) {
                    delete packetCtx;
                    packetCtx = nullptr;
                    continue;
                }

                if (noDecode) {
                    if (delegate) {
                        delegate->notifyExportProgress(packetCtx->getTime(), totalTime);
                    }

                    mediaMuxer.write(packetCtx);
                    delete packetCtx;
                    packetCtx = nullptr;
                } else {
                    if (audioDecoder->decode(packetCtx)) {
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
                    audioFilter->add(copyFrame);
                    delete copyFrame;
                    copyFrame = nullptr;

                    if (requestTime > curEndTime) {
                        break;
                    }

                    requestTime += frame->getDuration();

                    Frame* filterFrame = audioFilter->get();
                    if (!filterFrame) {
                        continue;
                    }
                    if (delegate) {
                        delegate->notifyExportProgress(filterFrame->getTime(), totalTime);
                    }
                    if (audioEncoder->encode(filterFrame)) {
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
                    mediaMuxer.write(packetCtx);
                    delete packetCtx;
                    packetCtx = nullptr;
                }
            }
        }
            break;
        case TIME_TRIM_MODE:
        {
            duration = mediaDemuxer.getDuration();
            const int64_t curEndTime = startTime == -1 ? 0 : startTime;
            const int64_t curStartTime = endTime == -1 ? duration : endTime;
            totalTime = (curEndTime - 0) + (duration - curStartTime);
            if (totalTime > duration) {
                break;
            }

            mediaDemuxer.seek(0);
            int64_t requestTime = 0;
            while (true) {
                if (cancel_) {
                    break;
                }

                if (!packetCtx) {
                    packetCtx = mediaDemuxer.read();
                }
                if (!packetCtx) {
                    break;
                }

                if (packetCtx->getPacketType() != PacketContext::PACKET_AUDIO_TYPE) {
                    delete packetCtx;
                    packetCtx = nullptr;
                    continue;
                }

                if (noDecode) {
                    if (delegate) {
                        delegate->notifyExportProgress(packetCtx->getTime(), totalTime);
                    }

                    mediaMuxer.write(packetCtx);
                    delete packetCtx;
                    packetCtx = nullptr;
                } else {
                    if (audioDecoder->decode(packetCtx)) {
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
                    audioFilter->add(copyFrame);
                    delete copyFrame;
                    copyFrame = nullptr;

                    requestTime += frame->getDuration();

                    if (requestTime > curEndTime) {
                        break;
                    }

                    Frame* filterFrame = audioFilter->get();
                    if (!filterFrame) {
                        continue;
                    }
                    if (delegate) {
                        delegate->notifyExportProgress(filterFrame->getTime(), totalTime);
                    }
                    if (audioEncoder->encode(filterFrame)) {
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
                    mediaMuxer.write(packetCtx);
                    delete packetCtx;
                    packetCtx = nullptr;
                }
            }

            const int64_t writePakcetTime = requestTime;

            mediaDemuxer.seek(curStartTime);
            requestTime = curStartTime;
            while (true) {
                if (cancel_) {
                    break;
                }

                if (!packetCtx) {
                    packetCtx = mediaDemuxer.read();
                }
                if (!packetCtx) {
                    break;
                }

                if (packetCtx->getPacketType() != PacketContext::PACKET_AUDIO_TYPE) {
                    delete packetCtx;
                    packetCtx = nullptr;
                    continue;
                }

                if (noDecode) {
                    if (delegate) {
                        delegate->notifyExportProgress(packetCtx->getTime(), totalTime);
                    }

                    mediaMuxer.write(packetCtx);
                    delete packetCtx;
                    packetCtx = nullptr;
                } else {
                    if (audioDecoder->decode(packetCtx)) {
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
                    audioFilter->add(copyFrame);
                    delete copyFrame;
                    copyFrame = nullptr;

                    requestTime += frame->getDuration();

                    Frame* filterFrame = audioFilter->get();
                    if (!filterFrame) {
                        continue;
                    }
                    if (delegate) {
                        delegate->notifyExportProgress(filterFrame->getTime(), totalTime);
                    }
                    if (audioEncoder->encode(filterFrame)) {
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
                    mediaMuxer.write(packetCtx);
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
            error = 11;
            break;
        }

        if (!noDecode) {
            audioFilter->add(nullptr);

            do {
                if (cancel_) {
                    break;
                }

                Frame* filterFrame = audioFilter->get();
                if (!filterFrame) {
                    break;
                }
                if (delegate) {
                    delegate->notifyExportProgress(filterFrame->getTime(), totalTime);
                }
                if (audioEncoder->encode(filterFrame)) {
                }
                delete filterFrame;
                filterFrame = nullptr;
            } while (true);

            audioEncoder->encode(nullptr);

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
                mediaMuxer.write(packetCtx);
                delete packetCtx;
                packetCtx = nullptr;
            } while (true);
        }

        if (cancel_) {
            break;
        }

        if (delegate) {
            delegate->notifyExportProgress(totalTime, totalTime);
        }

        result = true;
    } while (false);

    if (audioDecoder) {
        audioDecoder->close();

        delete audioDecoder;
        audioDecoder = nullptr;
    }

    if (audioDecoderDelegate) {
        delete audioDecoderDelegate;
        audioDecoderDelegate = nullptr;
    }

    mediaDemuxer.close();
    coverMediaDemuxer.close();

    if (audioEncoder) {
        audioEncoder->close();

        delete audioEncoder;
        audioEncoder = nullptr;
    }

    if (audioEncoderDelegate) {
        delete audioEncoderDelegate;
        audioEncoderDelegate = nullptr;
    }

    mediaMuxer.finish();
    mediaMuxer.close();

    if (audioFilter) {
        audioFilter->close();

        delete audioFilter;
        audioFilter = nullptr;
    }

    clearBuffer();

    if (delegate) {
        delegate->notifyExportFinish();
    }

    (void)result;
    return error;
}

int AudioUtilInternal::editAudio(char *inData, int inDataSize, char *coverData, int coverDataSize, char **outData, int *outDataSize, char *formatName, const int64_t startTime, const int64_t endTime, const int64_t fadeIn, const int64_t fadeOut, const float tempo, const TimeMode &timeMode, const AudioFormat &audioFormat, const std::map<MetadataType, string> &metadatas, const MediaUtilDelegate *delegate)
{
    bool result = false;
    int error = 0;

    cancel_ = false;

    MediaDemuxer mediaDemuxer;
    AudioDecoder* audioDecoder{nullptr};
    DecoderDelegate* audioDecoderDelegate{nullptr};
    AudioFilter* audioFilter{nullptr};
    AudioEncoder* audioEncoder{nullptr};
    EncoderDelegate* audioEncoderDelegate{nullptr};
    MediaMuxer mediaMuxer;
    MediaDemuxer coverMediaDemuxer;

    do {
        if (!mediaDemuxer.open2(inData, inDataSize)) {
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
        if (!mediaMuxer.open(formatType)) {
            error = 2;
            break;
        }

        if (!(mediaDemuxer.getMediaType() == MEDIA_VIDEO_AUDIO_TYPE && mediaMuxer.getMediaType() == MEDIA_VIDEO_AUDIO_TYPE)) {
            error = 3;
            break;
        }

        const bool noCover = [&coverData, &coverDataSize, &coverMediaDemuxer](){
            if (!coverData || !coverDataSize) {
                return true;
            }
#if 0
            if (!coverMediaDemuxer.open(coverData, coverDataSize)) {
                return true;
            }
#else
            if (!coverMediaDemuxer.open2(coverData, coverDataSize)) {
                return true;
            }
#endif
            return false;
        }();

        mediaMuxer.setMuxerType(MediaMuxer::MUXER_FFMPEG_TYPE);

        std::map<MetadataType, string> inMetadatas = mediaDemuxer.getMetadatas();
        std::map<MetadataType, string> outMetadatas = metadatas;
        outMetadatas.merge(inMetadatas);
        mediaMuxer.setMetadatas(outMetadatas);

        const bool noDecode = [&mediaDemuxer, &mediaMuxer, &audioFormat, &startTime, &endTime, &fadeIn, &fadeOut, &tempo](){
            if (mediaDemuxer.getAudioCodecId() != mediaMuxer.getAudioCodecId()) {
                return false;
            }
            AVCodecParameters* codecPar = static_cast<AVCodecParameters*>(mediaDemuxer.getAudioCodecPar());
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
            return true;
        }();

        if (noDecode) {
            const int defaultAudioIndex = MediaLibrary::getDefaultAudioIndex(static_cast<AVFormatContext*>(mediaDemuxer.getFormatContext()));
            if (defaultAudioIndex == -1) {
                error = 4;
                break;
            }
            mediaMuxer.addStream(&mediaDemuxer, defaultAudioIndex);
        } else {
            audioDecoderDelegate = new AudioDecoderDelegateImpl(this);
            audioDecoder = new AudioDecoder(audioDecoderDelegate);
            if (!audioDecoder->open(CODEC_SOFTWARE_TYPE, &mediaDemuxer)) {
                error = 4;
                break;
            }

            audioEncoderDelegate = new AudioEncoderDelegateImpl(this);
            audioEncoder = new AudioEncoder(audioEncoderDelegate);
            std::map<CodecKey, CodecValue> codecInfos = audioDecoder->getCodecInfos();
            switch (mediaMuxer.getFormatType()) {
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
            if (!audioEncoder->open(CODEC_SOFTWARE_TYPE, &mediaMuxer, codecInfos)) {
                error = 5;
                break;
            }

            audioFilter = new AudioFilter;
            string filterDescription;
            const std::map<CodecKey, CodecValue>& inCodecInfos = audioDecoder->getCodecInfos();
            const std::map<CodecKey, CodecValue>& outCodecInfos = audioEncoder->getCodecInfos();
            int samples = 1024;
            auto it = outCodecInfos.find(CODEC_AUDIO_SAMPLE_SIZE_KEY);
            if (it != outCodecInfos.cend()) {
                samples = std::get<int>(it->second);
            }
            if (samples == 0) {
                samples = 1024;
            }
            std::vector<std::pair<int64_t, float> > volumeInfos;
            switch (timeMode) {
            case TIME_CUT_MODE: {
                const int64_t duration = mediaDemuxer.getDuration();
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
                const int64_t duration = mediaDemuxer.getDuration();
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
            filterDescription += volumeDescription + (volumeDescription.empty() ? "" : ",");
            string rubberbandDescription;
            rubberbandDescription = "atempo=" + std::to_string(tempo);
            filterDescription += rubberbandDescription + (rubberbandDescription.empty() ? "" : ",");
            filterDescription += "asetnsamples=n=" + std::to_string(samples) + ":p=0";
            if (!audioFilter->open(filterDescription, inCodecInfos, outCodecInfos)) {
                error = 6;
                break;
            }
        }

        if (!noCover) {
            const int defaultVideoIndex = MediaLibrary::getDefaultVideoIndex(static_cast<AVFormatContext*>(coverMediaDemuxer.getFormatContext()));
            if (defaultVideoIndex == -1) {
                error = 7;
                break;
            }
            mediaMuxer.addStream(&coverMediaDemuxer, defaultVideoIndex);
        }

        if (!mediaMuxer.start()) {
            error = 8;
            break;
        }

        if (!noCover) {
            coverMediaDemuxer.seek(0);
        }

        PacketContext* packetCtx = nullptr;
        while (!noCover) {
            if (cancel_) {
                break;
            }

            if (!packetCtx) {
                packetCtx = coverMediaDemuxer.read();
            }
            if (!packetCtx) {
                break;
            }

            if (packetCtx->getPacketType() != PacketContext::PACKET_VIDEO_TYPE) {
                delete packetCtx;
                packetCtx = nullptr;
                continue;
            }

            mediaMuxer.write(packetCtx);
            delete packetCtx;
            packetCtx = nullptr;
        }

        int64_t totalTime = 0;
        int64_t duration = 0;
        switch (timeMode) {
        case TIME_CUT_MODE:
        {
            duration = mediaDemuxer.getDuration();
            const int64_t curStartTime = startTime == -1 ? 0 : startTime;
            const int64_t curEndTime = endTime == -1 ? duration : endTime;
            totalTime = curEndTime - curStartTime;
#if 0
            if (totalTime > duration) {
                error = 9;
                break;
            }
#endif

            mediaDemuxer.seek(curStartTime);
            int64_t requestTime = curStartTime;
            while (true) {
                if (cancel_) {
                    break;
                }

                if (!packetCtx) {
                    packetCtx = mediaDemuxer.read();
                }
                if (!packetCtx) {
                    break;
                }

                if (packetCtx->getPacketType() != PacketContext::PACKET_AUDIO_TYPE) {
                    delete packetCtx;
                    packetCtx = nullptr;
                    continue;
                }

                if (noDecode) {
                    if (delegate) {
                        delegate->notifyExportProgress(packetCtx->getTime(), totalTime);
                    }

                    mediaMuxer.write(packetCtx);
                    delete packetCtx;
                    packetCtx = nullptr;
                } else {
                    if (audioDecoder->decode(packetCtx)) {
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
                    audioFilter->add(copyFrame);
                    delete copyFrame;
                    copyFrame = nullptr;

                    if (requestTime > curEndTime) {
                        break;
                    }

                    requestTime += frame->getDuration();

                    Frame* filterFrame = audioFilter->get();
                    if (!filterFrame) {
                        continue;
                    }
                    if (delegate) {
                        delegate->notifyExportProgress(filterFrame->getTime(), totalTime);
                    }
                    if (audioEncoder->encode(filterFrame)) {
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
                    mediaMuxer.write(packetCtx);
                    delete packetCtx;
                    packetCtx = nullptr;
                }
            }
        }
            break;
        case TIME_TRIM_MODE:
        {
            duration = mediaDemuxer.getDuration();
            const int64_t curEndTime = startTime == -1 ? 0 : startTime;
            const int64_t curStartTime = endTime == -1 ? duration : endTime;
            totalTime = (curEndTime - 0) + (duration - curStartTime);
#if 0
            if (totalTime > duration) {
                error = 9;
                break;
            }
#endif

            mediaDemuxer.seek(0);
            int64_t requestTime = 0;
            while (true) {
                if (cancel_) {
                    break;
                }

                if (!packetCtx) {
                    packetCtx = mediaDemuxer.read();
                }
                if (!packetCtx) {
                    break;
                }

                if (packetCtx->getPacketType() != PacketContext::PACKET_AUDIO_TYPE) {
                    delete packetCtx;
                    packetCtx = nullptr;
                    continue;
                }

                if (noDecode) {
                    if (delegate) {
                        delegate->notifyExportProgress(packetCtx->getTime(), totalTime);
                    }

                    mediaMuxer.write(packetCtx);
                    delete packetCtx;
                    packetCtx = nullptr;
                } else {
                    if (audioDecoder->decode(packetCtx)) {
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
                    audioFilter->add(copyFrame);
                    delete copyFrame;
                    copyFrame = nullptr;

                    requestTime += frame->getDuration();

                    if (requestTime > curEndTime) {
                        break;
                    }

                    Frame* filterFrame = audioFilter->get();
                    if (!filterFrame) {
                        continue;
                    }
                    if (delegate) {
                        delegate->notifyExportProgress(filterFrame->getTime(), totalTime);
                    }
                    if (audioEncoder->encode(filterFrame)) {
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
                    mediaMuxer.write(packetCtx);
                    delete packetCtx;
                    packetCtx = nullptr;
                }
            }

            const int64_t writePakcetTime = requestTime;

            mediaDemuxer.seek(curStartTime);
            requestTime = curStartTime;
            while (true) {
                if (cancel_) {
                    break;
                }

                if (!packetCtx) {
                    packetCtx = mediaDemuxer.read();
                }
                if (!packetCtx) {
                    break;
                }

                if (packetCtx->getPacketType() != PacketContext::PACKET_AUDIO_TYPE) {
                    delete packetCtx;
                    packetCtx = nullptr;
                    continue;
                }

                if (noDecode) {
                    if (delegate) {
                        delegate->notifyExportProgress(packetCtx->getTime(), totalTime);
                    }

                    mediaMuxer.write(packetCtx);
                    delete packetCtx;
                    packetCtx = nullptr;
                } else {
                    if (audioDecoder->decode(packetCtx)) {
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
                    audioFilter->add(copyFrame);
                    delete copyFrame;
                    copyFrame = nullptr;

                    requestTime += frame->getDuration();

                    Frame* filterFrame = audioFilter->get();
                    if (!filterFrame) {
                        continue;
                    }
                    if (delegate) {
                        delegate->notifyExportProgress(filterFrame->getTime(), totalTime);
                    }
                    if (audioEncoder->encode(filterFrame)) {
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
                    mediaMuxer.write(packetCtx);
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

        if (!noDecode) {
            audioFilter->add(nullptr);

            do {
                if (cancel_) {
                    break;
                }

                Frame* filterFrame = audioFilter->get();
                if (!filterFrame) {
                    break;
                }
                if (delegate) {
                    delegate->notifyExportProgress(filterFrame->getTime(), totalTime);
                }
                if (audioEncoder->encode(filterFrame)) {
                }
                delete filterFrame;
                filterFrame = nullptr;
            } while (true);

            audioEncoder->encode(nullptr);

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
                mediaMuxer.write(packetCtx);
                delete packetCtx;
                packetCtx = nullptr;
            } while (true);
        }

        if (cancel_) {
            break;
        }

        if (delegate) {
            delegate->notifyExportProgress(totalTime, totalTime);
        }

        result = true;
    } while (false);

    if (audioDecoder) {
        audioDecoder->close();

        delete audioDecoder;
        audioDecoder = nullptr;
    }

    if (audioDecoderDelegate) {
        delete audioDecoderDelegate;
        audioDecoderDelegate = nullptr;
    }

    mediaDemuxer.close();
    coverMediaDemuxer.close();

    if (audioEncoder) {
        audioEncoder->close();

        delete audioEncoder;
        audioEncoder = nullptr;
    }

    if (audioEncoderDelegate) {
        delete audioEncoderDelegate;
        audioEncoderDelegate = nullptr;
    }

    mediaMuxer.finish();
    mediaMuxer.close();

    if (audioFilter) {
        audioFilter->close();

        delete audioFilter;
        audioFilter = nullptr;
    }

    clearBuffer();

    const BufferData& bufferData = mediaMuxer.getBufferData();
    *outData = (char*)bufferData.buf;
    *outDataSize = bufferData.size - bufferData.room;

    if (delegate) {
        delegate->notifyExportFinish();
    }

    (void)result;
    return error;
}

bool AudioUtilInternal::waveform(const string &filePath, std::vector<float> &waveform, const MediaUtilDelegate *delegate)
{
    bool result = false;

    cancel_ = false;

    MediaDemuxer mediaDemuxer;
    AudioDecoder* audioDecoder{nullptr};
    DecoderDelegate* audioDecoderDelegate{nullptr};
    AudioFilter* audioFilter{nullptr};
    int16_t sampleValueMax = 0;
    std::vector<int16_t> sampleValueChunk;
    const size_t sampleValueChunkLimit = 30;
    vector<int16_t> chunks;

    do {
        if (!File::exist(filePath)) {
            break;
        }

        if (!mediaDemuxer.open(filePath)) {
            break;
        }

        if (mediaDemuxer.getMediaType() != MEDIA_VIDEO_AUDIO_TYPE) {
            break;
        }

        const int64_t totalTime = mediaDemuxer.getDuration();

        audioDecoderDelegate = new AudioDecoderDelegateImpl(this);
        audioDecoder = new AudioDecoder(audioDecoderDelegate);
        if (!audioDecoder->open(CODEC_SOFTWARE_TYPE, &mediaDemuxer)) {
            break;
        }

        audioFilter = new AudioFilter;
        string filterDescription;
        const std::map<CodecKey, CodecValue>& inCodecInfos = audioDecoder->getCodecInfos();
        std::map<CodecKey, CodecValue> outCodecInfos = inCodecInfos;
        outCodecInfos[CODEC_AUDIO_SAMPLE_FORMAT_EKY] = AV_SAMPLE_FMT_S16;
        outCodecInfos[CODEC_AUDIO_CHANNELS_KEY] = 1;
        outCodecInfos[CODEC_AUDIO_CHANNEL_LAYOUT_KEY] = static_cast<uint64_t>(av_get_default_channel_layout(1));
        int samples = 1024;
        auto it = outCodecInfos.find(CODEC_AUDIO_SAMPLE_SIZE_KEY);
        if (it != outCodecInfos.cend()) {
            samples = std::get<int>(it->second);
        }
        if (samples == 0) {
            samples = 1024;
        }
        filterDescription += "asetnsamples=n=" + std::to_string(samples) + ":p=0";
        if (!audioFilter->open(filterDescription, inCodecInfos, outCodecInfos)) {
            break;
        }

        mediaDemuxer.seek(0);

        PacketContext* packetCtx = nullptr;
        while (true) {
            if (cancel_) {
                break;
            }

            if (!packetCtx) {
                packetCtx = mediaDemuxer.read();
            }
            if (!packetCtx) {
                break;
            }

            if (packetCtx->getPacketType() != PacketContext::PACKET_AUDIO_TYPE) {
                delete packetCtx;
                packetCtx = nullptr;
                continue;
            }

            if (audioDecoder->decode(packetCtx)) {
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
            audioFilter->add(frame);

            Frame* filterFrame = audioFilter->get();
            if (!filterFrame) {
                continue;
            }
            if (delegate) {
                delegate->notifyExportProgress(filterFrame->getTime(), totalTime);
            }

            switch (filterFrame->getFrameType()) {
            case Frame::FRAME_FFMPEG_TYPE: {
                AVFrame* avFrame = static_cast<AVFrame*>(filterFrame->getFrame());
                const int16_t* data = (int16_t*)(avFrame->data[0]);
                for (int i = 0; i < avFrame->nb_samples; i++) {
                    const int16_t sampleValue = data[i];
                    sampleValueChunk.push_back(sampleValue);
                    if (sampleValueChunk.size() < sampleValueChunkLimit) {
                        continue;
                    }
                    const int16_t sampleValueMaxInChunk = *std::max_element(sampleValueChunk.begin(), sampleValueChunk.end());
                    chunks.push_back(sampleValueMaxInChunk);
                    if (sampleValueMaxInChunk > sampleValueMax) {
                        sampleValueMax = sampleValueMaxInChunk;
                    }
                    sampleValueChunk.clear();
                }
            }
                break;
            default:
                break;
            }

            delete filterFrame;
            filterFrame = nullptr;

            delete frameCtx;
            frameCtx = nullptr;
        }

        audioFilter->add(nullptr);

        do {
            if (cancel_) {
                break;
            }

            Frame* filterFrame = audioFilter->get();
            if (!filterFrame) {
                break;
            }
            if (delegate) {
                delegate->notifyExportProgress(filterFrame->getTime(), totalTime);
            }

            switch (filterFrame->getFrameType()) {
            case Frame::FRAME_FFMPEG_TYPE: {
                AVFrame* avFrame = static_cast<AVFrame*>(filterFrame->getFrame());
                const int16_t* data = (int16_t*)(avFrame->data[0]);
                for (int i = 0; i < avFrame->nb_samples; i++) {
                    const int16_t sampleValue = data[i];
                    sampleValueChunk.push_back(sampleValue);
                    if (sampleValueChunk.size() < sampleValueChunkLimit) {
                        continue;
                    }
                    const int16_t sampleValueMaxInChunk = *std::max_element(sampleValueChunk.begin(), sampleValueChunk.end());
                    chunks.push_back(sampleValueMaxInChunk);
                    if (sampleValueMaxInChunk > sampleValueMax) {
                        sampleValueMax = sampleValueMaxInChunk;
                    }
                    sampleValueChunk.clear();
                }
            }
                break;
            default:
                break;
            }

            delete filterFrame;
            filterFrame = nullptr;
        } while (true);

        const int16_t sampleValueMaxInChunk = *std::max_element(sampleValueChunk.begin(), sampleValueChunk.end());
        chunks.push_back(sampleValueMaxInChunk);
        if (sampleValueMaxInChunk > sampleValueMax) {
            sampleValueMax = sampleValueMaxInChunk;
        }
        sampleValueChunk.clear();

        if (cancel_) {
            break;
        }

        if (delegate) {
            delegate->notifyExportProgress(totalTime, totalTime);
        }

        const float scaleFactor = sampleValueMax == 0 ? 0.f : 21.f / sampleValueMax;
        for (const int16_t chunk : chunks) {
            const float waveformValue = chunk * scaleFactor * 0.8f;
            waveform.push_back(waveformValue > 1.f ? waveformValue : 1.f);
        }

        result = true;
    } while (false);

    if (audioDecoder) {
        audioDecoder->close();

        delete audioDecoder;
        audioDecoder = nullptr;
    }

    if (audioDecoderDelegate) {
        delete audioDecoderDelegate;
        audioDecoderDelegate = nullptr;
    }

    mediaDemuxer.close();

    if (audioFilter) {
        audioFilter->close();

        delete audioFilter;
        audioFilter = nullptr;
    }

    clearBuffer();

    if (delegate) {
        delegate->notifyExportFinish();
    }

    return result;
}

int AudioUtilInternal::waveform(char *inData, int inDataSize, float **waveform, int *waveformSize, const MediaUtilDelegate *delegate)
{
    bool result = false;
    int error = 0;

    cancel_ = false;

    MediaDemuxer mediaDemuxer;
    AudioDecoder* audioDecoder{nullptr};
    DecoderDelegate* audioDecoderDelegate{nullptr};
    AudioFilter* audioFilter{nullptr};
    int16_t sampleValueMax = 0;
    std::vector<int16_t> sampleValueChunk;
    const size_t sampleValueChunkLimit = 30;
    vector<int16_t> chunks;

    do {
        if (!mediaDemuxer.open(inData, inDataSize)) {
            error = 1;
            break;
        }

        if (mediaDemuxer.getMediaType() != MEDIA_VIDEO_AUDIO_TYPE) {
            error = 2;
            break;
        }

        const int64_t totalTime = mediaDemuxer.getDuration();

        audioDecoderDelegate = new AudioDecoderDelegateImpl(this);
        audioDecoder = new AudioDecoder(audioDecoderDelegate);
        if (!audioDecoder->open(CODEC_SOFTWARE_TYPE, &mediaDemuxer)) {
            error = 3;
            break;
        }

        audioFilter = new AudioFilter;
        string filterDescription;
        const std::map<CodecKey, CodecValue>& inCodecInfos = audioDecoder->getCodecInfos();
        std::map<CodecKey, CodecValue> outCodecInfos = inCodecInfos;
        outCodecInfos[CODEC_AUDIO_SAMPLE_FORMAT_EKY] = AV_SAMPLE_FMT_S16;
        outCodecInfos[CODEC_AUDIO_CHANNELS_KEY] = 1;
        outCodecInfos[CODEC_AUDIO_CHANNEL_LAYOUT_KEY] = static_cast<uint64_t>(av_get_default_channel_layout(1));
        int samples = 1024;
        auto it = outCodecInfos.find(CODEC_AUDIO_SAMPLE_SIZE_KEY);
        if (it != outCodecInfos.cend()) {
            samples = std::get<int>(it->second);
        }
        if (samples == 0) {
            samples = 1024;
        }
        filterDescription += "asetnsamples=n=" + std::to_string(samples) + ":p=0";
        if (!audioFilter->open(filterDescription, inCodecInfos, outCodecInfos)) {
            error = 4;
            break;
        }

        mediaDemuxer.seek(0);

        PacketContext* packetCtx = nullptr;
        while (true) {
            if (cancel_) {
                break;
            }

            if (!packetCtx) {
                packetCtx = mediaDemuxer.read();
            }
            if (!packetCtx) {
                break;
            }

            if (packetCtx->getPacketType() != PacketContext::PACKET_AUDIO_TYPE) {
                delete packetCtx;
                packetCtx = nullptr;
                continue;
            }

            if (audioDecoder->decode(packetCtx)) {
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
            audioFilter->add(frame);

            Frame* filterFrame = audioFilter->get();
            if (!filterFrame) {
                continue;
            }
            if (delegate) {
                delegate->notifyExportProgress(filterFrame->getTime(), totalTime);
            }

            switch (filterFrame->getFrameType()) {
            case Frame::FRAME_FFMPEG_TYPE: {
                AVFrame* avFrame = static_cast<AVFrame*>(filterFrame->getFrame());
                const int16_t* data = (int16_t*)(avFrame->data[0]);
                for (int i = 0; i < avFrame->nb_samples; i++) {
                    const int16_t sampleValue = data[i];
                    sampleValueChunk.push_back(sampleValue);
                    if (sampleValueChunk.size() < sampleValueChunkLimit) {
                        continue;
                    }
                    const int16_t sampleValueMaxInChunk = *std::max_element(sampleValueChunk.begin(), sampleValueChunk.end());
                    chunks.push_back(sampleValueMaxInChunk);
                    if (sampleValueMaxInChunk > sampleValueMax) {
                        sampleValueMax = sampleValueMaxInChunk;
                    }
                    sampleValueChunk.clear();
                }
            }
                break;
            default:
                break;
            }

            delete filterFrame;
            filterFrame = nullptr;

            delete frameCtx;
            frameCtx = nullptr;
        }

        audioFilter->add(nullptr);

        do {
            if (cancel_) {
                break;
            }

            Frame* filterFrame = audioFilter->get();
            if (!filterFrame) {
                break;
            }
            if (delegate) {
                delegate->notifyExportProgress(filterFrame->getTime(), totalTime);
            }

            switch (filterFrame->getFrameType()) {
            case Frame::FRAME_FFMPEG_TYPE: {
                AVFrame* avFrame = static_cast<AVFrame*>(filterFrame->getFrame());
                const int16_t* data = (int16_t*)(avFrame->data[0]);
                for (int i = 0; i < avFrame->nb_samples; i++) {
                    const int16_t sampleValue = data[i];
                    sampleValueChunk.push_back(sampleValue);
                    if (sampleValueChunk.size() < sampleValueChunkLimit) {
                        continue;
                    }
                    const int16_t sampleValueMaxInChunk = *std::max_element(sampleValueChunk.begin(), sampleValueChunk.end());
                    chunks.push_back(sampleValueMaxInChunk);
                    if (sampleValueMaxInChunk > sampleValueMax) {
                        sampleValueMax = sampleValueMaxInChunk;
                    }
                    sampleValueChunk.clear();
                }
            }
                break;
            default:
                break;
            }

            delete filterFrame;
            filterFrame = nullptr;
        } while (true);

        const int16_t sampleValueMaxInChunk = *std::max_element(sampleValueChunk.begin(), sampleValueChunk.end());
        chunks.push_back(sampleValueMaxInChunk);
        if (sampleValueMaxInChunk > sampleValueMax) {
            sampleValueMax = sampleValueMaxInChunk;
        }
        sampleValueChunk.clear();

        if (cancel_) {
            error = 5;
            break;
        }

        if (delegate) {
            delegate->notifyExportProgress(totalTime, totalTime);
        }

        std::vector<float> waveforms;
        const float scaleFactor = sampleValueMax == 0 ? 0.f : 21.f / sampleValueMax;
        for (const int16_t chunk : chunks) {
            const float waveformValue = chunk * scaleFactor * 0.8f;
            waveforms.push_back(waveformValue > 1.f ? waveformValue : 1.f);
        }
        *waveform = waveforms.data();
        *waveformSize = chunks.size();

        result = true;
    } while (false);

    if (audioDecoder) {
        audioDecoder->close();

        delete audioDecoder;
        audioDecoder = nullptr;
    }

    if (audioDecoderDelegate) {
        delete audioDecoderDelegate;
        audioDecoderDelegate = nullptr;
    }

    mediaDemuxer.close();

    if (audioFilter) {
        audioFilter->close();

        delete audioFilter;
        audioFilter = nullptr;
    }

    clearBuffer();

    if (delegate) {
        delegate->notifyExportFinish();
    }

    (void)result;
    return error;
}

int AudioUtilInternal::testFile(char *data, int dataSize)
{
    (void)data;(void)dataSize;
    int error = 0;

    MediaDemuxer mediaDemuxer;

    do {
#if 0
        unsigned char imageData[] = {0xff, 0xd8, 0xff, 0xe0, 0x00, 0x10, 0x4a, 0x46, 0x49, 0x46, 0x00, 0x01, 0x01, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0xff, 0xe2, 0x02, 0x28, 0x49, 0x43, 0x43, 0x5f, 0x50, 0x52, 0x4f, 0x46, 0x49, 0x4c, 0x45, 0x00, 0x01, 0x01, 0x00, 0x00, 0x02, 0x18, 0x00, 0x00, 0x00, 0x00, 0x02, 0x10, 0x00, 0x00, 0x6d, 0x6e, 0x74, 0x72, 0x52, 0x47, 0x42, 0x20, 0x58, 0x59, 0x5a, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x61, 0x63, 0x73, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0xf6, 0xd6, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0xd3, 0x2d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x09, 0x64, 0x65, 0x73, 0x63, 0x00, 0x00, 0x00, 0xf0, 0x00, 0x00, 0x00, 0x74, 0x72, 0x58, 0x59, 0x5a, 0x00, 0x00, 0x01, 0x64, 0x00, 0x00, 0x00, 0x14, 0x67, 0x58, 0x59, 0x5a, 0x00, 0x00, 0x01, 0x78, 0x00, 0x00, 0x00, 0x14, 0x62, 0x58, 0x59, 0x5a, 0x00, 0x00, 0x01, 0x8c, 0x00, 0x00, 0x00, 0x14, 0x72, 0x54, 0x52, 0x43, 0x00, 0x00, 0x01, 0xa0, 0x00, 0x00, 0x00, 0x28, 0x67, 0x54, 0x52, 0x43, 0x00, 0x00, 0x01, 0xa0, 0x00, 0x00, 0x00, 0x28, 0x62, 0x54, 0x52, 0x43, 0x00, 0x00, 0x01, 0xa0, 0x00, 0x00, 0x00, 0x28, 0x77, 0x74, 0x70, 0x74, 0x00, 0x00, 0x01, 0xc8, 0x00, 0x00, 0x00, 0x14, 0x63, 0x70, 0x72, 0x74, 0x00, 0x00, 0x01, 0xdc, 0x00, 0x00, 0x00, 0x3c, 0x6d, 0x6c, 0x75, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0c, 0x65, 0x6e, 0x55, 0x53, 0x00, 0x00, 0x00, 0x58, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x73, 0x00, 0x52, 0x00, 0x47, 0x00, 0x42, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x58, 0x59, 0x5a, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x6f, 0xa2, 0x00, 0x00, 0x38, 0xf5, 0x00, 0x00, 0x03, 0x90, 0x58, 0x59, 0x5a, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x62, 0x99, 0x00, 0x00, 0xb7, 0x85, 0x00, 0x00, 0x18, 0xda, 0x58, 0x59, 0x5a, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x24, 0xa0, 0x00, 0x00, 0x0f, 0x84, 0x00, 0x00, 0xb6, 0xcf, 0x70, 0x61, 0x72, 0x61, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x02, 0x66, 0x66, 0x00, 0x00, 0xf2, 0xa7, 0x00, 0x00, 0x0d, 0x59, 0x00, 0x00, 0x13, 0xd0, 0x00, 0x00, 0x0a, 0x5b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x58, 0x59, 0x5a, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf6, 0xd6, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0xd3, 0x2d, 0x6d, 0x6c, 0x75, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0c, 0x65, 0x6e, 0x55, 0x53, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x47, 0x00, 0x6f, 0x00, 0x6f, 0x00, 0x67, 0x00, 0x6c, 0x00, 0x65, 0x00, 0x20, 0x00, 0x49, 0x00, 0x6e, 0x00, 0x63, 0x00, 0x2e, 0x00, 0x20, 0x00, 0x32, 0x00, 0x30, 0x00, 0x31, 0x00, 0x36, 0xff, 0xdb, 0x00, 0x43, 0x00, 0x03, 0x02, 0x02, 0x02, 0x02, 0x02, 0x03, 0x02, 0x02, 0x02, 0x03, 0x03, 0x03, 0x03, 0x04, 0x06, 0x04, 0x04, 0x04, 0x04, 0x04, 0x08, 0x06, 0x06, 0x05, 0x06, 0x09, 0x08, 0x0a, 0x0a, 0x09, 0x08, 0x09, 0x09, 0x0a, 0x0c, 0x0f, 0x0c, 0x0a, 0x0b, 0x0e, 0x0b, 0x09, 0x09, 0x0d, 0x11, 0x0d, 0x0e, 0x0f, 0x10, 0x10, 0x11, 0x10, 0x0a, 0x0c, 0x12, 0x13, 0x12, 0x10, 0x13, 0x0f, 0x10, 0x10, 0x10, 0xff, 0xdb, 0x00, 0x43, 0x01, 0x03, 0x03, 0x03, 0x04, 0x03, 0x04, 0x08, 0x04, 0x04, 0x08, 0x10, 0x0b, 0x09, 0x0b, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0xff, 0xc0, 0x00, 0x11, 0x08, 0x00, 0x0a, 0x00, 0x0a, 0x03, 0x01, 0x22, 0x00, 0x02, 0x11, 0x01, 0x03, 0x11, 0x01, 0xff, 0xc4, 0x00, 0x15, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0xff, 0xc4, 0x00, 0x14, 0x10, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xc4, 0x00, 0x14, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xff, 0xc4, 0x00, 0x14, 0x11, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xda, 0x00, 0x0c, 0x03, 0x01, 0x00, 0x02, 0x11, 0x03, 0x11, 0x00, 0x3f, 0x00, 0x95, 0x00, 0x2d, 0x89, 0xdf, 0xff, 0xd9};
        int imageDataSize = 841;
        unsigned char* copyImageData = new unsigned char[imageDataSize];
        memcpy(copyImageData, imageData, imageDataSize);

        if (!mediaDemuxer.open2((const char*)copyImageData, imageDataSize)) {
            break;
        }
#else
        if (!mediaDemuxer.open2(data, dataSize)) {
            error = 1;
            break;
        }
#endif

        if (mediaDemuxer.getMediaType() != MEDIA_STILL_PICTURE_TYPE) {
            error = 2;
            break;
        }
    } while (false);

    mediaDemuxer.close();

    return error;
}

int AudioUtilInternal::getInfo(char *inData, int inDataSize, char **coverData, int *coverDataSize, int *coverWidth, int *coverHeight, int *coverStride, std::map<MetadataType, string> &metadatas)
{
    bool result = false;
    int error = 0;

    MediaDemuxer mediaDemuxer;

    do {
        if (!mediaDemuxer.open(inData, inDataSize)) {
            error = 1;
            break;
        }

        if (coverData && coverDataSize && coverWidth && coverHeight && coverStride) {
            if (mediaDemuxer.getFormatType() != FORMAT_MP3_TYPE && mediaDemuxer.getFormatType() != FORMAT_FLAC_TYPE) {
                error = 2;
                break;
            }

            ImageFrame* imageFrame = nullptr;

            VideoDecoder* videoDecoder{nullptr};
            DecoderDelegate* videoDecoderDelegate{nullptr};

            do {
                videoDecoderDelegate = new VideoDecoderDelegateImpl(this);
                videoDecoder = new VideoDecoder(videoDecoderDelegate);
                if (!videoDecoder->open(CODEC_SOFTWARE_TYPE, &mediaDemuxer)) {
                    break;
                }

                mediaDemuxer.seek(0);

                PacketContext* packetCtx = nullptr;
                while (true) {
                    if (!packetCtx) {
                        packetCtx = mediaDemuxer.read();
                    }
                    if (!packetCtx) {
                        break;
                    }

                    if (packetCtx->getPacketType() != PacketContext::PACKET_VIDEO_TYPE) {
                        delete packetCtx;
                        packetCtx = nullptr;
                        continue;
                    }

                    if (videoDecoder->decode(packetCtx)) {
                        delete packetCtx;
                        packetCtx = nullptr;
                    }

                    FrameContext* frameCtx = nullptr;
                    {
                        std::lock_guard<std::mutex> lock(videoDecoderBufferMutex_);
                        (void)lock;
                        while (!videoDecoderBuffer_.empty()) {
                            frameCtx = videoDecoderBuffer_.front();
                            videoDecoderBuffer_.pop();
                            break;
                        }
                    }
                    if (!frameCtx) {
                        continue;
                    }

                    Frame* frame = frameCtx->getFrame();
                    Frame* convertFrame = frame->convert(Frame::FRAME_IMAGE_TYPE);
                    imageFrame = dynamic_cast<ImageFrame*>(convertFrame);
                }
            } while (false);

            if (videoDecoder) {
                videoDecoder->close();

                delete videoDecoder;
                videoDecoder = nullptr;
            }

            if (videoDecoderDelegate) {
                delete videoDecoderDelegate;
                videoDecoderDelegate = nullptr;
            }

            if (imageFrame) {
                const MSize size = imageFrame->getSize();
                *coverWidth = size.getWidth();
                *coverHeight = size.getHeight();
                *coverStride = *coverWidth * 4;
                *coverData = (char*)imageFrame->getFrame();
                *coverDataSize = *coverStride * *coverHeight;

                delete imageFrame;
                imageFrame = nullptr;
            }
        }

        metadatas = mediaDemuxer.getMetadatas();

        result = true;
    } while (false);

    mediaDemuxer.close();

    (void)result;
    return error;
}

int AudioUtilInternal::getInfo(char *inData, int inDataSize, char **coverData, int *coverDataSize, std::map<MetadataType, string> &metadatas, int64_t *duration)
{
    bool result = false;
    int error = 0;

    MediaDemuxer mediaDemuxer;

    do {
        if (!mediaDemuxer.open2(inData, inDataSize)) {
            error = 1;
            break;
        }

        if (coverData && coverDataSize) {
            AVFormatContext* formatCtx = static_cast<AVFormatContext*>(mediaDemuxer.getFormatContext());
            for (unsigned int i = 0; i < formatCtx->nb_streams; i++) {
                if (formatCtx->streams[i]->disposition & AV_DISPOSITION_ATTACHED_PIC) {
                    AVPacket packet = formatCtx->streams[i]->attached_pic;
                    *coverDataSize = packet.size;
                    *coverData = (char*)malloc(*coverDataSize);
                    memcpy(*coverData, packet.data, *coverDataSize);
                }
            }
        }

        metadatas = mediaDemuxer.getMetadatas();

        if (duration) {
            *duration = mediaDemuxer.getDuration();
        }

        result = true;
    } while (false);

    mediaDemuxer.close();

    (void)result;
    return error;
}

int AudioUtilInternal::streamIO(const string &inFilePath, const string &outFilePath)
{
    MediaDemuxer mediaDemuxer;
    AudioDecoder* audioDecoder{nullptr};
    DecoderDelegate* audioDecoderDelegate{nullptr};
    AudioEncoder* audioEncoder{nullptr};
    EncoderDelegate* audioEncoderDelegate{nullptr};
    MediaMuxer mediaMuxer;
    int error = 0;

    do {
        if (!File::exist(inFilePath)) {
            error = 1;
            break;
        }
        if (File::exist(outFilePath)) {
            error = 2;
            break;
        }

        if (!mediaDemuxer.open2(inFilePath)) {
            error = 3;
            break;
        }
        if (!mediaMuxer.open2(outFilePath)) {
            error = 4;
            break;
        }

        if (!(mediaDemuxer.getMediaType() == MEDIA_VIDEO_AUDIO_TYPE && mediaMuxer.getMediaType() == MEDIA_VIDEO_AUDIO_TYPE)) {
            break;
        }

        mediaMuxer.setMuxerType(MediaMuxer::MUXER_FFMPEG_TYPE);

#if 0
        const int defaultAudioIndex = MediaLibrary::getDefaultAudioIndex(static_cast<AVFormatContext*>(mediaDemuxer.getFormatContext()));
        const int defaultVideoIndex = MediaLibrary::getDefaultVideoIndex(static_cast<AVFormatContext*>(mediaDemuxer.getFormatContext()));
        if (defaultAudioIndex == -1 && defaultVideoIndex == -1) {
            error = 5;
            break;
        }
        if (defaultAudioIndex != -1) {
            mediaMuxer.addStream(&mediaDemuxer, defaultAudioIndex);
        }
        if (defaultVideoIndex != -1) {
            mediaMuxer.addStream(&mediaDemuxer, defaultVideoIndex);
        }
#endif

        audioDecoderDelegate = new AudioDecoderDelegateImpl(this);
        audioDecoder = new AudioDecoder(audioDecoderDelegate);
        if (!audioDecoder->open(CODEC_SOFTWARE_TYPE, &mediaDemuxer)) {
            break;
        }

        audioEncoderDelegate = new AudioEncoderDelegateImpl(this);
        audioEncoder = new AudioEncoder(audioEncoderDelegate);
        std::map<CodecKey, CodecValue> codecInfos = audioDecoder->getCodecInfos();
        switch (mediaMuxer.getFormatType()) {
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
        if (!audioEncoder->open(CODEC_SOFTWARE_TYPE, &mediaMuxer, codecInfos)) {
            break;
        }

        if (!mediaMuxer.start()) {
            error = 6;
            break;
        }

        PacketContext* packetCtx = nullptr;
        while (true) {
            if (cancel_) {
                break;
            }

            if (!packetCtx) {
                packetCtx = mediaDemuxer.read();
            }
            if (!packetCtx) {
                break;
            }

            if (packetCtx->getPacketType() != PacketContext::PACKET_AUDIO_TYPE) {
                delete packetCtx;
                packetCtx = nullptr;
                continue;
            }

            if (audioDecoder->decode(packetCtx)) {
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
            if (audioEncoder->encode(frame)) {
            }
            delete frameCtx;
            frameCtx = nullptr;

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
            mediaMuxer.write(packetCtx);
            delete packetCtx;
            packetCtx = nullptr;
        }

        if (cancel_) {
            break;
        }
    } while (false);

    if (audioDecoder) {
        audioDecoder->close();

        delete audioDecoder;
        audioDecoder = nullptr;
    }

    if (audioDecoderDelegate) {
        delete audioDecoderDelegate;
        audioDecoderDelegate = nullptr;
    }

    mediaDemuxer.close();

    if (audioEncoder) {
        audioEncoder->close();

        delete audioEncoder;
        audioEncoder = nullptr;
    }

    if (audioEncoderDelegate) {
        delete audioEncoderDelegate;
        audioEncoderDelegate = nullptr;
    }

    mediaMuxer.finish();
    mediaMuxer.close();

    clearBuffer();

    return error;
}

}

