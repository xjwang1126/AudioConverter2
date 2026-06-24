#ifndef DECODER_H
#define DECODER_H

extern "C" {
#include <libavformat/avformat.h>
}

#include "Codec/codecinfo.h"

#include <map>
#include <string>

namespace MediaLibrary {

class PacketContext;
class FrameContext;
class MediaDemuxer;

class DecoderDelegate {
public:
    virtual ~DecoderDelegate() = default;

    virtual void output(FrameContext* frameCtx) = 0;
};

class Decoder
{
public:
    virtual bool open(const CodecType& decoderType, MediaDemuxer* mediaDemuxer, const std::map<CodecKey, CodecValue>& codecInfos) = 0;
    virtual void close() = 0;
    virtual bool decode(PacketContext* packetCtx) = 0;
    virtual void flush() = 0;

public:
    const std::map<CodecKey, CodecValue>& getCodecInfos() const { return codecInfos_; }

protected:
    Decoder(DecoderDelegate* delegate = nullptr);
    virtual ~Decoder();

    virtual bool openSoftwareDecoder() = 0;
    virtual void closeSoftwareDecoder() = 0;
    virtual bool softwareDecoderDecode(PacketContext* packetCtx) = 0;
    virtual void softwareDecoderFlush() = 0;

protected:
    CodecType codecType_{CODEC_HARDWARE_TYPE};
    MediaDemuxer* mediaDemuxer_{nullptr};
    DecoderDelegate* delegate_{nullptr};
    AVCodecContext* softwareDecoder_;
    std::map<CodecKey, CodecValue> codecInfos_;
};

}

#endif // DECODER_H
