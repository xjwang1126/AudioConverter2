#include "imagegeneratorinternal.h"

#include "Common/Model/framecontext.h"
#include "Common/Model/packetcontext.h"

#include <cassert>
#include <iostream>

namespace MediaLibrary {

ImageGeneratorInternal::ImageGeneratorInternal()
{
}

void ImageGeneratorInternal::setSize(const MSize &size, const bool minMode)
{
    if (size_ == size) {
        return;
    }
    size_ = size;
    minMode_ = minMode;

    if (!videoFilter_ || !videoDecoder_) {
        return;
    }

    videoFilter_->close();

    const MSize mediaSize = mediaDemuxer_.getSize();
    const MSize formatSize = size_ == MSize() ? mediaSize : size_;
    MSize outSize;
    if (minMode_) {
        scaleSizeByMin(mediaSize, std::min(formatSize.getWidth(), formatSize.getHeight()), &outSize);
    } else {
        scaleSizeByMax(mediaSize, std::min(formatSize.getWidth(), formatSize.getHeight()), &outSize);
    }

    string filterDescription;
    const std::map<CodecKey, CodecValue>& inCodecInfos = videoDecoder_->getCodecInfos();
    std::map<CodecKey, CodecValue> outCodecInfos = inCodecInfos;
    outCodecInfos[CODEC_VIDEO_PIXEL_FORMAT_KEY] = AV_PIX_FMT_RGBA;
    outCodecInfos[CODEC_VIDEO_WIDTH_KEY] = static_cast<int>(outSize.getWidth());
    outCodecInfos[CODEC_VIDEO_HEIGHT_KEY] = static_cast<int>(outSize.getHeight());
    filterDescription += "scale=" + std::to_string(static_cast<int>(outSize.getWidth())) + ":" + std::to_string(static_cast<int>(outSize.getHeight()));
    if (!videoFilter_->open(filterDescription, inCodecInfos, outCodecInfos)) {
        assert(0);
    }
}

int ImageGeneratorInternal::open(const std::string &filePath)
{
    int error = 0;

    do {
        if (!mediaDemuxer_.open(filePath)) {
            error = 1;
            break;
        }

        const MSize inSize = mediaDemuxer_.getSize();
        const MSize maxSize = size_ == MSize() ? inSize : size_;
        MSize outSize;
        scaleSizeByMin(inSize, maxSize, &outSize);

        videoDecoderDelegate_ = new VideoDecoderDelegateImpl(this);
        videoDecoder_ = new VideoDecoder(videoDecoderDelegate_);
        if (!videoDecoder_->open(CODEC_SOFTWARE_TYPE, &mediaDemuxer_)) {
            error = 2;
            break;
        }

        videoFilter_ = new VideoFilter();
        string filterDescription;
        const std::map<CodecKey, CodecValue>& inCodecInfos = videoDecoder_->getCodecInfos();
        std::map<CodecKey, CodecValue> outCodecInfos = inCodecInfos;
        outCodecInfos[CODEC_VIDEO_PIXEL_FORMAT_KEY] = AV_PIX_FMT_RGBA;
        outCodecInfos[CODEC_VIDEO_WIDTH_KEY] = static_cast<int>(outSize.getWidth());
        outCodecInfos[CODEC_VIDEO_HEIGHT_KEY] = static_cast<int>(outSize.getHeight());
        filterDescription += "scale=" + std::to_string(static_cast<int>(outSize.getWidth())) + ":" + std::to_string(static_cast<int>(outSize.getHeight()));
        if (!videoFilter_->open(filterDescription, inCodecInfos, outCodecInfos)) {
            error = 3;
            break;
        }

        seek(0);
    } while (false);

    return error;
}

int ImageGeneratorInternal::open(char *data, int dataSize)
{
    int error = 0;

    do {
        if (!mediaDemuxer_.open(data, dataSize)) {
            error = 1;
            break;
        }

        const MSize inSize = mediaDemuxer_.getSize();
        const MSize maxSize = size_ == MSize() ? inSize : size_;
        MSize outSize;
        scaleSizeByMin(inSize, maxSize, &outSize);

        videoDecoderDelegate_ = new VideoDecoderDelegateImpl(this);
        videoDecoder_ = new VideoDecoder(videoDecoderDelegate_);
        if (!videoDecoder_->open(CODEC_SOFTWARE_TYPE, &mediaDemuxer_)) {
            error = 2;
            break;
        }

        videoFilter_ = new VideoFilter();
        string filterDescription;
        const std::map<CodecKey, CodecValue>& inCodecInfos = videoDecoder_->getCodecInfos();
        std::map<CodecKey, CodecValue> outCodecInfos = inCodecInfos;
        outCodecInfos[CODEC_VIDEO_PIXEL_FORMAT_KEY] = AV_PIX_FMT_RGBA;
        outCodecInfos[CODEC_VIDEO_WIDTH_KEY] = static_cast<int>(outSize.getWidth());
        outCodecInfos[CODEC_VIDEO_HEIGHT_KEY] = static_cast<int>(outSize.getHeight());
        filterDescription += "scale=" + std::to_string(static_cast<int>(outSize.getWidth())) + ":" + std::to_string(static_cast<int>(outSize.getHeight()));
        if (!videoFilter_->open(filterDescription, inCodecInfos, outCodecInfos)) {
            error = 3;
            break;
        }

        seek(0);
    } while (false);

    return error;
}

void ImageGeneratorInternal::close()
{
    if (videoDecoder_) {
        videoDecoder_->close();

        delete videoDecoder_;
        videoDecoder_ = nullptr;
    }

    if (videoDecoderDelegate_) {
        delete videoDecoderDelegate_;
        videoDecoderDelegate_ = nullptr;
    }

    mediaDemuxer_.close();

    if (videoFilter_) {
        videoFilter_->close();

        delete videoFilter_;
        videoFilter_ = nullptr;
    }

    clearBuffer();
}

void ImageGeneratorInternal::seek(const int64_t time)
{
    demuxerEnd_ = false;
    decoderEnd_ = false;
    filterEnd_ = false;

    mediaDemuxer_.seek(time);

    videoDecoder_->flush();

    clearBuffer();

    videoFilter_->close();

    const MSize mediaSize = mediaDemuxer_.getSize();
    const MSize formatSize = size_ == MSize() ? mediaSize : size_;
    MSize outSize;
    if (minMode_) {
        scaleSizeByMin(mediaSize, std::min(formatSize.getWidth(), formatSize.getHeight()), &outSize);
    } else {
        scaleSizeByMax(mediaSize, std::min(formatSize.getWidth(), formatSize.getHeight()), &outSize);
    }

    string filterDescription;
    const std::map<CodecKey, CodecValue>& inCodecInfos = videoDecoder_->getCodecInfos();
    std::map<CodecKey, CodecValue> outCodecInfos = inCodecInfos;
    outCodecInfos[CODEC_VIDEO_PIXEL_FORMAT_KEY] = AV_PIX_FMT_RGBA;
    outCodecInfos[CODEC_VIDEO_WIDTH_KEY] = static_cast<int>(outSize.getWidth());
    outCodecInfos[CODEC_VIDEO_HEIGHT_KEY] = static_cast<int>(outSize.getHeight());
    filterDescription += "scale=" + std::to_string(static_cast<int>(outSize.getWidth())) + ":" + std::to_string(static_cast<int>(outSize.getHeight()));
    if (!videoFilter_->open(filterDescription, inCodecInfos, outCodecInfos)) {
        assert(0);
    }
}

int ImageGeneratorInternal::getImageData(char **data, int *dataSize, int *width, int *height, int *stride, int64_t *time, bool* end)
{
    PacketContext* packetCtx = nullptr;
    while (true) {
        if (demuxerEnd_ && decoderEnd_ && filterEnd_) {
            *end = true;
            break;
        } else {
            *end = false;
        }

        while (true) {
            if (demuxerEnd_) {
                break;
            }
            if (!packetCtx) {
                packetCtx = mediaDemuxer_.read();
            }
            if (!packetCtx) {
                demuxerEnd_ = true;
                break;
            }
            if (packetCtx->getPacketType() == PacketContext::PACKET_VIDEO_TYPE) {
                break;
            }
            delete packetCtx;
            packetCtx = nullptr;
        }

        do {
            if (decoderEnd_) {
                break;
            }
            if (demuxerEnd_) {
                videoDecoder_->decode(nullptr);
                decoderEnd_ = true;
                break;
            }
            if (videoDecoder_->decode(packetCtx)) {
                delete packetCtx;
                packetCtx = nullptr;
            }
        } while (false);

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
            if (decoderEnd_) {
                videoFilter_->add(nullptr);
                filterEnd_ = true;
            } else {
                continue;
            }
        } else {
            Frame* frame = frameCtx->getFrame();
            assert(frame->getFrameType() == Frame::FRAME_FFMPEG_TYPE);
            videoFilter_->add(frame);
            delete frameCtx;
            frameCtx = nullptr;
        }

        Frame* filterFrame = videoFilter_->get();
        if (!filterFrame) {
            continue;
        }

        Frame* convertFrame = filterFrame->convert(Frame::FRAME_IMAGE_TYPE);
        ImageFrame* imageFrame = dynamic_cast<ImageFrame*>(convertFrame);
        const MSize size = imageFrame->getSize();
        *data = (char*)imageFrame->getFrame();
        *dataSize = static_cast<int>(size.getWidth()) * 4 * static_cast<int>(size.getHeight());
        *width = static_cast<int>(size.getWidth());
        *height = static_cast<int>(size.getHeight());
        *stride = static_cast<int>(size.getWidth()) * 4;
        *time = filterFrame->getTime();

        delete filterFrame;
        filterFrame = nullptr;

        break;
    }

    return 0;
}

void ImageGeneratorInternal::decodeVideoFrame(FrameContext *frameCtx)
{
    std::lock_guard<std::mutex> guard(videoDecoderBufferMutex_);
    (void)guard;
    videoDecoderBuffer_.push(frameCtx);
}

void ImageGeneratorInternal::clearBuffer()
{
    while (!videoDecoderBuffer_.empty()) {
        FrameContext* frameCtx = videoDecoderBuffer_.front();
        videoDecoderBuffer_.pop();
        delete frameCtx;
        frameCtx = nullptr;
    }
}

ImageGeneratorInternal::VideoDecoderDelegateImpl::VideoDecoderDelegateImpl(ImageGeneratorInternal *internal)
    : internal_(internal)
{
}

ImageGeneratorInternal::VideoDecoderDelegateImpl::~VideoDecoderDelegateImpl()
{
    internal_ = nullptr;
}

void ImageGeneratorInternal::VideoDecoderDelegateImpl::output(FrameContext *frameCtx)
{
    if (internal_) {
        internal_->decodeVideoFrame(frameCtx);
    }
}

}
