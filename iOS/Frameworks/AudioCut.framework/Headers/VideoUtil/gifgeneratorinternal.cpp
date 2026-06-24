#include "gifgeneratorinternal.h"

#include "Media/mediademuxer.h"
#include "Media/mediamuxer.h"
#include "Codec/Decoder/videodecoder.h"
#include "Codec/Encoder/videoencoder.h"
#include "Filter/VideoFilter/videofilter.h"
#include "Model/packetcontext.h"
#include "Model/framecontext.h"
#include "Model/frame.h"
#include "Model/ffmpegframe.h"
#include "Model/imageframe.h"

#include <cassert>

#ifndef OPTIMIZE_ADD_IMAGE_DATA_PERFORMANCE
#define OPTIMIZE_ADD_IMAGE_DATA_PERFORMANCE 0
#endif

extern "C" {
#include <libswscale/swscale.h>
}

namespace MediaLibrary {

GIFGeneratorInternal::GIFGeneratorInternal()
{
}

bool GIFGeneratorInternal::open(const FillMode &fillMode, const VideoFormat &videoFormat, const MSize &mediaSize, const bool minMode, const int loop, const GIFMode &gifMode)
{
    fillMode_ = fillMode;
    videoFormat_ = videoFormat;
    mediaSize_ = mediaSize;
    gifMode_ = gifMode;

    mediaMuxer_ = new MediaMuxer();
    if (!mediaMuxer_->open(FORMAT_GIF_TYPE)) {
        return false;
    }
    if (mediaMuxer_->getMediaType() != MEDIA_MOTION_PICTURE_TYPE) {
        return false;
    }
    mediaMuxer_->setMuxerType(MediaMuxer::MUXER_FFMPEG_TYPE);

    const float fps = videoFormat.fps;
    assert(fps > 0.f);

    frameDuration_ = static_cast<int64_t>(SECOND_TO_MILLISECOND_UNIT / fps);

    const MSize size(videoFormat.width == -1 ? mediaSize.getWidth() : videoFormat.width, videoFormat.height == -1 ? mediaSize.getHeight() : videoFormat.height);
    if (minMode) {
        scaleSizeByMin(mediaSize, std::min(size.getWidth(), size.getHeight()), &outSize_);
    } else {
        scaleSizeByMax(mediaSize, std::min(size.getWidth(), size.getHeight()), &outSize_);
    }

    videoEncoderDelegate_ = new VideoEncoderDelegateImpl(this);
    videoEncoder_ = new VideoEncoder(videoEncoderDelegate_);
    std::map<CodecKey, CodecValue> codecInfos;
    codecInfos[CODEC_ID_KEY] = AV_CODEC_ID_GIF;
    codecInfos[CODEC_VIDEO_PIXEL_FORMAT_KEY] = AV_PIX_FMT_PAL8;
    codecInfos[CODEC_VIDEO_WIDTH_KEY] = static_cast<int>(outSize_.getWidth());
    codecInfos[CODEC_VIDEO_HEIGHT_KEY] = static_cast<int>(outSize_.getHeight());
    codecInfos[CODEC_VIDEO_FPS_KEY] = fps;
    if (!videoEncoder_->open(CODEC_SOFTWARE_TYPE, mediaMuxer_, codecInfos)) {
        return false;
    }

    videoFilter_ = new VideoFilter;
    string filterDescription;
    pixelFormat_ = AV_PIX_FMT_NONE;
    switch (gifMode_) {
    case GIF_FAST_MODE: {
        filterDescription += "null";
#if !OPTIMIZE_ADD_IMAGE_DATA_PERFORMANCE
        pixelFormat_ = AV_PIX_FMT_PAL8;
#else
        pixelFormat_ = AV_PIX_FMT_RGBA;
#endif
    }
        break;
    case GIF_QUALITY_MODE: {
        filterDescription += "split [o1] [o2];[o1] palettegen [p]; [o2] fifo [o3];[o3] [p] paletteuse";
        pixelFormat_ = AV_PIX_FMT_RGBA;
    }
        break;
    default:
        break;
    }
    std::map<CodecKey, CodecValue> inCodecInfos;
    inCodecInfos[CODEC_VIDEO_WIDTH_KEY] = static_cast<int>(outSize_.getWidth());
    inCodecInfos[CODEC_VIDEO_HEIGHT_KEY] = static_cast<int>(outSize_.getHeight());
    inCodecInfos[CODEC_VIDEO_PIXEL_FORMAT_KEY] = pixelFormat_;
    const std::map<CodecKey, CodecValue>& outCodecInfos = videoEncoder_->getCodecInfos();
    if (!videoFilter_->open(filterDescription, inCodecInfos, outCodecInfos)) {
        return false;
    }

    if (!mediaMuxer_->start(loop)) {
        return false;
    }

    return true;
}

void GIFGeneratorInternal::close()
{
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

    if (videoFilter_) {
        videoFilter_->close();

        delete videoFilter_;
        videoFilter_ = nullptr;
    }
}

int GIFGeneratorInternal::addImage(char *data, int dataSize)
{
    int error = 0;

    if (data && dataSize > 0) {
        MediaDemuxer mediaDemuxer;
        VideoDecoder* videoDecoder{nullptr};
        DecoderDelegate* videoDecoderDelegate{nullptr};
        VideoFilter* scaleVideoFilter{nullptr};

        do {
            if (!mediaDemuxer.open(data, dataSize)) {
                error = 10;
                break;
            }
            if (mediaDemuxer.getMediaType() != MEDIA_STILL_PICTURE_TYPE) {
                error = 11;
                break;
            }

            const MSize inSize = mediaDemuxer.getSize();
            MSize outSize;
            scaleSizeByMin(inSize, outSize_, &outSize);

            videoDecoderDelegate = new VideoDecoderDelegateImpl(this);
            videoDecoder = new VideoDecoder(videoDecoderDelegate);
            if (!videoDecoder->open(CODEC_SOFTWARE_TYPE, &mediaDemuxer)) {
                error = 12;
                break;
            }

            scaleVideoFilter = new VideoFilter;
            string filterDescription;
            const std::map<CodecKey, CodecValue>& inCodecInfos = videoDecoder->getCodecInfos();
            std::map<CodecKey, CodecValue> outCodecInfos;
            outCodecInfos[CODEC_VIDEO_WIDTH_KEY] = static_cast<int>(outSize.getWidth());
            outCodecInfos[CODEC_VIDEO_HEIGHT_KEY] = static_cast<int>(outSize.getHeight());
            outCodecInfos[CODEC_VIDEO_PIXEL_FORMAT_KEY] = pixelFormat_;
            const int outWidth = static_cast<int>(outSize.getWidth());
            const int outHeight = static_cast<int>(outSize.getHeight());
            switch (fillMode_) {
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
                error = 13;
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
                error = 14;
                break;
            }

            const int64_t frameTime = requestTime_;
            frame->setTime(frameTime);
            frame->setDuration(frameDuration_);
            scaleVideoFilter->add(frame);

            requestTime_ += frameDuration_;

            Frame* scaleFilterFrame = scaleVideoFilter->get();
            if (!scaleFilterFrame) {
                error = 15;
                break;
            }
            switch (fillMode_) {
            case FILL_FIT_MODE: {
                Frame* frame = FFmpegFrame::create(outSize_, pixelFormat_);
                assert(frame);
                frame->copy(scaleFilterFrame);
                frame->setTime(scaleFilterFrame->getTime());
                frame->setDuration(scaleFilterFrame->getDuration());
                videoFilter_->add(frame);
                delete frame;
                frame = nullptr;
            }
                break;
            case FILL_FILL_MODE: {
                Frame* frame = FFmpegFrame::create(outSize_, pixelFormat_);
                assert(frame);
                frame->copy(scaleFilterFrame);
                frame->setTime(scaleFilterFrame->getTime());
                frame->setDuration(scaleFilterFrame->getDuration());
                videoFilter_->add(frame);
                delete frame;
                frame = nullptr;
            }
                break;
            case FILL_STRETCH_MODE: {
                videoFilter_->add(scaleFilterFrame);
            }
                break;
            default:
                break;
            }
            delete frame;
            frame = nullptr;
            delete scaleFilterFrame;
            scaleFilterFrame = nullptr;

            Frame* filterFrame = videoFilter_->get();
            if (!filterFrame) {
                error = 16;
                break;
            }
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
                error = 17;
                break;
            }
            mediaMuxer_->write(packetCtx);
            delete packetCtx;
            packetCtx = nullptr;
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

        if (scaleVideoFilter) {
            scaleVideoFilter->close();

            delete scaleVideoFilter;
            scaleVideoFilter = nullptr;
        }
    } else {
        videoFilter_->add(nullptr);

        do {
            Frame* filterFrame = videoFilter_->get();
            if (!filterFrame) {
                break;
            }
            if (videoEncoder_->encode(filterFrame)) {
            }
            delete filterFrame;
            filterFrame = nullptr;
        } while (true);

        videoEncoder_->encode(nullptr);

        do {
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
    }

    return error;
}

int GIFGeneratorInternal::addImageData(char *data, int dataSize, int width, int height, int stride)
{
    int error = 0;

    if (data && dataSize > 0) {
#if !OPTIMIZE_ADD_IMAGE_DATA_PERFORMANCE
        VideoFilter* scaleVideoFilter{nullptr};
#endif

        do {
#if !OPTIMIZE_ADD_IMAGE_DATA_PERFORMANCE
            const MSize inSize(width, height);
            MSize outSize;
            scaleSizeByMin(inSize, outSize_, &outSize);

            scaleVideoFilter = new VideoFilter;
            string filterDescription;
            std::map<CodecKey, CodecValue> inCodecInfos;
            inCodecInfos[CODEC_VIDEO_WIDTH_KEY] = static_cast<int>(inSize.getWidth());
            inCodecInfos[CODEC_VIDEO_HEIGHT_KEY] = static_cast<int>(inSize.getHeight());
            inCodecInfos[CODEC_VIDEO_PIXEL_FORMAT_KEY] = AV_PIX_FMT_RGBA;
            std::map<CodecKey, CodecValue> outCodecInfos;
            outCodecInfos[CODEC_VIDEO_WIDTH_KEY] = static_cast<int>(outSize.getWidth());
            outCodecInfos[CODEC_VIDEO_HEIGHT_KEY] = static_cast<int>(outSize.getHeight());
            outCodecInfos[CODEC_VIDEO_PIXEL_FORMAT_KEY] = pixelFormat_;
            const int outWidth = static_cast<int>(outSize.getWidth());
            const int outHeight = static_cast<int>(outSize.getHeight());
            switch (fillMode_) {
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
                error = 10;
                break;
            }

            ImageFrame* imageFrame = new ImageFrame();
            assert(dataSize == stride * height);
            assert(stride == width * 4);
            imageFrame->setFrame(data);
            imageFrame->setSize(MSize(width, height));

            Frame* convertFrame = imageFrame->convert(Frame::FRAME_FFMPEG_TYPE);

            delete imageFrame;
            imageFrame = nullptr;

            const int64_t frameTime = requestTime_;
            convertFrame->setTime(frameTime);
            convertFrame->setDuration(frameDuration_);
            scaleVideoFilter->add(convertFrame);

            requestTime_ += frameDuration_;

            delete convertFrame;
            convertFrame = nullptr;

            Frame* scaleFilterFrame = scaleVideoFilter->get();
            if (!scaleFilterFrame) {
                error = 11;
                break;
            }
            switch (fillMode_) {
            case FILL_FIT_MODE: {
                Frame* frame = FFmpegFrame::create(outSize, pixelFormat_);
                assert(frame);
                frame->copy(scaleFilterFrame);
                frame->setTime(scaleFilterFrame->getTime());
                frame->setDuration(scaleFilterFrame->getDuration());
                videoFilter_->add(frame);
                delete frame;
                frame = nullptr;
            }
                break;
            case FILL_FILL_MODE: {
                Frame* frame = FFmpegFrame::create(outSize, pixelFormat_);
                assert(frame);
                frame->copy(scaleFilterFrame);
                frame->setTime(scaleFilterFrame->getTime());
                frame->setDuration(scaleFilterFrame->getDuration());
                videoFilter_->add(frame);
                delete frame;
                frame = nullptr;
            }
                break;
            case FILL_STRETCH_MODE: {
                videoFilter_->add(scaleFilterFrame);
            }
                break;
            default:
                break;
            }
            delete scaleFilterFrame;
            scaleFilterFrame = nullptr;
#else
            auto startTime = std::chrono::steady_clock::now();

            ImageFrame* imageFrame = new ImageFrame();
            assert(dataSize == stride * height);
            assert(stride == width * 4);
            imageFrame->setFrame(data);
            imageFrame->setSize(MSize(width, height));

            Frame* convertFrame = imageFrame->convert(Frame::FRAME_FFMPEG_TYPE);

            delete imageFrame;
            imageFrame = nullptr;

            const int64_t frameTime = requestTime_;
            convertFrame->setTime(frameTime);
            convertFrame->setDuration(frameDuration_);
            videoFilter_->add(convertFrame);

            requestTime_ += frameDuration_;

            delete convertFrame;
            convertFrame = nullptr;
#endif

            Frame* filterFrame = videoFilter_->get();
            if (!filterFrame) {
                error = 12;
                break;
            }
            if (videoEncoder_->encode(filterFrame)) {
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
                error = 13;
                break;
            }
            mediaMuxer_->write(packetCtx);
            delete packetCtx;
            packetCtx = nullptr;
        } while (false);

#if !OPTIMIZE_ADD_IMAGE_DATA_PERFORMANCE
        if (scaleVideoFilter) {
            scaleVideoFilter->close();

            delete scaleVideoFilter;
            scaleVideoFilter = nullptr;
        }
#endif
    } else {
        videoFilter_->add(nullptr);

        do {
            Frame* filterFrame = videoFilter_->get();
            if (!filterFrame) {
                break;
            }
            if (videoEncoder_->encode(filterFrame)) {
            }
            delete filterFrame;
            filterFrame = nullptr;
        } while (true);

        videoEncoder_->encode(nullptr);

        do {
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
    }

    return error;
}

int GIFGeneratorInternal::addEmptyImageData(bool *end, int *progress)
{
    int error = 0;

    bool endInternal = false;
    int progressInternal = 0;
    const std::chrono::steady_clock::time_point beginTime = std::chrono::steady_clock::now();
    bool timeMeet = false;

    do {
        if (!videoFilterFinish_) {
            videoFilter_->add(nullptr);
            videoFilterFinish_ = true;
        }

        do {
            const std::chrono::steady_clock::time_point endTime = std::chrono::steady_clock::now();
            std::chrono::duration<double, std::milli> time = endTime - beginTime;
            if (time.count() > 100) {
                timeMeet = true;
                break;
            }

            Frame* filterFrame = videoFilter_->get();
            if (!filterFrame) {
                progressInternal = 80;
                break;
            }
            if (videoEncoder_->encode(filterFrame)) {
            }
            progressInternal = std::max(std::min((int)((filterFrame->getTime() * 1.f / requestTime_) * 80), 80), 0);
            delete filterFrame;
            filterFrame = nullptr;
        } while (true);

        if (timeMeet) {
            break;
        }

        if (!videoEncoderFinish_) {
            videoEncoder_->encode(nullptr);
            videoEncoderFinish_ = true;
        }

        do {
            const std::chrono::steady_clock::time_point endTime = std::chrono::steady_clock::now();
            std::chrono::duration<double, std::milli> time = endTime - beginTime;
            if (time.count() > 100) {
                timeMeet = true;
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
                endInternal = true;
                progressInternal = 100;
                break;
            }
            progressInternal += std::max(std::min((int)((packetCtx->getTime() * 1.f / requestTime_) * 20), 20), 0);
            mediaMuxer_->write(packetCtx);
            delete packetCtx;
            packetCtx = nullptr;
        } while (true);

        if (timeMeet) {
            break;
        }
    } while (false);

    *end = endInternal;
    *progress = progressInternal;

    return error;
}

void GIFGeneratorInternal::getGIFData(char **data, int *dataSize)
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

void GIFGeneratorInternal::getGIFData(char **data, int *dataSize, bool *end, int *progress, int readDataSize)
{
    if (mediaMuxer_) {
        if (readDataSize == 0) {
            mediaMuxer_->finish();
        }

        const BufferData& bufferData = mediaMuxer_->getBufferData();
        *data = (char*)bufferData.buf + readDataSize;
        *dataSize = std::min((int)(bufferData.size - bufferData.room - readDataSize), 1 * 1024 * 1024);
        *end = readDataSize >= (int)(bufferData.size - bufferData.room);
        *progress = *end ? 100 : std::max(std::min((int)((readDataSize * 1.f / (int)(bufferData.size - bufferData.room)) * 100), 100), 0);
    } else {
        *data = nullptr;
        *dataSize = 0;
    }
}

void GIFGeneratorInternal::decodeVideoFrame(FrameContext *frameCtx)
{
    std::lock_guard<std::mutex> guard(videoDecoderBufferMutex_);
    (void)guard;
    videoDecoderBuffer_.push(frameCtx);
}

void GIFGeneratorInternal::encodeVideoPacket(PacketContext *packetCtx)
{
    std::lock_guard<std::mutex> guard(videoEncoderBufferMutex_);
    (void)guard;
    videoEncoderBuffer_.push(packetCtx);
}

GIFGeneratorInternal::VideoDecoderDelegateImpl::VideoDecoderDelegateImpl(GIFGeneratorInternal *internal)
    : internal_(internal)
{
}

GIFGeneratorInternal::VideoDecoderDelegateImpl::~VideoDecoderDelegateImpl()
{
    internal_ = nullptr;
}

void GIFGeneratorInternal::VideoDecoderDelegateImpl::output(FrameContext *frameCtx)
{
    if (internal_) {
        internal_->decodeVideoFrame(frameCtx);
    }
}

GIFGeneratorInternal::VideoEncoderDelegateImpl::VideoEncoderDelegateImpl(GIFGeneratorInternal *internal)
    : internal_(internal)
{
}

GIFGeneratorInternal::VideoEncoderDelegateImpl::~VideoEncoderDelegateImpl()
{
    internal_ = nullptr;
}

void GIFGeneratorInternal::VideoEncoderDelegateImpl::output(PacketContext *packetCtx)
{
    if (internal_) {
        internal_->encodeVideoPacket(packetCtx);
    }
}

}
