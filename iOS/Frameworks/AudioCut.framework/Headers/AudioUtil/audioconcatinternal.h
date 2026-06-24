#ifndef AUDIOCONCATINTERNAL_H
#define AUDIOCONCATINTERNAL_H

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
class DecoderDelegate;
class AudioEncoder;
class AudioFilter;
class EncoderDelegate;
class MediaMuxer;
class FrameContext;
class PacketContext;

class AudioConcatInternal
{
public:
    class AudioDecoderDelegateImpl : public DecoderDelegate {
    public:
        AudioDecoderDelegateImpl(AudioConcatInternal* internal);
        virtual ~AudioDecoderDelegateImpl();

        virtual void output(FrameContext* frameCtx);

    private:
        AudioConcatInternal* internal_;
    };

    class AudioEncoderDelegateImpl : public EncoderDelegate {
    public:
        AudioEncoderDelegateImpl(AudioConcatInternal* internal);
        virtual ~AudioEncoderDelegateImpl();

        virtual void output(PacketContext* packetCtx);

    private:
        AudioConcatInternal* internal_;
    };

    friend class AudioDecoderDelegateImpl;
    friend class AudioEncoderDelegateImpl;

public:
    AudioConcatInternal();

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

private:
    MediaDemuxer* mediaDemuxer_{nullptr};
    AudioDecoder* audioDecoder_{nullptr};
    DecoderDelegate* audioDecoderDelegate_{nullptr};
    AudioFilter* audioFilter_{nullptr};
    AudioEncoder* audioEncoder_{nullptr};
    EncoderDelegate* audioEncoderDelegate_{nullptr};
    MediaMuxer* mediaMuxer_{nullptr};

    bool cancel_{false};

    bool end_{false};

    int64_t offsetTime_{0};

    std::queue<FrameContext*> audioDecoderBuffer_;
    std::mutex audioDecoderBufferMutex_;

    std::queue<PacketContext*> audioEncoderBuffer_;
    std::mutex audioEncoderBufferMutex_;
};

}

#endif // AUDIOCONCATINTERNAL_H
