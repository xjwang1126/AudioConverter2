#ifndef VIDEODECODER_H
#define VIDEODECODER_H

#include "Codec/codecinfo.h"
#include "decoder.h"

#include <map>

namespace MediaLibrary {

class VideoDecoder : public Decoder
{
public:
    VideoDecoder(DecoderDelegate* delegate = nullptr);
    virtual ~VideoDecoder();

    virtual bool open(const CodecType& decoderType, MediaDemuxer* mediaDemuxer, const std::map<CodecKey, CodecValue>& codecInfos = {}) override;
    virtual void close() override;
    virtual bool decode(PacketContext* packetCtx) override;
    virtual void flush() override;

protected:
    virtual bool openSoftwareDecoder() override;
    virtual void closeSoftwareDecoder() override;
    virtual bool softwareDecoderDecode(PacketContext* packetCtx) override;
    virtual void softwareDecoderFlush() override;
};

}

#endif // VIDEODECODER_H
