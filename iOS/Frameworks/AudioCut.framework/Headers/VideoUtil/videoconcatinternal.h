#ifndef VIDEOCONCATINTERNAL_H
#define VIDEOCONCATINTERNAL_H

#include "Common/Codec/Decoder/decoder.h"
#include "Common/Codec/Encoder/encoder.h"
#include "Format/msize.h"

#include "MediaUtil/mediautilinfo.h"
#include "AudioUtil/audioutilinfo.h"

#include <queue>
#include <mutex>

namespace MediaLibrary {

class MediaDemuxer;
class AudioDecoder;
class VideoDecoder;
class DecoderDelegate;
class AudioEncoder;
class VideoEncoder;
class AudioFilter;
class VideoFilter;
class EncoderDelegate;
class MediaMuxer;
class FrameContext;
class PacketContext;

class VideoConcatInternal
{
public:
    class AudioDecoderDelegateImpl : public DecoderDelegate {
    public:
        AudioDecoderDelegateImpl(VideoConcatInternal* internal);
        virtual ~AudioDecoderDelegateImpl();

        virtual void output(FrameContext* frameCtx);

    private:
        VideoConcatInternal* internal_;
    };

    class AudioEncoderDelegateImpl : public EncoderDelegate {
    public:
        AudioEncoderDelegateImpl(VideoConcatInternal* internal);
        virtual ~AudioEncoderDelegateImpl();

        virtual void output(PacketContext* packetCtx);

    private:
        VideoConcatInternal* internal_;
    };

    class VideoDecoderDelegateImpl : public DecoderDelegate {
    public:
        VideoDecoderDelegateImpl(VideoConcatInternal* internal);
        virtual ~VideoDecoderDelegateImpl();

        virtual void output(FrameContext* frameCtx);

    private:
        VideoConcatInternal* internal_;
    };

    class VideoEncoderDelegateImpl : public EncoderDelegate {
    public:
        VideoEncoderDelegateImpl(VideoConcatInternal* internal);
        virtual ~VideoEncoderDelegateImpl();

        virtual void output(PacketContext* packetCtx);

    private:
        VideoConcatInternal* internal_;
    };

    friend class AudioDecoderDelegateImpl;
    friend class AudioEncoderDelegateImpl;
    friend class VideoDecoderDelegateImpl;
    friend class VideoEncoderDelegateImpl;

public:
    VideoConcatInternal();

    int open();
    int append(char* inData, int inDataSize);
    int append2(char* inData, int inDataSize);
    void process2(bool* end, int* progress);
    void cancel();
    void close();

    void getFileData(char** data, int* dataSize);
    void getFileData(char** data, int* dataSize, bool* end, int* progress, int readDataSize);

private:
    void decodeAudioFrame(FrameContext* frameCtx);
    void encodeAudioPacket(PacketContext* packetCtx);
    void decodeVideoFrame(FrameContext* frameCtx);
    void encodeVideoPacket(PacketContext* packetCtx);

private:
    MediaDemuxer* mediaDemuxer_{nullptr};
    AudioDecoder* audioDecoder_{nullptr};
    VideoDecoder* videoDecoder_{nullptr};
    DecoderDelegate* audioDecoderDelegate_{nullptr};
    DecoderDelegate* videoDecoderDelegate_{nullptr};
    AudioFilter* audioFilter_{nullptr};
    VideoFilter* videoFilter_{nullptr};
    AudioEncoder* audioEncoder_{nullptr};
    VideoEncoder* videoEncoder_{nullptr};
    EncoderDelegate* audioEncoderDelegate_{nullptr};
    EncoderDelegate* videoEncoderDelegate_{nullptr};
    MediaMuxer* mediaMuxer_{nullptr};

    bool cancel_{false};

    bool end_{false};

    int64_t offsetTime_{0};

    std::queue<FrameContext*> audioDecoderBuffer_;
    std::mutex audioDecoderBufferMutex_;

    std::queue<FrameContext*> videoDecoderBuffer_;
    std::mutex videoDecoderBufferMutex_;

    std::queue<PacketContext*> audioEncoderBuffer_;
    std::mutex audioEncoderBufferMutex_;

    std::queue<PacketContext*> videoEncoderBuffer_;
    std::mutex videoEncoderBufferMutex_;
};

}

#endif // VIDEOCONCATINTERNAL_H
