#ifndef AUDIODECODER_H
#define AUDIODECODER_H

#include "Codec/codecinfo.h"
#include "decoder.h"

namespace MediaLibrary {

class AudioDecoder : public Decoder
{
public:
    AudioDecoder(DecoderDelegate* delegate = nullptr);
    virtual ~AudioDecoder();

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

#endif // AUDIODECODER_H
