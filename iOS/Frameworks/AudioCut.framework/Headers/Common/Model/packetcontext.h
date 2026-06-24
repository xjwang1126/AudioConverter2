#ifndef PACKETCONTEXT_H
#define PACKETCONTEXT_H

#include "frameinfo.h"

extern "C" {
#include <libavformat/avformat.h>
}

namespace MediaLibrary {

class PacketContext
{
public:
    enum PacketType {
        PACKET_NONE_TYPE = 0,
        PACKET_AUDIO_TYPE,
        PACKET_VIDEO_TYPE,
    };

public:
    ~PacketContext();

    void setPacket(AVPacket* packet) { packet_ = packet; }
    AVPacket* getPacket() const { return packet_; }

    void setPacketType(const PacketType& packetType) { packetType_ = packetType; }
    PacketType getPacketType() const { return packetType_; }

    int64_t getTime(const TimeBase& timeBase = {1, SECOND_TO_MILLISECOND_UNIT}) const;
    int64_t getDuration(const TimeBase& timeBase = {1, SECOND_TO_MILLISECOND_UNIT}) const;

private:
    AVPacket* packet_{nullptr};
    PacketType packetType_{PACKET_NONE_TYPE};
};

}

#endif // PACKETCONTEXT_H
