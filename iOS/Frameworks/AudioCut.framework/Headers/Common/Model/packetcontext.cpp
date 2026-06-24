#include "packetcontext.h"

namespace MediaLibrary {

PacketContext::~PacketContext()
{
    if (packet_) {
        av_packet_free(&packet_);
        packet_ = nullptr;
    }
}

int64_t PacketContext::getTime(const TimeBase &timeBase) const
{
    if (!packet_) {
        return -1;
    }
    return static_cast<int64_t>((packet_->pts * 1. * timeBase.den) / (timeBase.num * AV_TIME_BASE));
}

int64_t PacketContext::getDuration(const TimeBase &timeBase) const
{
    if (!packet_) {
        return 0;
    }
    return static_cast<int64_t>((packet_->duration * 1. * timeBase.den) / (timeBase.num * AV_TIME_BASE));
}

}
