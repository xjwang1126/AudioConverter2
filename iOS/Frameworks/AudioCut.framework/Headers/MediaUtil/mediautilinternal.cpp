#include "mediautilinternal.h"
#include "mediautildelegate.h"

#include "Common/Model/packetcontext.h"
#include "Common/Model/framecontext.h"
#include "Common/Model/ffmpegframe.h"
#include "Common/Media/mediacommon.h"
#include "Common/Media/mediademuxer.h"
#include "Common/Media/mediamuxer.h"
#include "Common/Codec/Decoder/videodecoder.h"
#include "Common/Codec/Decoder/audiodecoder.h"
#include "Common/Codec/Encoder/videoencoder.h"
#include "Common/Codec/Encoder/audioencoder.h"
#include "Common/Filter/VideoFilter/videofilter.h"
#include "Common/Filter/AudioFilter/audiofilter.h"
#include "Common/Util/file.h"

#include <queue>
#include <stack>
#include <cassert>
#include <fstream>

namespace MediaLibrary {

MediaUtilInternal::MediaUtilInternal()
{
}

void MediaUtilInternal::setQuietLog()
{
    av_log_set_level(AV_LOG_QUIET);
}

bool MediaUtilInternal::transcode(const string &inFilePath, const string &outFilePath, const int64_t startTime, const int64_t endTime, const VideoFormat &videoFormat, const AudioFormat &audioFormat, const std::map<TaskType, int> &extraTasks, const MediaUtilDelegate *delegate)
{
    bool result = false;

    cancel_ = false;

    MediaDemuxer mediaDemuxer;
    VideoDecoder* videoDecoder{nullptr};
    DecoderDelegate* videoDecoderDelegate{nullptr};
    AudioDecoder* audioDecoder{nullptr};
    DecoderDelegate* audioDecoderDelegate{nullptr};
    VideoFilter* videoFilter{nullptr};
    AudioFilter* audioFilter{nullptr};
    VideoEncoder* videoEncoder{nullptr};
    EncoderDelegate* videoEncoderDelegate{nullptr};
    AudioEncoder* audioEncoder{nullptr};
    EncoderDelegate* audioEncoderDelegate{nullptr};
    MediaMuxer mediaMuxer;

    do {
        if (!File::exist(inFilePath)) {
            break;
        }
        if (File::exist(outFilePath)) {
            break;
        }

        if (!mediaDemuxer.open(inFilePath)) {
            break;
        }
        if (!mediaMuxer.open(outFilePath)) {
            break;
        }

        if (mediaDemuxer.getMediaType() != mediaMuxer.getMediaType()) {
            break;
        }

        const int64_t duration = mediaDemuxer.getDuration();
        const int64_t curStartTime = startTime == -1 ? 0 : startTime;
        const int64_t curEndTime = endTime == -1 ? duration : endTime;
        const int64_t totalTime = curEndTime - curStartTime;
        if (totalTime > duration) {
            break;
        }

        const MSize inSize = mediaDemuxer.getSize();
        const MSize maxSize(videoFormat.width == -1 ? inSize.getWidth() : videoFormat.width, videoFormat.height == -1 ? inSize.getHeight() : videoFormat.height);
        MSize outSize;
        scaleSizeByMin(inSize, maxSize, &outSize);

        const float fps = videoFormat.fps < 0.f ? mediaDemuxer.getFPS() : videoFormat.fps;

        bool error = false;
        switch (mediaDemuxer.getMediaType()) {
        case MEDIA_STILL_PICTURE_TYPE: {
            if (!mediaDemuxer.openPicture(outSize)) {
                error = true;
                break;
            }
            ImageFrame* pictureFrame = mediaDemuxer.getPicture();
            if (!mediaMuxer.savePicture(pictureFrame)) {
                error = true;
                break;
            }
            mediaDemuxer.closePicture();
        }
            break;
        case MEDIA_MOTION_PICTURE_TYPE: {
            mediaMuxer.setMuxerType(MediaMuxer::MUXER_FFMPEG_TYPE);

            videoDecoderDelegate = new VideoDecoderDelegateImpl(this);
            videoDecoder = new VideoDecoder(videoDecoderDelegate);
            if (!videoDecoder->open(CODEC_SOFTWARE_TYPE, &mediaDemuxer)) {
                error = true;
                break;
            }

            int rotate = 0;
            {
                auto it = extraTasks.find(TASK_ROTATE_TYPE);
                if (it != extraTasks.end()) {
                    rotate = it->second;
                }
            }

            videoEncoderDelegate = new VideoEncoderDelegateImpl(this);
            videoEncoder = new VideoEncoder(videoEncoderDelegate);
            std::map<CodecKey, CodecValue> codecInfos;
            codecInfos[CODEC_ID_KEY] = AV_CODEC_ID_GIF;
            codecInfos[CODEC_VIDEO_PIXEL_FORMAT_KEY] = AV_PIX_FMT_PAL8;
            if (rotate == 90 || rotate == 270) {
                codecInfos[CODEC_VIDEO_WIDTH_KEY] = static_cast<int>(outSize.getHeight());
                codecInfos[CODEC_VIDEO_HEIGHT_KEY] = static_cast<int>(outSize.getWidth());
            } else {
                codecInfos[CODEC_VIDEO_WIDTH_KEY] = static_cast<int>(outSize.getWidth());
                codecInfos[CODEC_VIDEO_HEIGHT_KEY] = static_cast<int>(outSize.getHeight());
            }
            codecInfos[CODEC_VIDEO_FPS_KEY] = fps;
            if (!videoEncoder->open(CODEC_SOFTWARE_TYPE, &mediaMuxer, codecInfos)) {
                error = true;
                break;
            }

            videoFilter = new VideoFilter;
            string filterDescription;
            filterDescription += "scale=" + std::to_string(static_cast<int>(outSize.getWidth())) + ":" + std::to_string(static_cast<int>(outSize.getHeight()));
            string singleFilterDescription;
            auto it = extraTasks.find(TASK_VERTICAL_FLIP_TYPE);
            if (it != extraTasks.end()) {
                singleFilterDescription = "vflip";
                filterDescription += (filterDescription.empty() ? "" : ",") + singleFilterDescription;
            }
            it = extraTasks.find(TASK_HORIZONTAL_FLIP_TYPE);
            if (it != extraTasks.end()) {
                singleFilterDescription = "hflip";
                filterDescription += (filterDescription.empty() ? "" : ",") + singleFilterDescription;
            }
            switch (rotate) {
            case 0:
                break;
            case 90:
                singleFilterDescription = "transpose=1";
                filterDescription += (filterDescription.empty() ? "" : ",") + singleFilterDescription;
                break;
            case 180:
                singleFilterDescription = "transpose=1,transpose=1";
                filterDescription += (filterDescription.empty() ? "" : ",") + singleFilterDescription;
                break;
            case 270:
                singleFilterDescription = "transpose=1,transpose=1,transpose=1";
                filterDescription += (filterDescription.empty() ? "" : ",") + singleFilterDescription;
                break;
            default:
                break;
            }
            const std::map<CodecKey, CodecValue>& inCodecInfos = videoDecoder->getCodecInfos();
            const std::map<CodecKey, CodecValue>& outCodecInfos = videoEncoder->getCodecInfos();
            if (!videoFilter->open(filterDescription, inCodecInfos, outCodecInfos)) {
                error = true;
                break;
            }

            if (!mediaMuxer.start()) {
                error = true;
                break;
            }

            mediaDemuxer.seek(curStartTime);

            int64_t requestTime = curStartTime;
            int64_t frameDuration = static_cast<int64_t>(SECOND_TO_MILLISECOND_UNIT / fps);
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
                        if (((requestTime >= frameCtx->getTime()) && (requestTime < (frameCtx->getTime() + frameCtx->getDuration()))) || (requestTime < frameCtx->getTime())) {
                            break;
                        } else {
                            videoDecoderBuffer_.pop();
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
                videoFilter->add(copyFrame);
                delete copyFrame;
                copyFrame = nullptr;

                if (requestTime > curEndTime) {
                    break;
                }

                requestTime += frameDuration;

                Frame* filterFrame = videoFilter->get();
                if (!filterFrame) {
                    continue;
                }
                if (delegate) {
                    delegate->notifyExportProgress(filterFrame->getTime(), totalTime);
                }
                if (videoEncoder->encode(filterFrame)) {
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
                mediaMuxer.write(packetCtx);
                delete packetCtx;
                packetCtx = nullptr;
            }

            videoFilter->add(nullptr);

            do {
                if (cancel_) {
                    break;
                }

                Frame* filterFrame = videoFilter->get();
                if (!filterFrame) {
                    break;
                }
                if (delegate) {
                    delegate->notifyExportProgress(filterFrame->getTime(), totalTime);
                }
                if (videoEncoder->encode(filterFrame)) {
                }
                delete filterFrame;
                filterFrame = nullptr;
            } while (true);

            videoEncoder->encode(nullptr);

            do {
                if (cancel_) {
                    break;
                }

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
                    break;
                }
                mediaMuxer.write(packetCtx);
                delete packetCtx;
                packetCtx = nullptr;
            } while (true);
        }
            break;
        case MEDIA_VIDEO_AUDIO_TYPE: {
            mediaMuxer.setMuxerType(MediaMuxer::MUXER_FFMPEG_TYPE);

            const bool haveVideo = mediaDemuxer.haveVideo() && mediaMuxer.haveVideo();
            const bool haveAudio = mediaDemuxer.haveAudio() && mediaMuxer.haveAudio();

            if (haveVideo) {
                videoDecoderDelegate = new VideoDecoderDelegateImpl(this);
                videoDecoder = new VideoDecoder(videoDecoderDelegate);
                if (!videoDecoder->open(CODEC_SOFTWARE_TYPE, &mediaDemuxer)) {
                    error = true;
                    break;
                }

                int rotate = 0;
                {
                    auto it = extraTasks.find(TASK_ROTATE_TYPE);
                    if (it != extraTasks.end()) {
                        rotate = it->second;
                    }
                }

                videoEncoderDelegate = new VideoEncoderDelegateImpl(this);
                videoEncoder = new VideoEncoder(videoEncoderDelegate);
                std::map<CodecKey, CodecValue> codecInfos;
                codecInfos[CODEC_ID_KEY] = AV_CODEC_ID_H264;
                codecInfos[CODEC_VIDEO_PIXEL_FORMAT_KEY] = AV_PIX_FMT_YUV420P;
                if (rotate == 90 || rotate == 270) {
                    codecInfos[CODEC_VIDEO_WIDTH_KEY] = static_cast<int>(outSize.getHeight());
                    codecInfos[CODEC_VIDEO_HEIGHT_KEY] = static_cast<int>(outSize.getWidth());
                } else {
                    codecInfos[CODEC_VIDEO_WIDTH_KEY] = static_cast<int>(outSize.getWidth());
                    codecInfos[CODEC_VIDEO_HEIGHT_KEY] = static_cast<int>(outSize.getHeight());
                }
                codecInfos[CODEC_VIDEO_FPS_KEY] = fps;
                if (!videoEncoder->open(CODEC_SOFTWARE_TYPE, &mediaMuxer, codecInfos)) {
                    error = true;
                    break;
                }

                videoFilter = new VideoFilter;
                string filterDescription;
                filterDescription += "scale=" + std::to_string(static_cast<int>(outSize.getWidth())) + ":" + std::to_string(static_cast<int>(outSize.getHeight()));
                string singleFilterDescription;
                auto it = extraTasks.find(TASK_VERTICAL_FLIP_TYPE);
                if (it != extraTasks.end()) {
                    singleFilterDescription = "vflip";
                    filterDescription += (filterDescription.empty() ? "" : ",") + singleFilterDescription;
                }
                it = extraTasks.find(TASK_HORIZONTAL_FLIP_TYPE);
                if (it != extraTasks.end()) {
                    singleFilterDescription = "hflip";
                    filterDescription += (filterDescription.empty() ? "" : ",") + singleFilterDescription;
                }
                switch (rotate) {
                case 0:
                    break;
                case 90:
                    singleFilterDescription = "transpose=1";
                    filterDescription += (filterDescription.empty() ? "" : ",") + singleFilterDescription;
                    break;
                case 180:
                    singleFilterDescription = "transpose=1,transpose=1";
                    filterDescription += (filterDescription.empty() ? "" : ",") + singleFilterDescription;
                    break;
                case 270:
                    singleFilterDescription = "transpose=1,transpose=1,transpose=1";
                    filterDescription += (filterDescription.empty() ? "" : ",") + singleFilterDescription;
                    break;
                default:
                    break;
                }
                const std::map<CodecKey, CodecValue>& inCodecInfos = videoDecoder->getCodecInfos();
                const std::map<CodecKey, CodecValue>& outCodecInfos = videoEncoder->getCodecInfos();
                if (!videoFilter->open(filterDescription, inCodecInfos, outCodecInfos)) {
                    error = true;
                    break;
                }
            }

            if (haveAudio) {
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
                }
                    break;
                case FORMAT_FLAC_TYPE: {
                    codecInfos[CODEC_ID_KEY] = AV_CODEC_ID_FLAC;
                }
                    break;
                case FORMAT_OGG_TYPE: {
                    codecInfos[CODEC_ID_KEY] = AV_CODEC_ID_VORBIS;
                }
                    break;
                case FORMAT_WAV_TYPE: {
                    codecInfos[CODEC_ID_KEY] = AV_CODEC_ID_PCM_S16LE;
                }
                    break;
                case FORMAT_M4A_TYPE: {
                    codecInfos[CODEC_ID_KEY] = AV_CODEC_ID_AAC;
                }
                    break;
                case FORMAT_MP4_TYPE: {
                    codecInfos[CODEC_ID_KEY] = AV_CODEC_ID_AAC;
                }
                    break;
                default:
                    break;
                }
                if (audioFormat.sampleRate != -1) {
                    codecInfos[CODEC_AUDIO_SAMPLE_RATE_KEY] = audioFormat.sampleRate;
                }
                if (audioFormat.channels != -1) {
                    codecInfos[CODEC_AUDIO_CHANNELS_KEY] = audioFormat.channels;
                    codecInfos[CODEC_AUDIO_CHANNEL_LAYOUT_KEY] = static_cast<uint64_t>(av_get_default_channel_layout(audioFormat.channels));
                }
                if (!audioEncoder->open(CODEC_SOFTWARE_TYPE, &mediaMuxer, codecInfos)) {
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
                filterDescription += "asetnsamples=n=" + std::to_string(samples) + ":p=0";
                if (!audioFilter->open(filterDescription, inCodecInfos, outCodecInfos)) {
                    break;
                }
            }

            if (!mediaMuxer.start()) {
                error = true;
                break;
            }

            mediaDemuxer.seek(curStartTime);

            int64_t videoRequestTime = curStartTime;
            int64_t audioRequestTime = curStartTime;
            int64_t videoFrameTime = 0;
            int64_t audioFrameTime = 0;
            int64_t frameDuration = static_cast<int64_t>(SECOND_TO_MILLISECOND_UNIT / fps);
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

                switch (packetCtx->getPacketType()) {
                case PacketContext::PACKET_VIDEO_TYPE: {
                    if (!haveVideo) {
                        delete packetCtx;
                        packetCtx = nullptr;
                        break;
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
                            if (((videoRequestTime >= frameCtx->getTime()) && (videoRequestTime < (frameCtx->getTime() + frameCtx->getDuration()))) || (videoRequestTime < frameCtx->getTime())) {
                                break;
                            } else {
                                videoDecoderBuffer_.pop();
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
                    const int64_t frameTime = videoRequestTime - curStartTime;
                    copyFrame->setTime(frameTime);
                    videoFilter->add(copyFrame);
                    delete copyFrame;
                    copyFrame = nullptr;

                    if (videoRequestTime > curEndTime) {
                        break;
                    }

                    videoRequestTime += frameDuration;

                    Frame* filterFrame = videoFilter->get();
                    if (!filterFrame) {
                        continue;
                    }
                    videoFrameTime = filterFrame->getTime();

                    if (videoEncoder->encode(filterFrame)) {
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
                    mediaMuxer.write(packetCtx);
                    delete packetCtx;
                    packetCtx = nullptr;
                }
                    break;
                case PacketContext::PACKET_AUDIO_TYPE: {
                    if (!haveAudio) {
                        delete packetCtx;
                        packetCtx = nullptr;
                        break;
                    }

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
                            if (((audioRequestTime >= frameCtx->getTime()) && (audioRequestTime < (frameCtx->getTime() + frameCtx->getDuration()))) || (audioRequestTime < frameCtx->getTime())) {
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
                    const int64_t frameTime = audioRequestTime - curStartTime;
                    copyFrame->setTime(frameTime);
                    audioFilter->add(copyFrame);
                    delete copyFrame;
                    copyFrame = nullptr;

                    if (audioRequestTime > curEndTime) {
                        break;
                    }

                    audioRequestTime += frame->getDuration();

                    Frame* filterFrame = audioFilter->get();
                    if (!filterFrame) {
                        continue;
                    }
                    audioFrameTime = filterFrame->getTime();
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
                    break;
                default:
                    break;
                }

                if (haveVideo) {
                    if (delegate) {
                        delegate->notifyExportProgress(videoFrameTime, totalTime);
                    }
                    if (videoRequestTime > curEndTime) {
                        break;
                    }
                } else if (haveAudio) {
                    if (delegate) {
                        delegate->notifyExportProgress(audioFrameTime, totalTime);
                    }
                    if (audioRequestTime > curEndTime) {
                        break;
                    }
                }
            }

            if (haveVideo) {
                videoFilter->add(nullptr);

                do {
                    if (cancel_) {
                        break;
                    }

                    Frame* filterFrame = videoFilter->get();
                    if (!filterFrame) {
                        break;
                    }
                    if (delegate) {
                        delegate->notifyExportProgress(filterFrame->getTime(), totalTime);
                    }
                    if (videoEncoder->encode(filterFrame)) {
                    }
                    delete filterFrame;
                    filterFrame = nullptr;
                } while (true);

                videoEncoder->encode(nullptr);

                do {
                    if (cancel_) {
                        break;
                    }

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
                        break;
                    }
                    mediaMuxer.write(packetCtx);
                    delete packetCtx;
                    packetCtx = nullptr;
                } while (true);
            }

            if (haveAudio) {
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
        }
            break;
        default:
            break;
        }
        if (error) {
            break;
        }
        if (cancel_) {
            break;
        }

        if (delegate) {
            delegate->notifyExportProgress(totalTime, totalTime);
        }

        result = true;
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

    if (videoEncoder) {
        videoEncoder->close();

        delete videoEncoder;
        videoEncoder = nullptr;
    }

    if (videoEncoderDelegate) {
        delete videoEncoderDelegate;
        videoEncoderDelegate = nullptr;
    }

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

    if (videoFilter) {
        videoFilter->close();

        delete videoFilter;
        videoFilter = nullptr;
    }

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

bool MediaUtilInternal::reverse(const string &inFilePath, const string &outFilePath, const MediaUtilDelegate *delegate)
{
    bool result = false;

    cancel_ = false;

    MediaDemuxer mediaDemuxer;
    VideoDecoder* videoDecoder{nullptr};
    DecoderDelegate* videoDecoderDelegate{nullptr};
    VideoFilter* videoFilter{nullptr};
    VideoEncoder* videoEncoder{nullptr};
    EncoderDelegate* videoEncoderDelegate{nullptr};
    MediaMuxer mediaMuxer;

    do {
        if (!File::exist(inFilePath)) {
            break;
        }
        if (File::exist(outFilePath)) {
            break;
        }

        if (!mediaDemuxer.open(inFilePath)) {
            break;
        }
        if (!mediaMuxer.open(outFilePath)) {
            break;
        }

        if (mediaDemuxer.getMediaType() != mediaMuxer.getMediaType() && mediaDemuxer.getMediaType() == MEDIA_STILL_PICTURE_TYPE) {
            break;
        }

        const int64_t duration = mediaDemuxer.getDuration();
        const int64_t curStartTime = 0;
        const int64_t curEndTime = duration;
        const int64_t totalTime = curEndTime - curStartTime;

        bool error = false;
        switch (mediaDemuxer.getMediaType()) {
        case MEDIA_MOTION_PICTURE_TYPE: {
            mediaMuxer.setMuxerType(MediaMuxer::MUXER_FFMPEG_TYPE);

            videoDecoderDelegate = new VideoDecoderDelegateImpl(this);
            videoDecoder = new VideoDecoder(videoDecoderDelegate);
            if (!videoDecoder->open(CODEC_SOFTWARE_TYPE, &mediaDemuxer)) {
                error = true;
                break;
            }

            videoEncoderDelegate = new VideoEncoderDelegateImpl(this);
            videoEncoder = new VideoEncoder(videoEncoderDelegate);
            std::map<CodecKey, CodecValue> codecInfos = videoDecoder->getCodecInfos();
            codecInfos[CODEC_VIDEO_PIXEL_FORMAT_KEY] = AV_PIX_FMT_PAL8;
            if (!videoEncoder->open(CODEC_SOFTWARE_TYPE, &mediaMuxer, codecInfos)) {
                error = true;
                break;
            }

            videoFilter = new VideoFilter;
            string filterDescription;
            filterDescription += "null";
            const std::map<CodecKey, CodecValue>& inCodecInfos = videoDecoder->getCodecInfos();
            const std::map<CodecKey, CodecValue>& outCodecInfos = videoEncoder->getCodecInfos();
            if (!videoFilter->open(filterDescription, inCodecInfos, outCodecInfos)) {
                error = true;
                break;
            }

            if (!mediaMuxer.start()) {
                error = true;
                break;
            }

            mediaDemuxer.seek(curStartTime);

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

                if (packetCtx->getPacketType() != PacketContext::PACKET_VIDEO_TYPE) {
                    delete packetCtx;
                    packetCtx = nullptr;
                    continue;
                }

                if (videoDecoder->decode(packetCtx)) {
                    delete packetCtx;
                    packetCtx = nullptr;
                }
            }

            {
                std::lock_guard<std::mutex> lock(videoDecoderBufferMutex_);
                (void)lock;
                std::stack<FrameContext*> stack;
                while (!videoDecoderBuffer_.empty()) {
                    stack.push(videoDecoderBuffer_.front());
                    videoDecoderBuffer_.pop();
                }
                while (!stack.empty()) {
                    videoDecoderBuffer_.push(stack.top());
                    stack.pop();
                }
            }

            while (true) {
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
                    break;
                }

                Frame* frame = frameCtx->getFrame();
                const int64_t frameTime = duration - frame->getTime();
                frame->setTime(frameTime);
                videoFilter->add(frame);
                if (delegate) {
                    delegate->notifyExportProgress(frameTime, totalTime);
                }

                Frame* filterFrame = videoFilter->get();
                if (!filterFrame) {
                    continue;
                }
                if (videoEncoder->encode(filterFrame)) {
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
                mediaMuxer.write(packetCtx);
                delete packetCtx;
                packetCtx = nullptr;
            }

            videoFilter->add(nullptr);

            do {
                if (cancel_) {
                    break;
                }

                Frame* filterFrame = videoFilter->get();
                if (!filterFrame) {
                    break;
                }
                if (delegate) {
                    delegate->notifyExportProgress(filterFrame->getTime(), totalTime);
                }
                if (videoEncoder->encode(filterFrame)) {
                }
                delete filterFrame;
                filterFrame = nullptr;
            } while (true);

            videoEncoder->encode(nullptr);

            do {
                if (cancel_) {
                    break;
                }

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
                    break;
                }
                mediaMuxer.write(packetCtx);
                delete packetCtx;
                packetCtx = nullptr;
            } while (true);
        }
            break;
        case MEDIA_VIDEO_AUDIO_TYPE: {
        }
            break;
        default:
            break;
        }
        if (error) {
            break;
        }
        if (cancel_) {
            break;
        }

        if (delegate) {
            delegate->notifyExportProgress(totalTime, totalTime);
        }

        result = true;
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

    mediaDemuxer.close();

    if (videoEncoder) {
        videoEncoder->close();

        delete videoEncoder;
        videoEncoder = nullptr;
    }

    if (videoEncoderDelegate) {
        delete videoEncoderDelegate;
        videoEncoderDelegate = nullptr;
    }

    mediaMuxer.finish();
    mediaMuxer.close();

    if (videoFilter) {
        videoFilter->close();

        delete videoFilter;
        videoFilter = nullptr;
    }

    clearBuffer();

    if (delegate) {
        delegate->notifyExportFinish();
    }

    return result;
}

bool MediaUtilInternal::memoryFile(const string &inFilePath, const string &outFilePath, const MediaUtilDelegate *delegate)
{
    bool result = false;

    do {
        if (!File::exist(inFilePath)) {
            break;
        }
        if (File::exist(outFilePath)) {
            break;
        }

        std::ifstream inFile(inFilePath, std::ios::binary);

        if (!inFile.is_open()) {
            break;
        }

        inFile.seekg(0, std::ios::end);
        std::streamsize size = inFile.tellg();
        inFile.seekg(0, std::ios::beg);

        std::vector<char> buffer(size);

        if (!inFile.read(buffer.data(), size)) {
            break;
        }

        inFile.close();

#if !USE_CUSTOM_COMPONENT
        unsigned char* data = reinterpret_cast<unsigned char*>(buffer.data());
        int dataSize = size;

        AVFormatContext *inFormatCtx = avformat_alloc_context();
        if (!inFormatCtx) {
            break;
        }

        AVIOContext *inAVIOCtx = avio_alloc_context(data, dataSize, 0, nullptr, nullptr, nullptr, nullptr);
        if (!inAVIOCtx) {
            break;
        }

        inFormatCtx->pb = inAVIOCtx;

        const char *inFilename = "dummy";
        int ret = 0;
        if ((ret = avformat_open_input(&inFormatCtx, inFilename, nullptr, nullptr)) < 0) {
            break;
        }

        if ((ret = avformat_find_stream_info(inFormatCtx, nullptr)) < 0) {
            break;
        }

        struct BufferData bufferData;
        memset(&bufferData, 0, sizeof(BufferData));
        const size_t bufferSize = 1024;
        const size_t outAVIOCtxBufferSize = 4096;

        bufferData.ptr = bufferData.buf = static_cast<uint8_t*>(av_malloc(bufferSize));
        if (!bufferData.buf) {
            ret = AVERROR(ENOMEM);
            break;
        }
        bufferData.size = bufferData.room = bufferSize;

        uint8_t *avio_ctx_buffer = static_cast<uint8_t*>(av_malloc(outAVIOCtxBufferSize));
        if (!avio_ctx_buffer) {
            ret = AVERROR(ENOMEM);
            break;
        }

        auto readPacketFunc = [](void *opaque, uint8_t *buffer, int bufferSize)->int {
            (void)opaque;(void)buffer;(void)bufferSize;
            return 0;
        };
        auto writePacketFunc = [](void *opaque, uint8_t *buffer, int bufferSize)->int {
            struct BufferData *bufferData = (struct BufferData *)opaque;
            while (bufferSize > bufferData->room) {
                int64_t offset = bufferData->ptr - bufferData->buf;
                bufferData->buf = static_cast<uint8_t*>(av_realloc_f(bufferData->buf, 2, bufferData->size));
                if (!bufferData->buf)
                    return AVERROR(ENOMEM);
                bufferData->size *= 2;
                bufferData->ptr = bufferData->buf + offset;
                bufferData->room = bufferData->size - offset;
            }

            memcpy(bufferData->ptr, buffer, bufferSize);
            bufferData->ptr  += bufferSize;
            bufferData->room -= bufferSize;

            return bufferSize;
        };
        auto seekFunc = [](void *opaque, int64_t offset, int whence)->int64_t {
            struct BufferData *bufferData = (struct BufferData *)opaque;
            switch(whence){
                case SEEK_SET:
                    bufferData->ptr = bufferData->buf + offset;
//                    return bd->ptr;
                    break;
                case SEEK_CUR:
                    bufferData->ptr += offset;
                    break;
                case SEEK_END:
                    bufferData->ptr = (bufferData->buf + bufferData->size) + offset;
//                    return bd->ptr;
                    break;
                case AVSEEK_SIZE:
                    return bufferData->size;
                    break;
                default:
                   return -1;
            }
            return 1;
        };
        AVIOContext *outAVIOCtx = avio_alloc_context(avio_ctx_buffer, outAVIOCtxBufferSize, 1, &bufferData, readPacketFunc, writePacketFunc, seekFunc);
        if (!outAVIOCtx) {
            break;
        }

        AVFormatContext *outFormatCtx = nullptr;
        if ((ret = avformat_alloc_output_context2(&outFormatCtx, nullptr, "gif", nullptr)) < 0) {
            break;
        }

        outFormatCtx->pb = outAVIOCtx;
        outFormatCtx->flags |= AVFMT_FLAG_CUSTOM_IO;

        for (unsigned int i = 0; i < inFormatCtx->nb_streams; i++) {
            AVStream *inStream = inFormatCtx->streams[i];
            AVStream *outStream = avformat_new_stream(outFormatCtx, nullptr);
            if (!outStream) {
                ret = -1;
                break;
            }
            avcodec_parameters_copy(outStream->codecpar, inStream->codecpar);
            outStream->codecpar->codec_tag = 0;
        }
        if (ret < 0) {
            break;
        }

        if ((ret = avformat_write_header(outFormatCtx, nullptr)) < 0) {
            break;
        }

        AVPacket packet;
        while (av_read_frame(inFormatCtx, &packet) >= 0) {
            av_write_frame(outFormatCtx, &packet);
            av_packet_unref(&packet);
        }

        if ((ret = av_write_trailer(outFormatCtx)) != 0) {
            break;
        }

        avformat_close_input(&inFormatCtx);
        inFormatCtx = nullptr;

        avformat_free_context(outFormatCtx);
        outFormatCtx = nullptr;

        std::ofstream outFile(outFilePath, std::ios::binary);

        if (!outFile.is_open()) {
            break;
        }

        outFile.write(reinterpret_cast<char*>(bufferData.buf), bufferData.size - bufferData.room);

        outFile.close();
#else
        int error = 0;
        (void)error;

        cancel_ = false;

        MediaDemuxer mediaDemuxer;
        MediaMuxer mediaMuxer;

        do {
            const int inDataSize = size;
            char* inData = static_cast<char*>(malloc(inDataSize));
            memcpy(inData, buffer.data(), size);

            if (!mediaDemuxer.open(inData, inDataSize)) {
                error = 1;
                break;
            }
            if (!mediaMuxer.open(mediaDemuxer.getFormatType())) {
                error = 2;
                break;
            }

            mediaMuxer.setMuxerType(MediaMuxer::MUXER_FFMPEG_TYPE);

            const int defaultVideoIndex = MediaLibrary::getDefaultVideoIndex(static_cast<AVFormatContext*>(mediaDemuxer.getFormatContext()));
            if (defaultVideoIndex == -1) {
                error = 4;
                break;
            }
            mediaMuxer.addStream(&mediaDemuxer, defaultVideoIndex);

            if (!mediaMuxer.start()) {
                error = 5;
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

                if (packetCtx->getPacketType() != PacketContext::PACKET_VIDEO_TYPE) {
                    delete packetCtx;
                    packetCtx = nullptr;
                    continue;
                }

                mediaMuxer.write(packetCtx);
                delete packetCtx;
                packetCtx = nullptr;
            }

            if (cancel_) {
                error = 6;
                break;
            }
        } while (false);

        mediaDemuxer.close();

        mediaMuxer.finish();
        mediaMuxer.close();

        clearBuffer();

        if (delegate) {
            delegate->notifyExportFinish();
        }

        std::ofstream outFile(outFilePath, std::ios::binary);

        if (!outFile.is_open()) {
            break;
        }

        const BufferData& bufferData = mediaMuxer.getBufferData();
        outFile.write(reinterpret_cast<char*>(bufferData.buf), bufferData.size - bufferData.room);

        outFile.close();
#endif

        result = true;
    } while (false);

    return result;
}

const string MediaUtilInternal::checkSupportFormatCodec()
{
    string supportFormatCodec;

    supportFormatCodec += "Supported input formats:\n";
    void *i = nullptr;
    const AVInputFormat *inFormat = nullptr;
    while ((inFormat = av_demuxer_iterate(&i))) {
        supportFormatCodec += inFormat->name;
        supportFormatCodec += "\n";
    }

    supportFormatCodec += "\nSupported output formats:\n";
    void *o = nullptr;
    const AVOutputFormat *outFormat = nullptr;
    while ((outFormat = av_muxer_iterate(&o))) {
        supportFormatCodec += outFormat->name;
        supportFormatCodec += "\n";
    }

    string supportDecoderInfo;
    string supportEncoderInfo;
    void *codecIt = nullptr;
    const AVCodec *codec = nullptr;
    while ((codec = av_codec_iterate(&codecIt))) {
        if (av_codec_is_decoder(codec)) {
            supportDecoderInfo += codec->name;
            supportDecoderInfo += "\n";
        } else if (av_codec_is_encoder(codec)) {
            supportEncoderInfo += codec->name;
            supportEncoderInfo += "\n";
        }
    }
    supportFormatCodec += "\nSupported decoders:\n";
    supportFormatCodec += supportDecoderInfo;
    supportFormatCodec += "\nSupported encoders:\n";
    supportFormatCodec += supportDecoderInfo;

    return supportFormatCodec;
}

MediaInfo MediaUtilInternal::checkMediaInfo(const string &filePath)
{
    MediaInfo mediaInfo;
    MediaDemuxer mediaDemuxer;

    do {
        if (!File::exist(filePath)) {
            break;
        }

        if (!mediaDemuxer.open(filePath)) {
            break;
        }

        if (mediaDemuxer.getMediaType() == MEDIA_NONE_TYPE) {
            break;
        }

        AVFormatContext* formatCtx = static_cast<AVFormatContext*>(mediaDemuxer.getFormatContext());
        if (!formatCtx) {
            break;
        }
        mediaInfo.format = formatCtx->iformat->name;
        mediaInfo.duration = formatCtx->duration;
        mediaInfo.streams = formatCtx->nb_streams;
    } while (false);

    return mediaInfo;
}

void MediaUtilInternal::cancel()
{
    cancel_ = true;
}

void MediaUtilInternal::decodeVideoFrame(FrameContext *frameCtx)
{
    std::lock_guard<std::mutex> guard(videoDecoderBufferMutex_);
    (void)guard;
    videoDecoderBuffer_.push(frameCtx);
}

void MediaUtilInternal::decodeAudioFrame(FrameContext *frameCtx)
{
    std::lock_guard<std::mutex> guard(audioDecoderBufferMutex_);
    (void)guard;
    audioDecoderBuffer_.push(frameCtx);
}

void MediaUtilInternal::encodeVideoPacket(PacketContext *packetCtx)
{
    std::lock_guard<std::mutex> guard(videoEncoderBufferMutex_);
    (void)guard;
    videoEncoderBuffer_.push(packetCtx);
}

void MediaUtilInternal::encodeAudioPacket(PacketContext *packetCtx)
{
    std::lock_guard<std::mutex> guard(audioEncoderBufferMutex_);
    (void)guard;
    audioEncoderBuffer_.push(packetCtx);
}

void MediaUtilInternal::clearBuffer()
{
    while (!videoDecoderBuffer_.empty()) {
        FrameContext* frameCtx = videoDecoderBuffer_.front();
        videoDecoderBuffer_.pop();
        delete frameCtx;
        frameCtx = nullptr;
    }

    while (!audioDecoderBuffer_.empty()) {
        FrameContext* frameCtx = audioDecoderBuffer_.front();
        audioDecoderBuffer_.pop();
        delete frameCtx;
        frameCtx = nullptr;
    }

    while (!videoEncoderBuffer_.empty()) {
        PacketContext* packetCtx = videoEncoderBuffer_.front();
        videoEncoderBuffer_.pop();
        delete packetCtx;
        packetCtx = nullptr;
    }

    while (!audioEncoderBuffer_.empty()) {
        PacketContext* packetCtx = audioEncoderBuffer_.front();
        audioEncoderBuffer_.pop();
        delete packetCtx;
        packetCtx = nullptr;
    }
}

MediaUtilInternal::VideoDecoderDelegateImpl::VideoDecoderDelegateImpl(MediaUtilInternal *mediaUtil)
    : mediaUtil_(mediaUtil)
{
}

MediaUtilInternal::VideoDecoderDelegateImpl::~VideoDecoderDelegateImpl()
{
    mediaUtil_ = nullptr;
}

void MediaUtilInternal::VideoDecoderDelegateImpl::output(FrameContext *frameCtx)
{
    if (mediaUtil_) {
        mediaUtil_->decodeVideoFrame(frameCtx);
    }
}

MediaUtilInternal::AudioDecoderDelegateImpl::AudioDecoderDelegateImpl(MediaUtilInternal *mediaUtil)
    : mediaUtil_(mediaUtil)
{
}

MediaUtilInternal::AudioDecoderDelegateImpl::~AudioDecoderDelegateImpl()
{
    mediaUtil_ = nullptr;
}

void MediaUtilInternal::AudioDecoderDelegateImpl::output(FrameContext *frameCtx)
{
    if (mediaUtil_) {
        mediaUtil_->decodeAudioFrame(frameCtx);
    }
}

MediaUtilInternal::VideoEncoderDelegateImpl::VideoEncoderDelegateImpl(MediaUtilInternal *mediaUtil)
    : mediaUtil_(mediaUtil)
{
}

MediaUtilInternal::VideoEncoderDelegateImpl::~VideoEncoderDelegateImpl()
{
    mediaUtil_ = nullptr;
}

void MediaUtilInternal::VideoEncoderDelegateImpl::output(PacketContext *packetCtx)
{
    if (mediaUtil_) {
        mediaUtil_->encodeVideoPacket(packetCtx);
    }
}

MediaUtilInternal::AudioEncoderDelegateImpl::AudioEncoderDelegateImpl(MediaUtilInternal *mediaUtil)
    : mediaUtil_(mediaUtil)
{
}

MediaUtilInternal::AudioEncoderDelegateImpl::~AudioEncoderDelegateImpl()
{
    mediaUtil_ = nullptr;
}

void MediaUtilInternal::AudioEncoderDelegateImpl::output(PacketContext *packetCtx)
{
    if (mediaUtil_) {
        mediaUtil_->encodeAudioPacket(packetCtx);
    }
}

}
