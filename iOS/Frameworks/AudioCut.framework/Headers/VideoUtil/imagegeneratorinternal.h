#ifndef IMAGEGENERATORINTERNAL_H
#define IMAGEGENERATORINTERNAL_H

#include "Common/Format/msize.h"
#include "Common/Model/imageframe.h"
#include "Common/Model/ffmpegframe.h"
#include "Common/Media/mediademuxer.h"
#include "Common/Codec/Decoder/videodecoder.h"
#include "Common/Filter/VideoFilter/videofilter.h"

#include <string>
#include <queue>

namespace MediaLibrary {

class ImageGeneratorInternal
{
public:
    class VideoDecoderDelegateImpl : public DecoderDelegate {
    public:
        VideoDecoderDelegateImpl(ImageGeneratorInternal* internal);
        virtual ~VideoDecoderDelegateImpl();

        virtual void output(FrameContext* frameCtx);

    private:
        ImageGeneratorInternal* internal_;
    };

    friend class VideoDecoderDelegateImpl;

public:
    ImageGeneratorInternal();

    void setSize(const MSize& size, const bool minMode);

    int open(const std::string& filePath);
    int open(char* data, int dataSize);
    void close();

    void seek(const int64_t time);

    int getImageData(char** data, int* dataSize, int* width, int* height, int* stride, int64_t* time, bool* end);

private:
    void decodeVideoFrame(FrameContext* frameCtx);

    void clearBuffer();

private:
    std::string filePath_;
    MSize size_;
    bool minMode_;
    MediaDemuxer mediaDemuxer_;
    VideoDecoder* videoDecoder_{nullptr};
    DecoderDelegate* videoDecoderDelegate_{nullptr};
    VideoFilter* videoFilter_{nullptr};
    bool demuxerEnd_{false};
    bool decoderEnd_{false};
    bool filterEnd_{false};

    std::queue<FrameContext*> videoDecoderBuffer_;
    std::mutex videoDecoderBufferMutex_;
};

}

#endif // IMAGERETRIVERINTERNAL_H
