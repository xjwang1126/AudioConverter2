#ifndef AUDIOGENERATORINTERNAL_H
#define AUDIOGENERATORINTERNAL_H

#include "audiogeneratorinfo.h"

#include "Common/Codec/Decoder/decoder.h"
#include "Common/Codec/Encoder/encoder.h"
#include "Format/msize.h"

#include <queue>
#include <mutex>

namespace MediaLibrary {

class MediaDemuxer;
class AudioDecoder;
class DecoderDelegate;
class AudioEncoder;
class AudioFilter;
class EncoderDelegate;
class MediaMuxer;
class FrameContext;
class PacketContext;

class AudioGeneratorInternal
{
public:
    class AudioDecoderDelegateImpl : public DecoderDelegate {
    public:
        AudioDecoderDelegateImpl(AudioGeneratorInternal* internal);
        virtual ~AudioDecoderDelegateImpl();

        virtual void output(FrameContext* frameCtx);

    private:
        AudioGeneratorInternal* internal_;
    };

    class AudioEncoderDelegateImpl : public EncoderDelegate {
    public:
        AudioEncoderDelegateImpl(AudioGeneratorInternal* internal);
        virtual ~AudioEncoderDelegateImpl();

        virtual void output(PacketContext* packetCtx);

    private:
        AudioGeneratorInternal* internal_;
    };

    friend class AudioDecoderDelegateImpl;
    friend class AudioEncoderDelegateImpl;

public:
    AudioGeneratorInternal();

    bool open(ReadCallback readCallback, SeekCallback demuxerSeekCallback, WriteCallback writeCallback, SeekCallback muxerSeekCallback, const std::string& formatName);
    void start();
    void stop();
    void close();

private:
    void decodeAudioFrame(FrameContext* frameCtx);
    void encodeAudioPacket(PacketContext* packetCtx);

    static int readPacket(void* opaque, uint8_t* buffer, int bufferSize);
    static int64_t demuxerSeek(void* opaque, int64_t offset, int whence);
    static int writePacket(void* opaque, uint8_t* buffer, int bufferSize);
    static int64_t muxerSeek(void* opaque, int64_t offset, int whence);

private:
    MediaDemuxer* mediaDemuxer_{nullptr};
    AudioDecoder* audioDecoder_{nullptr};
    DecoderDelegate* audioDecoderDelegate_{nullptr};
    AudioFilter* audioFilter_{nullptr};
    AudioEncoder* audioEncoder_{nullptr};
    EncoderDelegate* audioEncoderDelegate_{nullptr};
    MediaMuxer* mediaMuxer_{nullptr};

    bool noCodec_{false};

    bool stop_{false};

    std::queue<FrameContext*> audioDecoderBuffer_;
    std::mutex audioDecoderBufferMutex_;

    std::queue<PacketContext*> audioEncoderBuffer_;
    std::mutex audioEncoderBufferMutex_;

    ReadCallback readCallback_;
    SeekCallback demuxerSeekCallback_;
    WriteCallback wirteCallback_;
    SeekCallback muxerSeekCallback_;
};

}

#endif // AUDIOGENERATORINTERNAL_H
