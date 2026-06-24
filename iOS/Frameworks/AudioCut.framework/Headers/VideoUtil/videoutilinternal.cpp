#include "videoutilinternal.h"
#include "MediaUtil/mediautildelegate.h"

#include "Common/Model/packetcontext.h"
#include "Common/Model/framecontext.h"
#include "Common/Model/imageframe.h"
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

#include <cassert>

namespace MediaLibrary {

VideoUtilInternal::VideoUtilInternal()
{
}

bool VideoUtilInternal::videoToGIF(const string &inFilePath, const string &outFilePath, const int64_t startTime, const int64_t endTime, const VideoFormat &videoFormat, const GIFMode &gifMode, const MediaUtilDelegate *delegate)
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

        if (!(mediaDemuxer.getMediaType() == MEDIA_VIDEO_AUDIO_TYPE && mediaMuxer.getMediaType() == MEDIA_MOTION_PICTURE_TYPE)) {
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
        const MSize size(videoFormat.width == -1 ? inSize.getWidth() : videoFormat.width, videoFormat.height == -1 ? inSize.getHeight() : videoFormat.height);
        MSize outSize;
        scaleSizeByMax(inSize, std::min(size.getWidth(), size.getHeight()), &outSize);

        const float fps = videoFormat.fps < 0.f ? mediaDemuxer.getFPS() : videoFormat.fps;

        mediaMuxer.setMuxerType(MediaMuxer::MUXER_FFMPEG_TYPE);

        videoDecoderDelegate = new VideoDecoderDelegateImpl(this);
        videoDecoder = new VideoDecoder(videoDecoderDelegate);
        if (!videoDecoder->open(CODEC_SOFTWARE_TYPE, &mediaDemuxer)) {
            break;
        }

        videoEncoderDelegate = new VideoEncoderDelegateImpl(this);
        videoEncoder = new VideoEncoder(videoEncoderDelegate);
        std::map<CodecKey, CodecValue> codecInfos;
        codecInfos[CODEC_ID_KEY] = AV_CODEC_ID_GIF;
        codecInfos[CODEC_VIDEO_PIXEL_FORMAT_KEY] = AV_PIX_FMT_PAL8;
        codecInfos[CODEC_VIDEO_WIDTH_KEY] = static_cast<int>(outSize.getWidth());
        codecInfos[CODEC_VIDEO_HEIGHT_KEY] = static_cast<int>(outSize.getHeight());
        codecInfos[CODEC_VIDEO_FPS_KEY] = fps;
        if (!videoEncoder->open(CODEC_SOFTWARE_TYPE, &mediaMuxer, codecInfos)) {
            break;
        }

        videoFilter = new VideoFilter;
        string filterDescription;
        const std::map<CodecKey, CodecValue>& inCodecInfos = videoDecoder->getCodecInfos();
        const std::map<CodecKey, CodecValue>& outCodecInfos = videoEncoder->getCodecInfos();
        switch (gifMode) {
        case GIF_FAST_MODE:
            filterDescription += "scale=" + std::to_string(static_cast<int>(outSize.getWidth())) + ":" + std::to_string(static_cast<int>(outSize.getHeight()));
            break;
        case GIF_QUALITY_MODE:
            filterDescription += "scale=" + std::to_string(static_cast<int>(outSize.getWidth())) + ":" + std::to_string(static_cast<int>(outSize.getHeight())) + ":flags=lanczos,split [o1] [o2];[o1] palettegen [p]; [o2] fifo [o3];[o3] [p] paletteuse";
            break;
        default:
            break;
        }
        if (!videoFilter->open(filterDescription, inCodecInfos, outCodecInfos)) {
            break;
        }

        if (!mediaMuxer.start()) {
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

int VideoUtilInternal::videoToGIF(char *inData, int inDataSize, char **outData, int *outDataSize, const int64_t startTime, const int64_t endTime, const VideoFormat &videoFormat, const GIFMode &gifMode, const MediaUtilDelegate *delegate)
{
    bool result = false;
    int error = 0;

    cancel_ = false;

    MediaDemuxer mediaDemuxer;
    VideoDecoder* videoDecoder{nullptr};
    DecoderDelegate* videoDecoderDelegate{nullptr};
    VideoFilter* videoFilter{nullptr};
    VideoEncoder* videoEncoder{nullptr};
    EncoderDelegate* videoEncoderDelegate{nullptr};
    MediaMuxer mediaMuxer;

    do {
        if (!mediaDemuxer.open(inData, inDataSize)) {
            error = 1;
            break;
        }
        if (!mediaMuxer.open(FORMAT_GIF_TYPE)) {
            error = 2;
            break;
        }

        const int64_t duration = mediaDemuxer.getDuration();
        const int64_t curStartTime = startTime == -1 ? 0 : startTime;
        const int64_t curEndTime = endTime == -1 ? duration : endTime;
        const int64_t totalTime = curEndTime - curStartTime;
        if (totalTime > duration) {
            error = 3;
            break;
        }

        const MSize inSize = mediaDemuxer.getSize();
        const MSize size(videoFormat.width == -1 ? inSize.getWidth() : videoFormat.width, videoFormat.height == -1 ? inSize.getHeight() : videoFormat.height);
        MSize outSize;
        scaleSizeByMax(inSize, std::min(size.getWidth(), size.getHeight()), &outSize);

        const float fps = videoFormat.fps < 0.f ? mediaDemuxer.getFPS() : videoFormat.fps;
        (void)fps;

        mediaMuxer.setMuxerType(MediaMuxer::MUXER_FFMPEG_TYPE);

        videoDecoderDelegate = new VideoDecoderDelegateImpl(this);
        videoDecoder = new VideoDecoder(videoDecoderDelegate);
        if (!videoDecoder->open(CODEC_SOFTWARE_TYPE, &mediaDemuxer)) {
            break;
        }

        videoEncoderDelegate = new VideoEncoderDelegateImpl(this);
        videoEncoder = new VideoEncoder(videoEncoderDelegate);
        std::map<CodecKey, CodecValue> codecInfos;
        codecInfos[CODEC_ID_KEY] = AV_CODEC_ID_GIF;
        codecInfos[CODEC_VIDEO_PIXEL_FORMAT_KEY] = AV_PIX_FMT_PAL8;
        codecInfos[CODEC_VIDEO_WIDTH_KEY] = static_cast<int>(outSize.getWidth());
        codecInfos[CODEC_VIDEO_HEIGHT_KEY] = static_cast<int>(outSize.getHeight());
        codecInfos[CODEC_VIDEO_FPS_KEY] = fps;
        if (!videoEncoder->open(CODEC_SOFTWARE_TYPE, &mediaMuxer, codecInfos)) {
            break;
        }

        videoFilter = new VideoFilter;
        string filterDescription;
        const std::map<CodecKey, CodecValue>& inCodecInfos = videoDecoder->getCodecInfos();
        const std::map<CodecKey, CodecValue>& outCodecInfos = videoEncoder->getCodecInfos();
        switch (gifMode) {
        case GIF_FAST_MODE:
            filterDescription += "scale=" + std::to_string(static_cast<int>(outSize.getWidth())) + ":" + std::to_string(static_cast<int>(outSize.getHeight()));
            break;
        case GIF_QUALITY_MODE:
            filterDescription += "scale=" + std::to_string(static_cast<int>(outSize.getWidth())) + ":" + std::to_string(static_cast<int>(outSize.getHeight())) + ":flags=lanczos,split [o1] [o2];[o1] palettegen [p]; [o2] fifo [o3];[o3] [p] paletteuse";
            break;
        default:
            break;
        }
        if (!videoFilter->open(filterDescription, inCodecInfos, outCodecInfos)) {
            break;
        }

        if (!mediaMuxer.start()) {
            error = 5;
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

        if (cancel_) {
            error = 6;
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

    const BufferData& bufferData = mediaMuxer.getBufferData();
    *outData = (char*)bufferData.buf;
    *outDataSize = bufferData.size - bufferData.room;

    if (delegate) {
        delegate->notifyExportFinish();
    }

    (void)result;
    return error;
}

bool VideoUtilInternal::imageToGIF(const std::vector<string> &inFilePaths, const string &outFilePath, const FillMode &fillMode, const VideoFormat &videoFormat, const GIFMode &gifMode, const MediaUtilDelegate *delegate)
{
    bool result = false;

    cancel_ = false;

    std::vector<MediaDemuxer*> mediaDemuxers;
    VideoFilter* videoFilter{nullptr};
    VideoEncoder* videoEncoder{nullptr};
    EncoderDelegate* videoEncoderDelegate{nullptr};
    MediaMuxer mediaMuxer;

    bool error = false;
    do {
        error = false;
        for (const string& inFilePath : inFilePaths) {
            if (!File::exist(inFilePath)) {
                error = true;
                break;
            }
        }
        if (error) {
            break;
        }
        if (File::exist(outFilePath)) {
            break;
        }

        error = false;
        mediaDemuxers.reserve(inFilePaths.size());
        for (size_t i = 0; i < inFilePaths.size(); i++) {
            const string& inFilePath = inFilePaths.at(i);
            MediaDemuxer* mediaDemuxer = new MediaDemuxer();
            if (!mediaDemuxer->open(inFilePath)) {
                delete mediaDemuxer;
                error = true;
                break;
            }
            mediaDemuxers.push_back(mediaDemuxer);
        }
        if (error) {
            break;
        }
        if (!mediaMuxer.open(outFilePath)) {
            break;
        }

        error = false;
        for (MediaDemuxer* mediaDemuxer : mediaDemuxers) {
            if (mediaDemuxer->getMediaType() != MEDIA_STILL_PICTURE_TYPE) {
                error = true;
                break;
            }
            if (!mediaDemuxer->openPicture(mediaDemuxer->getSize())) {
                error = true;
                break;
            }
        }
        if (error) {
            break;
        }
        if (mediaMuxer.getMediaType() != MEDIA_MOTION_PICTURE_TYPE) {
            break;
        }

        const float fps = videoFormat.fps < 0.f ? 1.f : videoFormat.fps;
        assert(fps > 0.f);

        const size_t frameNumber = inFilePaths.size();
        const int64_t duration = (SECOND_TO_MILLISECOND_UNIT / fps) * frameNumber;
        const int64_t totalTime = duration;

        const MSize inSize = [&mediaDemuxers](){
            if (mediaDemuxers.empty()) {
                return MSize();
            }
            return mediaDemuxers.at(0)->getSize();
        }();
        const MSize maxSize(videoFormat.width == -1 ? inSize.getWidth() : videoFormat.width, videoFormat.height == -1 ? inSize.getHeight() : videoFormat.height);
        MSize outSize;
        scaleSizeByMin(inSize, maxSize, &outSize);

        mediaMuxer.setMuxerType(MediaMuxer::MUXER_FFMPEG_TYPE);

        videoEncoderDelegate = new VideoEncoderDelegateImpl(this);
        videoEncoder = new VideoEncoder(videoEncoderDelegate);
        std::map<CodecKey, CodecValue> codecInfos;
        codecInfos[CODEC_ID_KEY] = AV_CODEC_ID_GIF;
        codecInfos[CODEC_VIDEO_PIXEL_FORMAT_KEY] = AV_PIX_FMT_PAL8;
        codecInfos[CODEC_VIDEO_WIDTH_KEY] = static_cast<int>(outSize.getWidth());
        codecInfos[CODEC_VIDEO_HEIGHT_KEY] = static_cast<int>(outSize.getHeight());
        codecInfos[CODEC_VIDEO_FPS_KEY] = fps;
        if (!videoEncoder->open(CODEC_SOFTWARE_TYPE, &mediaMuxer, codecInfos)) {
            break;
        }

        videoFilter = new VideoFilter;
        string filterDescription;
        AVPixelFormat inPixelFormat = AV_PIX_FMT_NONE;
        switch (gifMode) {
        case GIF_FAST_MODE: {
            inPixelFormat = AV_PIX_FMT_PAL8;
            filterDescription += "null";
        }
            break;
        case GIF_QUALITY_MODE: {
            inPixelFormat = AV_PIX_FMT_RGBA;
            filterDescription += "split [o1] [o2];[o1] palettegen [p]; [o2] fifo [o3];[o3] [p] paletteuse";
        }
            break;
        default:
            break;
        }
        std::map<CodecKey, CodecValue> inCodecInfos;
        inCodecInfos[CODEC_VIDEO_WIDTH_KEY] = static_cast<int>(outSize.getWidth());
        inCodecInfos[CODEC_VIDEO_HEIGHT_KEY] = static_cast<int>(outSize.getHeight());
        inCodecInfos[CODEC_VIDEO_PIXEL_FORMAT_KEY] = inPixelFormat;
        const std::map<CodecKey, CodecValue>& outCodecInfos = videoEncoder->getCodecInfos();
        if (!videoFilter->open(filterDescription, inCodecInfos, outCodecInfos)) {
            break;
        }

        if (!mediaMuxer.start()) {
            break;
        }

        int64_t requestTime = 0;
        int64_t frameDuration = static_cast<int64_t>(SECOND_TO_MILLISECOND_UNIT / fps);
        PacketContext* packetCtx = nullptr;
        for (const MediaDemuxer* mediaDemuxer : mediaDemuxers) {
            if (cancel_) {
                break;
            }

            VideoFilter* scaleVideoFilter = new VideoFilter;
            string filterDescription;
            std::map<CodecKey, CodecValue> inCodecInfos;
            const MSize inSize = mediaDemuxer->getSize();
            inCodecInfos[CODEC_VIDEO_WIDTH_KEY] = static_cast<int>(inSize.getWidth());
            inCodecInfos[CODEC_VIDEO_HEIGHT_KEY] = static_cast<int>(inSize.getHeight());
            inCodecInfos[CODEC_VIDEO_PIXEL_FORMAT_KEY] = AV_PIX_FMT_RGBA;
            std::map<CodecKey, CodecValue> outCodecInfos;
            outCodecInfos[CODEC_VIDEO_WIDTH_KEY] = static_cast<int>(outSize.getWidth());
            outCodecInfos[CODEC_VIDEO_HEIGHT_KEY] = static_cast<int>(outSize.getHeight());
            outCodecInfos[CODEC_VIDEO_PIXEL_FORMAT_KEY] = inPixelFormat;
            const int outWidth = static_cast<int>(outSize.getWidth());
            const int outHeight = static_cast<int>(outSize.getHeight());
            switch (fillMode) {
            case FILL_FIT_MODE:
                filterDescription += "scale=" + std::to_string(outWidth) + ":" + std::to_string(outHeight) + ":force_original_aspect_ratio=decrease";
                break;
            case FILL_FILL_MODE:
                filterDescription += "scale=" + std::to_string(outWidth) + ":" + std::to_string(outHeight) + ":force_original_aspect_ratio=increase";
                break;
            case FILL_STRETCH_MODE:
                filterDescription += "scale=" + std::to_string(outWidth) + ":" + std::to_string(outHeight);
                break;
            default:
                break;
            }
            if (!scaleVideoFilter->open(filterDescription, inCodecInfos, outCodecInfos)) {
                continue;
            }

            const ImageFrame* imageFrame = mediaDemuxer->getPicture();
            Frame* frame = imageFrame->convert(Frame::FRAME_FFMPEG_TYPE);
            const int64_t frameTime = requestTime;
            frame->setTime(frameTime);
            frame->setDuration(frameDuration);
            scaleVideoFilter->add(frame);

            requestTime += frameDuration;

            scaleVideoFilter->add(nullptr);

            Frame* scaleFilterFrame = scaleVideoFilter->get();
            if (!scaleFilterFrame) {
                continue;
            }
            if (delegate) {
                delegate->notifyExportProgress(scaleFilterFrame->getTime(), totalTime);
            }
            switch (fillMode) {
            case FILL_FIT_MODE: {
                Frame* frame = FFmpegFrame::create(outSize, inPixelFormat);
                assert(frame);
                frame->copy(scaleFilterFrame);
                frame->setTime(scaleFilterFrame->getTime());
                frame->setDuration(scaleFilterFrame->getDuration());
                videoFilter->add(frame);
                delete frame;
                frame = nullptr;
            }
                break;
            case FILL_FILL_MODE: {
                Frame* frame = FFmpegFrame::create(outSize, inPixelFormat);
                assert(frame);
                frame->copy(scaleFilterFrame);
                frame->setTime(scaleFilterFrame->getTime());
                frame->setDuration(scaleFilterFrame->getDuration());
                videoFilter->add(frame);
                delete frame;
                frame = nullptr;
            }
                break;
            case FILL_STRETCH_MODE: {
                videoFilter->add(scaleFilterFrame);
            }
                break;
            default:
                break;
            }
            delete scaleFilterFrame;
            scaleFilterFrame = nullptr;

            scaleVideoFilter->close();

            delete scaleVideoFilter;
            scaleVideoFilter = nullptr;

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

        if (cancel_) {
            break;
        }

        if (delegate) {
            delegate->notifyExportProgress(totalTime, totalTime);
        }

        result = true;
    } while (false);

    for (auto it = mediaDemuxers.begin(); it != mediaDemuxers.end();) {
        MediaDemuxer* mediaDemuxer = *it;
        it = mediaDemuxers.erase(it);
        mediaDemuxer->close();
        delete mediaDemuxer;
        mediaDemuxer = nullptr;
    }

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

int VideoUtilInternal::imageToGIF(char *inData, int inDataSize, char **outData, int *outDataSize, const FillMode &fillMode, const VideoFormat &videoFormat, const MediaUtilDelegate *delegate)
{
    bool result = false;
    int error = 0;

    cancel_ = false;

    std::vector<MediaDemuxer*> mediaDemuxers;
    VideoFilter* videoFilter{nullptr};
    VideoEncoder* videoEncoder{nullptr};
    EncoderDelegate* videoEncoderDelegate{nullptr};
    MediaMuxer mediaMuxer;

    do {
        MediaDemuxer* mediaDemuxer = new MediaDemuxer();
        if (!mediaDemuxer->open(inData, inDataSize)) {
            delete mediaDemuxer;
            error = 1;
            break;
        }
        mediaDemuxers.push_back(mediaDemuxer);
        if (!mediaMuxer.open(FORMAT_GIF_TYPE)) {
            error = 2;
            break;
        }

        for (MediaDemuxer* mediaDemuxer : mediaDemuxers) {
            if (mediaDemuxer->getMediaType() != MEDIA_STILL_PICTURE_TYPE) {
                error = 3;
                break;
            }
        }
        if (error) {
            break;
        }
        if (mediaMuxer.getMediaType() != MEDIA_MOTION_PICTURE_TYPE) {
            error = 4;
            break;
        }

        const float fps = videoFormat.fps < 0.f ? 1.f : videoFormat.fps;
        assert(fps > 0.f);

        const size_t frameNumber = mediaDemuxers.size();
        const int64_t duration = (SECOND_TO_MILLISECOND_UNIT / fps) * frameNumber;
        const int64_t totalTime = duration;

        const MSize inSize = [&mediaDemuxers](){
            if (mediaDemuxers.empty()) {
                return MSize();
            }
            return mediaDemuxers.at(0)->getSize();
        }();
        const MSize maxSize(videoFormat.width == -1 ? inSize.getWidth() : videoFormat.width, videoFormat.height == -1 ? inSize.getHeight() : videoFormat.height);
        MSize outSize;
        scaleSizeByMin(inSize, maxSize, &outSize);

        mediaMuxer.setMuxerType(MediaMuxer::MUXER_FFMPEG_TYPE);

        videoEncoderDelegate = new VideoEncoderDelegateImpl(this);
        videoEncoder = new VideoEncoder(videoEncoderDelegate);
        std::map<CodecKey, CodecValue> codecInfos;
        codecInfos[CODEC_ID_KEY] = AV_CODEC_ID_GIF;
        codecInfos[CODEC_VIDEO_PIXEL_FORMAT_KEY] = AV_PIX_FMT_PAL8;
        codecInfos[CODEC_VIDEO_WIDTH_KEY] = static_cast<int>(outSize.getWidth());
        codecInfos[CODEC_VIDEO_HEIGHT_KEY] = static_cast<int>(outSize.getHeight());
        codecInfos[CODEC_VIDEO_FPS_KEY] = fps;
        if (!videoEncoder->open(CODEC_SOFTWARE_TYPE, &mediaMuxer, codecInfos)) {
            error = 5;
            break;
        }

        videoFilter = new VideoFilter;
        string filterDescription;
        std::map<CodecKey, CodecValue> inCodecInfos;
        inCodecInfos[CODEC_VIDEO_WIDTH_KEY] = static_cast<int>(inSize.getWidth());
        inCodecInfos[CODEC_VIDEO_HEIGHT_KEY] = static_cast<int>(inSize.getHeight());
        inCodecInfos[CODEC_VIDEO_PIXEL_FORMAT_KEY] = AV_PIX_FMT_RGBA;
        const std::map<CodecKey, CodecValue>& outCodecInfos = videoEncoder->getCodecInfos();
        const int outWidth = static_cast<int>(outSize.getWidth());
        const int outHeight = static_cast<int>(outSize.getHeight());
        switch (fillMode) {
        case FILL_FIT_MODE:
            filterDescription += "scale=" + std::to_string(outWidth) + ":" + std::to_string(outHeight) + ":force_original_aspect_ratio=decrease";
            break;
        case FILL_FILL_MODE:
            filterDescription += "scale=" + std::to_string(outWidth) + ":" + std::to_string(outHeight) + ":force_original_aspect_ratio=increase";
            break;
        case FILL_STRETCH_MODE:
            filterDescription += "scale=" + std::to_string(outWidth) + ":" + std::to_string(outHeight);
            break;
        default:
            break;
        }
        if (!videoFilter->open(filterDescription, inCodecInfos, outCodecInfos)) {
            error = 6;
            break;
        }

        if (!mediaMuxer.start()) {
            error = 7;
            break;
        }

        int64_t requestTime = 0;
        int64_t frameDuration = static_cast<int64_t>(SECOND_TO_MILLISECOND_UNIT / fps);
        PacketContext* packetCtx = nullptr;
        for (MediaDemuxer* mediaDemuxer : mediaDemuxers) {
            if (cancel_) {
                break;
            }

            VideoDecoder* videoDecoder{nullptr};
            DecoderDelegate* videoDecoderDelegate{nullptr};
            Frame* frame = nullptr;

            do {
                videoDecoderDelegate = new VideoDecoderDelegateImpl(this);
                videoDecoder = new VideoDecoder(videoDecoderDelegate);
                if (!videoDecoder->open(CODEC_SOFTWARE_TYPE, mediaDemuxer)) {
                    break;
                }

                PacketContext* packetCtx = nullptr;
                while (true) {
                    if (!packetCtx) {
                        packetCtx = mediaDemuxer->read();
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
                        }
                    }
                    if (!frameCtx) {
                        continue;
                    }

                    frame = frameCtx->getFrame()->copy();

                    delete frameCtx;
                    frameCtx = nullptr;
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

            const int64_t frameTime = requestTime;
            frame->setTime(frameTime);
            frame->setDuration(frameDuration);
            videoFilter->add(frame);

            requestTime += frameDuration;

            Frame* filterFrame = videoFilter->get();
            if (!filterFrame) {
                continue;
            }
            if (delegate) {
                delegate->notifyExportProgress(filterFrame->getTime(), totalTime);
            }
            switch (fillMode) {
            case FILL_FIT_MODE: {
                Frame* frame = FFmpegFrame::create(outSize, AV_PIX_FMT_PAL8);
                assert(frame);
                frame->copy(filterFrame);
                frame->setTime(filterFrame->getTime());
                frame->setDuration(filterFrame->getDuration());
                if (videoEncoder->encode(frame)) {
                }
                delete frame;
                frame = nullptr;
            }
                break;
            case FILL_FILL_MODE: {
                Frame* frame = FFmpegFrame::create(outSize, AV_PIX_FMT_PAL8);
                assert(frame);
                frame->copy(filterFrame);
                frame->setTime(filterFrame->getTime());
                frame->setDuration(filterFrame->getDuration());
                if (videoEncoder->encode(frame)) {
                }
                delete frame;
                frame = nullptr;
            }
                break;
            case FILL_STRETCH_MODE: {
                if (videoEncoder->encode(filterFrame)) {
                }
            }
                break;
            default:
                break;
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
            switch (fillMode) {
            case FILL_FIT_MODE: {
                Frame* frame = FFmpegFrame::create(outSize, AV_PIX_FMT_PAL8);
                assert(frame);
                frame->copy(filterFrame);
                frame->setTime(filterFrame->getTime());
                frame->setDuration(filterFrame->getDuration());
                if (videoEncoder->encode(frame)) {
                }
                delete frame;
                frame = nullptr;
            }
                break;
            case FILL_FILL_MODE: {
                Frame* frame = FFmpegFrame::create(outSize, AV_PIX_FMT_PAL8);
                assert(frame);
                frame->copy(filterFrame);
                frame->setTime(filterFrame->getTime());
                frame->setDuration(filterFrame->getDuration());
                if (videoEncoder->encode(frame)) {
                }
                delete frame;
                frame = nullptr;
            }
                break;
            case FILL_STRETCH_MODE: {
                if (videoEncoder->encode(filterFrame)) {
                }
            }
                break;
            default:
                break;
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

        if (cancel_) {
            break;
        }

        if (delegate) {
            delegate->notifyExportProgress(totalTime, totalTime);
        }

        result = true;
    } while (false);

    for (auto it = mediaDemuxers.begin(); it != mediaDemuxers.end();) {
        MediaDemuxer* mediaDemuxer = *it;
        it = mediaDemuxers.erase(it);
        mediaDemuxer->close();
        delete mediaDemuxer;
        mediaDemuxer = nullptr;
    }

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

    const BufferData& bufferData = mediaMuxer.getBufferData();
    *outData = (char*)bufferData.buf;
    *outDataSize = bufferData.size - bufferData.room;

    if (delegate) {
        delegate->notifyExportFinish();
    }

    (void)result;
    return error;
}

int VideoUtilInternal::imageDataToGIF(char *inData, int inDataSize, int inWidth, int inHeight, int inStride, char **outData, int *outDataSize, const FillMode &fillMode, const VideoFormat &videoFormat, const MediaUtilDelegate *delegate)
{
    bool result = false;
    int error = -1;

    cancel_ = false;

    VideoFilter* videoFilter{nullptr};
    VideoEncoder* videoEncoder{nullptr};
    EncoderDelegate* videoEncoderDelegate{nullptr};
    MediaMuxer mediaMuxer;

    do {
        if (!inData || inDataSize <= 0 || inWidth <= 0 || inHeight <= 0 || inStride <= 0) {
            error = 1;
            break;
        }

        if (!mediaMuxer.open(FORMAT_GIF_TYPE)) {
            error = 2;
            break;
        }

        if (mediaMuxer.getMediaType() != MEDIA_MOTION_PICTURE_TYPE) {
            error = 3;
            break;
        }

        const float fps = videoFormat.fps < 0.f ? 1.f : videoFormat.fps;
        assert(fps > 0.f);

        const size_t frameNumber = 1;
        const int64_t duration = (SECOND_TO_MILLISECOND_UNIT / fps) * frameNumber;
        const int64_t totalTime = duration;

        const MSize inSize(inWidth, inHeight);
        const MSize maxSize(videoFormat.width == -1 ? inSize.getWidth() : videoFormat.width, videoFormat.height == -1 ? inSize.getHeight() : videoFormat.height);
        MSize outSize;
        scaleSizeByMin(inSize, maxSize, &outSize);

        mediaMuxer.setMuxerType(MediaMuxer::MUXER_FFMPEG_TYPE);

        videoEncoderDelegate = new VideoEncoderDelegateImpl(this);
        videoEncoder = new VideoEncoder(videoEncoderDelegate);
        std::map<CodecKey, CodecValue> codecInfos;
        codecInfos[CODEC_ID_KEY] = AV_CODEC_ID_GIF;
        codecInfos[CODEC_VIDEO_PIXEL_FORMAT_KEY] = AV_PIX_FMT_PAL8;
        codecInfos[CODEC_VIDEO_WIDTH_KEY] = static_cast<int>(outSize.getWidth());
        codecInfos[CODEC_VIDEO_HEIGHT_KEY] = static_cast<int>(outSize.getHeight());
        codecInfos[CODEC_VIDEO_FPS_KEY] = fps;
        if (!videoEncoder->open(CODEC_SOFTWARE_TYPE, &mediaMuxer, codecInfos)) {
            error = 4;
            break;
        }

        videoFilter = new VideoFilter;
        string filterDescription;
        std::map<CodecKey, CodecValue> inCodecInfos;
        inCodecInfos[CODEC_VIDEO_WIDTH_KEY] = static_cast<int>(inSize.getWidth());
        inCodecInfos[CODEC_VIDEO_HEIGHT_KEY] = static_cast<int>(inSize.getHeight());
        inCodecInfos[CODEC_VIDEO_PIXEL_FORMAT_KEY] = AV_PIX_FMT_RGBA;
        const std::map<CodecKey, CodecValue>& outCodecInfos = videoEncoder->getCodecInfos();
        const int outWidth = static_cast<int>(outSize.getWidth());
        const int outHeight = static_cast<int>(outSize.getHeight());
        switch (fillMode) {
        case FILL_FIT_MODE:
            filterDescription += "scale=" + std::to_string(outWidth) + ":" + std::to_string(outHeight) + ":force_original_aspect_ratio=decrease";
            break;
        case FILL_FILL_MODE:
            filterDescription += "scale=" + std::to_string(outWidth) + ":" + std::to_string(outHeight) + ":force_original_aspect_ratio=increase";
            break;
        case FILL_STRETCH_MODE:
            filterDescription += "scale=" + std::to_string(outWidth) + ":" + std::to_string(outHeight);
            break;
        default:
            break;
        }
        if (!videoFilter->open(filterDescription, inCodecInfos, outCodecInfos)) {
            error = 5;
            break;
        }

        if (!mediaMuxer.start()) {
            error = 6;
            break;
        }

        int64_t requestTime = 0;
        int64_t frameDuration = static_cast<int64_t>(SECOND_TO_MILLISECOND_UNIT / fps);
        PacketContext* packetCtx = nullptr;

        ImageFrame* imageFrame = new ImageFrame();
        assert(inDataSize == inStride * inHeight);
        assert(inStride == inWidth * 4);
        imageFrame->setFrame(inData);
        imageFrame->setSize(MSize(inWidth, inHeight));

        Frame* convertFrame = imageFrame->convert(Frame::FRAME_FFMPEG_TYPE);

        delete imageFrame;
        imageFrame = nullptr;

        const int64_t frameTime = requestTime;
        convertFrame->setTime(frameTime);
        convertFrame->setDuration(frameDuration);
        videoFilter->add(convertFrame);

        requestTime += frameDuration;

        delete convertFrame;
        convertFrame = nullptr;

        Frame* filterFrame = videoFilter->get();
        if (!filterFrame) {
            error = 7;
            break;
        }
        if (delegate) {
            delegate->notifyExportProgress(filterFrame->getTime(), totalTime);
        }
        switch (fillMode) {
        case FILL_FIT_MODE: {
            Frame* frame = FFmpegFrame::create(outSize, AV_PIX_FMT_PAL8);
            assert(frame);
            frame->copy(filterFrame);
            frame->setTime(filterFrame->getTime());
            frame->setDuration(filterFrame->getDuration());
            if (videoEncoder->encode(frame)) {
            }
            delete frame;
            frame = nullptr;
        }
            break;
        case FILL_FILL_MODE: {
            Frame* frame = FFmpegFrame::create(outSize, AV_PIX_FMT_PAL8);
            assert(frame);
            frame->copy(filterFrame);
            frame->setTime(filterFrame->getTime());
            frame->setDuration(filterFrame->getDuration());
            if (videoEncoder->encode(frame)) {
            }
            delete frame;
            frame = nullptr;
        }
            break;
        case FILL_STRETCH_MODE: {
            if (videoEncoder->encode(filterFrame)) {
            }
        }
            break;
        default:
            break;
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
            error = 8;
            break;
        }
        mediaMuxer.write(packetCtx);
        delete packetCtx;
        packetCtx = nullptr;

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
            switch (fillMode) {
            case FILL_FIT_MODE: {
                Frame* frame = FFmpegFrame::create(outSize, AV_PIX_FMT_PAL8);
                assert(frame);
                frame->copy(filterFrame);
                frame->setTime(filterFrame->getTime());
                frame->setDuration(filterFrame->getDuration());
                if (videoEncoder->encode(frame)) {
                }
                delete frame;
                frame = nullptr;
            }
                break;
            case FILL_FILL_MODE: {
                Frame* frame = FFmpegFrame::create(outSize, AV_PIX_FMT_PAL8);
                assert(frame);
                frame->copy(filterFrame);
                frame->setTime(filterFrame->getTime());
                frame->setDuration(filterFrame->getDuration());
                if (videoEncoder->encode(frame)) {
                }
                delete frame;
                frame = nullptr;
            }
                break;
            case FILL_STRETCH_MODE: {
                if (videoEncoder->encode(filterFrame)) {
                }
            }
                break;
            default:
                break;
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

        if (cancel_) {
            break;
        }

        if (delegate) {
            delegate->notifyExportProgress(totalTime, totalTime);
        }

        error = 0;
        result = true;
    } while (false);

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

    const BufferData& bufferData = mediaMuxer.getBufferData();
    *outData = (char*)bufferData.buf;
    *outDataSize = bufferData.size - bufferData.room;

    if (delegate) {
        delegate->notifyExportFinish();
    }

    (void)result;
    return error;
}

int VideoUtilInternal::imageDataToPicture(char *inData, int inDataSize, int inWidth, int inHeight, int inStride, char **outData, int *outDataSize, char *outFormatName, const FillMode &fillMode, const VideoFormat &videoFormat, const MediaUtilDelegate *delegate)
{
    bool result = false;
    int error = -1;

    cancel_ = false;

    VideoFilter* videoFilter{nullptr};
    VideoEncoder* videoEncoder{nullptr};
    EncoderDelegate* videoEncoderDelegate{nullptr};
    MediaMuxer mediaMuxer;

    do {
        if (!inData || inDataSize <= 0 || inWidth <= 0 || inHeight <= 0 || inStride <= 0) {
            error = 1;
            break;
        }

        FormatType formatType = FORMAT_NONE_TYPE;
        if (strcmp(outFormatName, "gif") == 0) {
            formatType = FORMAT_GIF_TYPE;
        } else if (strcmp(outFormatName, "jpg") == 0) {
            formatType = FORMAT_JPG_TYPE;
        } else {
            error = 2;
            break;
        }

        if (!mediaMuxer.open(formatType)) {
            error = 3;
            break;
        }

        const float fps = videoFormat.fps < 0.f ? 1.f : videoFormat.fps;
        assert(fps > 0.f);

        const size_t frameNumber = 1;
        const int64_t duration = (SECOND_TO_MILLISECOND_UNIT / fps) * frameNumber;
        const int64_t totalTime = duration;

        const MSize inSize(inWidth, inHeight);
        const MSize maxSize(videoFormat.width == -1 ? inSize.getWidth() : videoFormat.width, videoFormat.height == -1 ? inSize.getHeight() : videoFormat.height);
        MSize outSize;
        scaleSizeByMin(inSize, maxSize, &outSize);

        mediaMuxer.setMuxerType(MediaMuxer::MUXER_FFMPEG_TYPE);

        videoEncoderDelegate = new VideoEncoderDelegateImpl(this);
        videoEncoder = new VideoEncoder(videoEncoderDelegate);
        std::map<CodecKey, CodecValue> codecInfos;
        AVPixelFormat pixelFormat = AV_PIX_FMT_NONE;
        switch (formatType) {
        case FORMAT_GIF_TYPE: {
            codecInfos[CODEC_ID_KEY] = AV_CODEC_ID_GIF;
            pixelFormat = AV_PIX_FMT_PAL8;
            codecInfos[CODEC_VIDEO_PIXEL_FORMAT_KEY] = pixelFormat;
        }
            break;
        case FORMAT_JPG_TYPE: {
            codecInfos[CODEC_ID_KEY] = AV_CODEC_ID_MJPEG;
            pixelFormat = AV_PIX_FMT_YUVJ420P;
            codecInfos[CODEC_VIDEO_PIXEL_FORMAT_KEY] = pixelFormat;
        }
            break;
        default:
            break;
        }
        codecInfos[CODEC_VIDEO_WIDTH_KEY] = static_cast<int>(outSize.getWidth());
        codecInfos[CODEC_VIDEO_HEIGHT_KEY] = static_cast<int>(outSize.getHeight());
        codecInfos[CODEC_VIDEO_FPS_KEY] = fps;
        if (!videoEncoder->open(CODEC_SOFTWARE_TYPE, &mediaMuxer, codecInfos)) {
            error = 4;
            break;
        }

        videoFilter = new VideoFilter;
        string filterDescription;
        std::map<CodecKey, CodecValue> inCodecInfos;
        inCodecInfos[CODEC_VIDEO_WIDTH_KEY] = static_cast<int>(inSize.getWidth());
        inCodecInfos[CODEC_VIDEO_HEIGHT_KEY] = static_cast<int>(inSize.getHeight());
        inCodecInfos[CODEC_VIDEO_PIXEL_FORMAT_KEY] = AV_PIX_FMT_RGBA;
        const std::map<CodecKey, CodecValue>& outCodecInfos = videoEncoder->getCodecInfos();
        const int outWidth = static_cast<int>(outSize.getWidth());
        const int outHeight = static_cast<int>(outSize.getHeight());
        switch (fillMode) {
        case FILL_FIT_MODE:
            filterDescription += "scale=" + std::to_string(outWidth) + ":" + std::to_string(outHeight) + ":force_original_aspect_ratio=decrease";
            break;
        case FILL_FILL_MODE:
            filterDescription += "scale=" + std::to_string(outWidth) + ":" + std::to_string(outHeight) + ":force_original_aspect_ratio=increase";
            break;
        case FILL_STRETCH_MODE:
            filterDescription += "scale=" + std::to_string(outWidth) + ":" + std::to_string(outHeight);
            break;
        default:
            break;
        }
        if (!videoFilter->open(filterDescription, inCodecInfos, outCodecInfos)) {
            error = 5;
            break;
        }

        if (!mediaMuxer.start()) {
            error = 6;
            break;
        }

        int64_t requestTime = 0;
        int64_t frameDuration = static_cast<int64_t>(SECOND_TO_MILLISECOND_UNIT / fps);
        PacketContext* packetCtx = nullptr;

        ImageFrame* imageFrame = new ImageFrame();
        assert(inDataSize == inStride * inHeight);
        assert(inStride == inWidth * 4);
        imageFrame->setFrame(inData);
        imageFrame->setSize(MSize(inWidth, inHeight));

        Frame* convertFrame = imageFrame->convert(Frame::FRAME_FFMPEG_TYPE);

        delete imageFrame;
        imageFrame = nullptr;

        const int64_t frameTime = requestTime;
        convertFrame->setTime(frameTime);
        convertFrame->setDuration(frameDuration);
        videoFilter->add(convertFrame);

        requestTime += frameDuration;

        delete convertFrame;
        convertFrame = nullptr;

        Frame* filterFrame = videoFilter->get();
        if (!filterFrame) {
            error = 7;
            break;
        }
        if (delegate) {
            delegate->notifyExportProgress(filterFrame->getTime(), totalTime);
        }
        switch (fillMode) {
        case FILL_FIT_MODE: {
            Frame* frame = FFmpegFrame::create(outSize, pixelFormat);
            assert(frame);
            frame->copy(filterFrame);
            frame->setTime(filterFrame->getTime());
            frame->setDuration(filterFrame->getDuration());
            if (videoEncoder->encode(frame)) {
            }
            delete frame;
            frame = nullptr;
        }
            break;
        case FILL_FILL_MODE: {
            Frame* frame = FFmpegFrame::create(outSize, pixelFormat);
            assert(frame);
            frame->copy(filterFrame);
            frame->setTime(filterFrame->getTime());
            frame->setDuration(filterFrame->getDuration());
            if (videoEncoder->encode(frame)) {
            }
            delete frame;
            frame = nullptr;
        }
            break;
        case FILL_STRETCH_MODE: {
            if (videoEncoder->encode(filterFrame)) {
            }
        }
            break;
        default:
            break;
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
            error = 8;
            break;
        }
        mediaMuxer.write(packetCtx);
        delete packetCtx;
        packetCtx = nullptr;

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
            switch (fillMode) {
            case FILL_FIT_MODE: {
                Frame* frame = FFmpegFrame::create(outSize, pixelFormat);
                assert(frame);
                frame->copy(filterFrame);
                frame->setTime(filterFrame->getTime());
                frame->setDuration(filterFrame->getDuration());
                if (videoEncoder->encode(frame)) {
                }
                delete frame;
                frame = nullptr;
            }
                break;
            case FILL_FILL_MODE: {
                Frame* frame = FFmpegFrame::create(outSize, pixelFormat);
                assert(frame);
                frame->copy(filterFrame);
                frame->setTime(filterFrame->getTime());
                frame->setDuration(filterFrame->getDuration());
                if (videoEncoder->encode(frame)) {
                }
                delete frame;
                frame = nullptr;
            }
                break;
            case FILL_STRETCH_MODE: {
                if (videoEncoder->encode(filterFrame)) {
                }
            }
                break;
            default:
                break;
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

        if (cancel_) {
            break;
        }

        if (delegate) {
            delegate->notifyExportProgress(totalTime, totalTime);
        }

        error = 0;
        result = true;
    } while (false);

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

    const BufferData& bufferData = mediaMuxer.getBufferData();
    *outData = (char*)bufferData.buf;
    *outDataSize = bufferData.size - bufferData.room;

    if (delegate) {
        delegate->notifyExportFinish();
    }

    (void)result;
    return error;
}

int VideoUtilInternal::imageDataToGIFImageData(char *inData, int inDataSize, int inWidth, int inHeight, int inStride, char **outData, int *outDataSize)
{
    bool result = false;
    int error = -1;

    cancel_ = false;

    VideoFilter* videoFilter{nullptr};
    VideoEncoder* videoEncoder{nullptr};
    EncoderDelegate* videoEncoderDelegate{nullptr};
    MediaMuxer mediaMuxer;

    do {
        if (!inData || inDataSize <= 0 || inWidth <= 0 || inHeight <= 0 || inStride <= 0) {
            error = 1;
            break;
        }

        if (!mediaMuxer.open(FORMAT_GIF_TYPE)) {
            error = 2;
            break;
        }

        if (mediaMuxer.getMediaType() != MEDIA_MOTION_PICTURE_TYPE) {
            error = 3;
            break;
        }

        mediaMuxer.setMuxerType(MediaMuxer::MUXER_FFMPEG_TYPE);

        videoEncoderDelegate = new VideoEncoderDelegateImpl(this);
        videoEncoder = new VideoEncoder(videoEncoderDelegate);
        std::map<CodecKey, CodecValue> codecInfos;
        codecInfos[CODEC_ID_KEY] = AV_CODEC_ID_GIF;
        codecInfos[CODEC_VIDEO_PIXEL_FORMAT_KEY] = AV_PIX_FMT_PAL8;
        codecInfos[CODEC_VIDEO_WIDTH_KEY] = inWidth;
        codecInfos[CODEC_VIDEO_HEIGHT_KEY] = inHeight;
        codecInfos[CODEC_VIDEO_FPS_KEY] = 1.f;
        if (!videoEncoder->open(CODEC_SOFTWARE_TYPE, &mediaMuxer, codecInfos)) {
            error = 4;
            break;
        }

        videoFilter = new VideoFilter;
        string filterDescription;
        std::map<CodecKey, CodecValue> inCodecInfos;
        inCodecInfos[CODEC_VIDEO_WIDTH_KEY] = inWidth;
        inCodecInfos[CODEC_VIDEO_HEIGHT_KEY] = inHeight;
        inCodecInfos[CODEC_VIDEO_PIXEL_FORMAT_KEY] = AV_PIX_FMT_RGBA;
        const std::map<CodecKey, CodecValue>& outCodecInfos = videoEncoder->getCodecInfos();
        filterDescription += "scale=" + std::to_string(inWidth) + ":" + std::to_string(inHeight);
        if (!videoFilter->open(filterDescription, inCodecInfos, outCodecInfos)) {
            error = 5;
            break;
        }

        if (!mediaMuxer.start()) {
            error = 6;
            break;
        }

        ImageFrame* imageFrame = new ImageFrame();
        assert(inDataSize == inStride * inHeight);
        assert(inStride == inWidth * 4);
        imageFrame->setFrame(inData);
        imageFrame->setSize(MSize(inWidth, inHeight));

        Frame* convertFrame = imageFrame->convert(Frame::FRAME_FFMPEG_TYPE);

        delete imageFrame;
        imageFrame = nullptr;

        convertFrame->setTime(0);
        convertFrame->setDuration(0);
        videoFilter->add(convertFrame);

        delete convertFrame;
        convertFrame = nullptr;

        Frame* filterFrame = videoFilter->get();
        if (!filterFrame) {
            error = 7;
            break;
        }
        if (videoEncoder->encode(filterFrame)) {
        }
        delete filterFrame;
        filterFrame = nullptr;

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
            error = 8;
            break;
        }
        mediaMuxer.write(packetCtx);
        delete packetCtx;
        packetCtx = nullptr;

        videoFilter->add(nullptr);

        do {
            if (cancel_) {
                break;
            }

            Frame* filterFrame = videoFilter->get();
            if (!filterFrame) {
                break;
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

        if (cancel_) {
            break;
        }

        error = 0;
    } while (false);

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

    MediaDemuxer mediaDemuxer;
    VideoDecoder* videoDecoder{nullptr};
    DecoderDelegate* videoDecoderDelegate{nullptr};

    do {
        if (error != 0) {
            break;
        }

        const BufferData& bufferData = mediaMuxer.getBufferData();
        if (!mediaDemuxer.open((const char*)(bufferData.buf), bufferData.size - bufferData.room)) {
            error = 9;
            break;
        }
        if (mediaDemuxer.getMediaType() != MEDIA_MOTION_PICTURE_TYPE) {
            error = 10;
            break;
        }

        videoDecoderDelegate = new VideoDecoderDelegateImpl(this);
        videoDecoder = new VideoDecoder(videoDecoderDelegate);
        if (!videoDecoder->open(CODEC_SOFTWARE_TYPE, &mediaDemuxer)) {
            error = 11;
            break;
        }

        Frame* frame = nullptr;
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
                }
            }
            if (!frameCtx) {
                continue;
            }

            frame = frameCtx->getFrame()->copy();

            delete frameCtx;
            frameCtx = nullptr;
        }

        if (!frame) {
            error = 12;
            break;
        }

        Frame* convertFrame = frame->convert(Frame::FRAME_IMAGE_TYPE);
        ImageFrame* imageFrame = dynamic_cast<ImageFrame*>(convertFrame);
        const MSize size = imageFrame->getSize();
        const int width = size.getWidth();
        const int height = size.getHeight();
        const int stride = width * 4;
        const int dataSize = stride * height;
        *outData = (char*)imageFrame->getFrame();
        *outDataSize = dataSize;
        assert(width == inWidth);
        assert(height == inHeight);
        assert(stride == inStride);
        assert(dataSize == inDataSize);

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

    (void)result;
    return error;
}

#if !USE_CUSTOM_COMPONENT
// Custom write function for AVIOContext
static int write_to_memory(void *opaque, uint8_t *buf, int buf_size) {
    (void)opaque;(void)buf;(void)buf_size;
    // Append the encoded data to a buffer in memory
    // Here, we simply print the size of the encoded data
    printf("Encoded data size: %d\n", buf_size);
    return buf_size; // Return the number of bytes written
}
#endif

int VideoUtilInternal::testImage(char *inData, int inDataSize, int inWidth, int inHeight, int inStride)
{
#if USE_CUSTOM_COMPONENT
    bool result = false;
    int error = -1;

    cancel_ = false;

    VideoEncoder* videoEncoder{nullptr};
    EncoderDelegate* videoEncoderDelegate{nullptr};
    MediaMuxer mediaMuxer;

    do {
        if (!inData || inDataSize <= 0 || inWidth <= 0 || inHeight <= 0 || inStride <= 0) {
            error = 1;
            break;
        }

        if (!mediaMuxer.open(FORMAT_PNG_TYPE)) {
            error = 2;
            break;
        }

        if (mediaMuxer.getMediaType() != MEDIA_STILL_PICTURE_TYPE) {
            error = 3;
            break;
        }

        const MSize inSize(inWidth, inHeight);
        const MSize maxSize(inWidth, inHeight);
        MSize outSize;
        scaleSizeByMin(inSize, maxSize, &outSize);

        mediaMuxer.setMuxerType(MediaMuxer::MUXER_FFMPEG_TYPE);

        videoEncoderDelegate = new VideoEncoderDelegateImpl(this);
        videoEncoder = new VideoEncoder(videoEncoderDelegate);
        std::map<CodecKey, CodecValue> codecInfos;
        codecInfos[CODEC_ID_KEY] = AV_CODEC_ID_PNG;
        codecInfos[CODEC_VIDEO_PIXEL_FORMAT_KEY] = AV_PIX_FMT_RGBA;
        codecInfos[CODEC_VIDEO_WIDTH_KEY] = static_cast<int>(outSize.getWidth());
        codecInfos[CODEC_VIDEO_HEIGHT_KEY] = static_cast<int>(outSize.getHeight());
        codecInfos[CODEC_VIDEO_FPS_KEY] = 1.f;
        if (!videoEncoder->open(CODEC_SOFTWARE_TYPE, &mediaMuxer, codecInfos)) {
            error = 4;
            break;
        }

        if (!mediaMuxer.start()) {
            error = 6;
            break;
        }

        PacketContext* packetCtx = nullptr;

        ImageFrame* imageFrame = new ImageFrame();
        assert(inDataSize == inStride * inHeight);
        assert(inStride == inWidth * 4);
        imageFrame->setFrame(inData);
        imageFrame->setSize(MSize(inWidth, inHeight));

        Frame* convertFrame = imageFrame->convert(Frame::FRAME_FFMPEG_TYPE);

        delete imageFrame;
        imageFrame = nullptr;

        videoEncoder->encode(convertFrame);

        delete convertFrame;
        convertFrame = nullptr;

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
            error = 8;
            break;
        }
        mediaMuxer.write(packetCtx);
        delete packetCtx;
        packetCtx = nullptr;

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

        if (cancel_) {
            break;
        }

        error = 0;
        result = true;
    } while (false);

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

    clearBuffer();

#if 0
    const BufferData& bufferData = mediaMuxer.getBufferData();
    *outData = (char*)bufferData.buf;
    *outDataSize = bufferData.size - bufferData.room;
#endif

    (void)result;
    return error;
#else
    AVFormatContext *output_format_context = NULL;
    AVOutputFormat *output_format = NULL;
    AVStream *output_stream = NULL;
    AVCodecContext *codec_context = NULL;
    const AVCodec *codec = NULL;
    AVPacket pkt;
    AVFrame *frame = NULL;
    uint8_t *image_data = NULL;
    int ret;

    // Initialize FFmpeg
    av_register_all();

    av_log_set_level(AV_LOG_DEBUG);

    // Allocate output format context
    ret = avformat_alloc_output_context2(&output_format_context, NULL, "image2pipe", NULL);
    if (ret < 0) {
        fprintf(stderr, "Could not create output context: %s\n", av_err2str(ret));
        return ret;
    }

    // Set up a custom AVIOContext for in-memory output
    size_t output_buffer_size = 2 * 1024 * 1024;
    uint8_t *output_buffer = (uint8_t *)av_malloc(output_buffer_size);
    if (!output_buffer) {
        return -1;
    }
    AVIOContext *avio_context = avio_alloc_context(
        output_buffer, output_buffer_size, 1, NULL, NULL, write_to_memory, NULL);
    if (!avio_context) {
        fprintf(stderr, "Could not allocate AVIOContext\n");
        return AVERROR(ENOMEM);
    }
    output_format_context->pb = avio_context;

    // Create a video stream
    output_stream = avformat_new_stream(output_format_context, NULL);
    if (!output_stream) {
        fprintf(stderr, "Could not create output stream\n");
        return AVERROR_UNKNOWN;
    }

    // Find the PNG encoder
    codec = avcodec_find_encoder(AV_CODEC_ID_PNG);
    if (!codec) {
        fprintf(stderr, "PNG codec not found\n");
        return AVERROR_UNKNOWN;
    }

    // Allocate and configure the codec context
    codec_context = avcodec_alloc_context3(codec);
    if (!codec_context) {
        fprintf(stderr, "Could not allocate codec context\n");
        return AVERROR(ENOMEM);
    }

    // Set codec parameters
    codec_context->width = 640;  // Image width
    codec_context->height = 480; // Image height
    codec_context->pix_fmt = AV_PIX_FMT_RGBA; // Pixel format
    codec_context->time_base = (AVRational){1, 25}; // Time base (not critical for single imag

    // Open the codec
    ret = avcodec_open2(codec_context, codec, NULL);
    if (ret < 0) {
        fprintf(stderr, "Could not open codec: %s\n", av_err2str(ret));
        return ret;
    }

    // Set the codec parameters for the output stream
    avcodec_parameters_from_context(output_stream->codecpar, codec_context);

    // Write the header
    ret = avformat_write_header(output_format_context, NULL);
    if (ret < 0) {
        fprintf(stderr, "Error writing header: %s\n", av_err2str(ret));
        return ret;
    }

    // Allocate a frame for the raw image data
    frame = av_frame_alloc();
    if (!frame) {
        fprintf(stderr, "Could not allocate frame\n");
        return AVERROR(ENOMEM);
    }

    frame->format = AV_PIX_FMT_RGBA;
    frame->width = 640;
    frame->height = 480;
    frame->pts = 0;

    // Allocate memory for the raw image data
    ret = av_frame_get_buffer(frame, 32);
    if (ret < 0) {
        fprintf(stderr, "Could not allocate frame buffer: %s\n", av_err2str(ret));
        return ret;
    }

    // Fill the frame with dummy RGBA data (replace with your actual image data)
    for (int y = 0; y < 480; y++) {
        for (int x = 0; x < 640; x++) {
            frame->data[0][y * frame->linesize[0] + x * 4 + 0] = x % 255; // R
            frame->data[0][y * frame->linesize[0] + x * 4 + 1] = y % 255; // G
            frame->data[0][y * frame->linesize[0] + x * 4 + 2] = (x + y) % 255; // B
            frame->data[0][y * frame->linesize[0] + x * 4 + 3] = 255; // A
        }
    }

    // Encode the frame
    av_init_packet(&pkt);
    pkt.data = NULL;
    pkt.size = 0;

    ret = avcodec_send_frame(codec_context, frame);
    if (ret < 0) {
        fprintf(stderr, "Error sending frame to encoder: %s\n", av_err2str(ret));
        return ret;
    }

    ret = avcodec_receive_packet(codec_context, &pkt);
    if (ret < 0) {
        fprintf(stderr, "Error receiving packet from encoder: %s\n", av_err2str(ret));
        return ret;
    }

    // Write the encoded frame to memory
    ret = av_write_frame(output_format_context, &pkt);
    if (ret < 0) {
        fprintf(stderr, "Error writing frame: %s\n", av_err2str(ret));
        return ret;
    }

    // Write the trailer
    av_write_trailer(output_format_context);

    // Clean up
    av_frame_free(&frame);
    av_packet_unref(&pkt);
    avio_context_free(&output_format_context->pb);
    avformat_free_context(output_format_context);

    return 0;
#endif
}

std::unordered_map<int64_t, int64_t> VideoUtilInternal::shotDetection(const string &inFilePath, const MediaUtilDelegate *delegate)
{
    std::unordered_map<int64_t, int64_t> shotTimes;

    cancel_ = false;

    MediaDemuxer mediaDemuxer;
    VideoDecoder* videoDecoder{nullptr};
    DecoderDelegate* videoDecoderDelegate{nullptr};
    VideoFilter* videoFilter{nullptr};

    do {
        if (!File::exist(inFilePath)) {
            break;
        }

        if (!mediaDemuxer.open(inFilePath)) {
            break;
        }

        if (!(mediaDemuxer.getMediaType() == MEDIA_VIDEO_AUDIO_TYPE)) {
            break;
        }

        const int64_t totalTime = mediaDemuxer.getDuration();

        const MSize inSize = mediaDemuxer.getSize();
        const MSize maxSize(inSize);
        MSize outSize;
        scaleSizeByMin(inSize, maxSize, &outSize);

        videoDecoderDelegate = new VideoDecoderDelegateImpl(this);
        videoDecoder = new VideoDecoder(videoDecoderDelegate);
        if (!videoDecoder->open(CODEC_SOFTWARE_TYPE, &mediaDemuxer)) {
            break;
        }

        videoFilter = new VideoFilter;
        string filterDescription;
        const std::map<CodecKey, CodecValue>& inCodecInfos = videoDecoder->getCodecInfos();
        const std::map<CodecKey, CodecValue>& outCodecInfos = inCodecInfos;
        filterDescription += "scale=" + std::to_string(static_cast<int>(outSize.getWidth())) + ":" + std::to_string(static_cast<int>(outSize.getHeight()));
        if (!videoFilter->open(filterDescription, inCodecInfos, outCodecInfos)) {
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
                if (!videoDecoderBuffer_.empty()) {
                    frameCtx = videoDecoderBuffer_.front();
                    videoDecoderBuffer_.pop();
                }
            }
            if (!frameCtx) {
                continue;
            }
            Frame* frame = frameCtx->getFrame();
            videoFilter->add(frame);
            delete frameCtx;
            frameCtx = nullptr;

            Frame* filterFrame = videoFilter->get();
            if (!filterFrame) {
                continue;
            }
            if (delegate) {
                delegate->notifyExportProgress(filterFrame->getTime(), totalTime);
            }
            delete filterFrame;
            filterFrame = nullptr;
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
            delete filterFrame;
            filterFrame = nullptr;
        } while (true);

        if (cancel_) {
            break;
        }

        if (delegate) {
            delegate->notifyExportProgress(totalTime, totalTime);
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

    mediaDemuxer.close();

    if (videoFilter) {
        videoFilter->close();

        delete videoFilter;
        videoFilter = nullptr;
    }

    clearBuffer();

    if (delegate) {
        delegate->notifyExportFinish();
    }

    return shotTimes;
}

int VideoUtilInternal::getInfo(char *data, int dataSize, int *width, int *height, int64_t *duration, float *fps)
{
    int error = 0;

    MediaDemuxer mediaDemuxer;

    do {
        if (!mediaDemuxer.open(data, dataSize)) {
            error = 1;
            break;
        }

        const MSize size = mediaDemuxer.getSize();
        *width = size.getWidth();
        *height = size.getHeight();
        *duration = mediaDemuxer.getDuration();
        *fps = mediaDemuxer.getFPS();
    } while (false);

    mediaDemuxer.close();

    return error;
}

int VideoUtilInternal::edit(char *inData, int inDataSize, char **outData, int *outDataSize, char *formatName, const int64_t startTime, const int64_t endTime, const MediaUtilDelegate *delegate)
{
    bool result = false;
    int error = 0;

    cancel_ = false;

    MediaDemuxer mediaDemuxer;
    VideoDecoder* videoDecoder{nullptr};
    DecoderDelegate* videoDecoderDelegate{nullptr};
    AudioDecoder* audioDecoder{nullptr};
    DecoderDelegate* audioDecoderDelegate{nullptr};
    VideoEncoder* videoEncoder{nullptr};
    EncoderDelegate* videoEncoderDelegate{nullptr};
    AudioEncoder* audioEncoder{nullptr};
    EncoderDelegate* audioEncoderDelegate{nullptr};
    MediaMuxer mediaMuxer;

    do {
        if (!mediaDemuxer.open(inData, inDataSize)) {
            error = 1;
            break;
        }
        FormatType formatType = FORMAT_NONE_TYPE;
        if (strcmp(formatName, "mp4") == 0) {
            formatType = FORMAT_MP4_TYPE;
        } else {
            break;
        }
        if (!mediaMuxer.open(formatType)) {
            error = 2;
            break;
        }

        if (mediaDemuxer.getMediaType() != mediaMuxer.getMediaType()) {
            error = 3;
            break;
        }

        const int64_t duration = mediaDemuxer.getDuration();
        const int64_t curStartTime = startTime == -1 ? 0 : startTime;
        const int64_t curEndTime = endTime == -1 ? duration : endTime;
        const int64_t totalTime = curEndTime - curStartTime;
        if (totalTime > duration) {
            error = 4;
            break;
        }

        const MSize inSize = mediaDemuxer.getSize();
        MSize outSize = inSize;

        const float fps = mediaDemuxer.getFPS();

        switch (mediaDemuxer.getMediaType()) {
        case MEDIA_STILL_PICTURE_TYPE: {
        }
        break;
        case MEDIA_MOTION_PICTURE_TYPE: {
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
                    error = 5;
                    break;
                }

                videoEncoderDelegate = new VideoEncoderDelegateImpl(this);
                videoEncoder = new VideoEncoder(videoEncoderDelegate);
                std::map<CodecKey, CodecValue> codecInfos;
                codecInfos[CODEC_ID_KEY] = AV_CODEC_ID_H264;
                codecInfos[CODEC_VIDEO_PIXEL_FORMAT_KEY] = AV_PIX_FMT_YUV420P;
                codecInfos[CODEC_VIDEO_WIDTH_KEY] = static_cast<int>(outSize.getWidth());
                codecInfos[CODEC_VIDEO_HEIGHT_KEY] = static_cast<int>(outSize.getHeight());
                codecInfos[CODEC_VIDEO_FPS_KEY] = fps;
                if (!videoEncoder->open(CODEC_SOFTWARE_TYPE, &mediaMuxer, codecInfos)) {
                    error = 6;
                    break;
                }
            }

            if (haveAudio) {
                audioDecoderDelegate = new AudioDecoderDelegateImpl(this);
                audioDecoder = new AudioDecoder(audioDecoderDelegate);
                if (!audioDecoder->open(CODEC_SOFTWARE_TYPE, &mediaDemuxer)) {
                    error = 7;
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
                if (!audioEncoder->open(CODEC_SOFTWARE_TYPE, &mediaMuxer, codecInfos)) {
                    error = 8;
                    break;
                }
            }

            if (!mediaMuxer.start()) {
                error = 9;
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
                        if (!videoDecoderBuffer_.empty()) {
                            frameCtx = videoDecoderBuffer_.front();
                            videoDecoderBuffer_.pop();
                        }
                    }
                    if (!frameCtx) {
                        continue;
                    }

                    const int64_t frameTime = frameCtx->getTime();
                    if (frameTime < curStartTime || frameTime > curEndTime) {
                        break;
                    }

                    Frame* frame = frameCtx->getFrame();
                    Frame* copyFrame = frame->copy();
                    assert(copyFrame);
                    const int64_t newFrameTime = frameTime - curStartTime;
                    copyFrame->setTime(newFrameTime);
                    videoEncoder->encode(copyFrame);
                    delete copyFrame;
                    copyFrame = nullptr;

                    delete frameCtx;
                    frameCtx = nullptr;

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
                        if (!audioDecoderBuffer_.empty()) {
                            frameCtx = audioDecoderBuffer_.front();
                            audioDecoderBuffer_.pop();
                        }
                    }
                    if (!frameCtx) {
                        continue;
                    }

                    const int64_t frameTime = frameCtx->getTime();
                    if (frameTime < curStartTime || frameTime > curEndTime) {
                        break;
                    }

                    Frame* frame = frameCtx->getFrame();
                    Frame* copyFrame = frame->copy();
                    assert(copyFrame);
                    const int64_t newFrameTime = frameTime - curStartTime;
                    copyFrame->setTime(newFrameTime);
                    audioEncoder->encode(copyFrame);
                    delete copyFrame;
                    copyFrame = nullptr;

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

    clearBuffer();

    if (delegate) {
        delegate->notifyExportFinish();
    }

    return result;
}

}
