#ifndef ENCODER_H
#define ENCODER_H

#include "Codec/codecinfo.h"
#include "Media/mediamuxer.h"

#include <map>

namespace MediaLibrary {

class Frame;
class PacketContext;

class EncoderDelegate {
public:
    virtual ~EncoderDelegate() = default;

    virtual void output(PacketContext* packetCtx) = 0;
};

class Encoder
{
public:
    virtual bool open(const CodecType& codecType, MediaMuxer* mediaMuxer, const std::map<CodecKey, CodecValue>& codecInfos) = 0;
    virtual void close() = 0;
    virtual bool encode(Frame* frame) = 0;

public:
    const std::map<CodecKey, CodecValue>& getCodecInfos() const { return codecInfos_; }

protected:
    Encoder(EncoderDelegate* delegate = nullptr);
    virtual ~Encoder();

    virtual bool openSoftwareEncoder(const std::map<CodecKey, CodecValue>& codecInfos) = 0;
    virtual void closeSoftwareEncoder() = 0;
    virtual bool softwareEncoderEncode(Frame* frame) = 0;

protected:
    CodecType codecType_{CODEC_HARDWARE_TYPE};
    EncoderDelegate* delegate_{nullptr};
    void* softwareEncoder_;
    MediaMuxer* mediaMuxer_{nullptr};
    std::map<CodecKey, CodecValue> codecInfos_;
};

}

#endif // ENCODER_H
