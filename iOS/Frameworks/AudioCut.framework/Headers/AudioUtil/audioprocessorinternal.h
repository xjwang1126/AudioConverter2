#ifndef AUDIOPROCESSORINTERNAL_H
#define AUDIOPROCESSORINTERNAL_H

#ifndef ADD_BLOCK_PROCESS
#define ADD_BLOCK_PROCESS 1
#endif

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

class AudioProcessorInternal
{
public:
    class AudioDecoderDelegateImpl : public DecoderDelegate {
    public:
        AudioDecoderDelegateImpl(AudioProcessorInternal* internal);
        virtual ~AudioDecoderDelegateImpl();

        virtual void output(FrameContext* frameCtx);

    private:
        AudioProcessorInternal* internal_;
    };

    class AudioEncoderDelegateImpl : public EncoderDelegate {
    public:
        AudioEncoderDelegateImpl(AudioProcessorInternal* internal);
        virtual ~AudioEncoderDelegateImpl();

        virtual void output(PacketContext* packetCtx);

    private:
        AudioProcessorInternal* internal_;
    };

    friend class AudioDecoderDelegateImpl;
    friend class AudioEncoderDelegateImpl;

public:
    AudioProcessorInternal();

    int open(char* inData, int inDataSize, char* coverData, int coverDataSize, char* formatName, const int64_t startTime, const int64_t endTime, const int64_t fadeIn, const int64_t fadeOut, const float tempo, const float volume, const bool reverse, const TimeMode& timeMode, const AudioFormat& audioFormat, const std::map<MetadataType, string>& metadatas);
    void process();
    void process(bool* end, int* progress);
    void process(bool* end, int* progress, char** data, int* dataSize);
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

    MediaDemuxer* coverMediaDemuxer_{nullptr};

    TimeMode timeMode_;
    int64_t startTime_{0};
    int64_t endTime_{0};

#if ADD_BLOCK_PROCESS
    int64_t requestTime{0};
    int64_t totalTime{0};
    int64_t curStartTime{0};
    int64_t curEndTime{0};

    int64_t trimPakcetTime{-1};

    PacketContext* packetCtx{nullptr};
#endif

    size_t index_{0};

    bool noCover_{false};

    bool noDecode_{false};

    bool cancel_{false};

    bool end_{false};

    std::queue<FrameContext*> audioDecoderBuffer_;
    std::mutex audioDecoderBufferMutex_;

    std::queue<PacketContext*> audioEncoderBuffer_;
    std::mutex audioEncoderBufferMutex_;
};

}

#endif // AUDIOPROCESSORINTERNAL_H
