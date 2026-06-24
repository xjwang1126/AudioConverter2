#ifndef MEDIAUTILINTERNAL_H
#define MEDIAUTILINTERNAL_H

#include "mediautilinfo.h"
#include "Common/Codec/Decoder/decoder.h"
#include "Common/Codec/Encoder/encoder.h"

#include <string>
#include <queue>
#include <mutex>

using namespace std;

namespace MediaLibrary {

class MediaUtilDelegate;

class MediaUtilInternal
{
public:
    class VideoDecoderDelegateImpl : public DecoderDelegate {
    public:
        VideoDecoderDelegateImpl(MediaUtilInternal* mediaUtil);
        virtual ~VideoDecoderDelegateImpl();

        virtual void output(FrameContext* frameCtx);

    private:
        MediaUtilInternal* mediaUtil_;
    };

    class AudioDecoderDelegateImpl : public DecoderDelegate {
    public:
        AudioDecoderDelegateImpl(MediaUtilInternal* mediaUtil);
        virtual ~AudioDecoderDelegateImpl();

        virtual void output(FrameContext* frameCtx);

    private:
        MediaUtilInternal* mediaUtil_;
    };

    class VideoEncoderDelegateImpl : public EncoderDelegate {
    public:
        VideoEncoderDelegateImpl(MediaUtilInternal* mediaUtil);
        virtual ~VideoEncoderDelegateImpl();

        virtual void output(PacketContext* packetCtx);

    private:
        MediaUtilInternal* mediaUtil_;
    };

    class AudioEncoderDelegateImpl : public EncoderDelegate {
    public:
        AudioEncoderDelegateImpl(MediaUtilInternal* mediaUtil);
        virtual ~AudioEncoderDelegateImpl();

        virtual void output(PacketContext* packetCtx);

    private:
        MediaUtilInternal* mediaUtil_;
    };

    friend class VideoDecoderDelegateImpl;
    friend class VideoEncoderDelegateImpl;

public:
    MediaUtilInternal();

    void setQuietLog();

    bool transcode(const string& inFilePath, const string& outFilePath, const int64_t startTime, const int64_t endTime, const VideoFormat& videoFormat, const AudioFormat& audioFormat, const std::map<TaskType, int>& extraTasks, const MediaUtilDelegate* delegate);

    bool reverse(const string& inFilePath, const string& outFilePath, const MediaUtilDelegate* delegate);

    bool memoryFile(const string& inFilePath, const string& outFilePath, const MediaUtilDelegate* delegate);

    const string checkSupportFormatCodec();

    MediaInfo checkMediaInfo(const string& filePath);

    void cancel();

private:
    void decodeVideoFrame(FrameContext* frameCtx);
    void decodeAudioFrame(FrameContext* frameCtx);
    void encodeVideoPacket(PacketContext* packetCtx);
    void encodeAudioPacket(PacketContext* packetCtx);

protected:
    void clearBuffer();

protected:
    std::queue<FrameContext*> videoDecoderBuffer_;
    std::mutex videoDecoderBufferMutex_;

    std::queue<FrameContext*> audioDecoderBuffer_;
    std::mutex audioDecoderBufferMutex_;

    std::queue<PacketContext*> videoEncoderBuffer_;
    std::mutex videoEncoderBufferMutex_;

    std::queue<PacketContext*> audioEncoderBuffer_;
    std::mutex audioEncoderBufferMutex_;

    atomic_bool cancel_{false};
};

}

#endif // MEDIAUTILINTERNAL_H
