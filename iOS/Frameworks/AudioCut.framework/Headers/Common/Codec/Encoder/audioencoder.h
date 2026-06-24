#ifndef AUDIOENCODER_H
#define AUDIOENCODER_H

#include "encoder.h"

namespace MediaLibrary {

class AudioEncoder : public Encoder
{
public:
    AudioEncoder(EncoderDelegate* delegate = nullptr);
    virtual ~AudioEncoder();

    virtual bool open(const CodecType& codecType, MediaMuxer* mediaMuxer, const std::map<CodecKey, CodecValue>& codecInfos) override;
    virtual void close() override;
    virtual bool encode(Frame *frame) override;

private:
    virtual bool openSoftwareEncoder(const std::map<CodecKey, CodecValue>& codecInfos) override;
    virtual void closeSoftwareEncoder() override;
    virtual bool softwareEncoderEncode(Frame* frame) override;
};

}

#endif // AUDIOENCODER_H
