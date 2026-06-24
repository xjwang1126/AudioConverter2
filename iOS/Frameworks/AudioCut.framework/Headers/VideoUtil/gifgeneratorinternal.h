#ifndef GIFGENERATORINTERNAL_H
#define GIFGENERATORINTERNAL_H

#include "MediaUtil/mediautilinfo.h"
#include "VideoUtil/videoutilinfo.h"

#include "Common/Codec/Decoder/decoder.h"
#include "Common/Codec/Encoder/encoder.h"
#include "Format/msize.h"

#include <vector>
#include <queue>
#include <mutex>

namespace MediaLibrary {

class VideoFilter;
class VideoEncoder;
class EncoderDelegate;
class MediaMuxer;
class FrameContext;
class PacketContext;

class GIFGeneratorInternal
{
public:
    class VideoDecoderDelegateImpl : public DecoderDelegate {
    public:
        VideoDecoderDelegateImpl(GIFGeneratorInternal* internal);
        virtual ~VideoDecoderDelegateImpl();

        virtual void output(FrameContext* frameCtx);

    private:
        GIFGeneratorInternal* internal_;
    };

    class VideoEncoderDelegateImpl : public EncoderDelegate {
    public:
        VideoEncoderDelegateImpl(GIFGeneratorInternal* internal);
        virtual ~VideoEncoderDelegateImpl();

        virtual void output(PacketContext* packetCtx);

    private:
        GIFGeneratorInternal* internal_;
    };

    friend class VideoDecoderDelegateImpl;
    friend class VideoEncoderDelegateImpl;

public:
    GIFGeneratorInternal();

    bool open(const FillMode& fillMode, const VideoFormat& videoFormat, const MSize& mediaSize, const bool minMode, const int loop, const GIFMode& gifMode);
    void close();

    int addImage(char* data, int dataSize);
    int addImageData(char *data, int dataSize, int width, int height, int stride);
    int addEmptyImageData(bool* end, int* progress);
    void getGIFData(char** data, int* dataSize);
    void getGIFData(char** data, int* dataSize, bool* end, int* progress, int readDataSize);

private:
    void decodeVideoFrame(FrameContext* frameCtx);
    void encodeVideoPacket(PacketContext* packetCtx);

private:
    AVPixelFormat pixelFormat_{AV_PIX_FMT_NONE};
    VideoFilter* videoFilter_{nullptr};
    VideoEncoder* videoEncoder_{nullptr};
    EncoderDelegate* videoEncoderDelegate_{nullptr};
    MediaMuxer* mediaMuxer_;

    std::queue<FrameContext*> videoDecoderBuffer_;
    std::mutex videoDecoderBufferMutex_;

    std::queue<PacketContext*> videoEncoderBuffer_;
    std::mutex videoEncoderBufferMutex_;

    FillMode fillMode_;
    VideoFormat videoFormat_;
    MSize mediaSize_;
    MSize outSize_;
    GIFMode gifMode_;

    int64_t requestTime_{0};
    int64_t frameDuration_{0};

    bool videoFilterFinish_{false};
    bool videoEncoderFinish_{false};
};

}

#endif // GIFGENERATORINTERNAL_H
