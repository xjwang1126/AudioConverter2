#ifndef VIDEOENCODER_H
#define VIDEOENCODER_H

#include "encoder.h"

namespace MediaLibrary {

class VideoEncoder : public Encoder
{
public:
    VideoEncoder(EncoderDelegate* delegate = nullptr);
    virtual ~VideoEncoder();

    virtual bool open(const CodecType& codecType, MediaMuxer* mediaMuxer, const std::map<CodecKey, CodecValue>& codecInfos) override;
    virtual void close() override;
    virtual bool encode(Frame *frame) override;

private:
    virtual bool openSoftwareEncoder(const std::map<CodecKey, CodecValue>& codecInfos) override;
    virtual void closeSoftwareEncoder() override;
    virtual bool softwareEncoderEncode(Frame* frame) override;
};

}

#endif // VIDEOENCODER_H
