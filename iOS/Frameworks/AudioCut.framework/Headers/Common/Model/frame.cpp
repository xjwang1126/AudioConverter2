#include "frame.h"

namespace MediaLibrary {

Frame::Frame(const FrameType &frameType)
    : frameType_(frameType)
{
}

Frame::Frame(const Frame &frame)
    : frameType_(frame.frameType_)
    , timeBase_(frame.timeBase_)
{
}

Frame::~Frame()
{
}

}
