#include "framecontext.h"
#include "frame.h"

namespace MediaLibrary {

FrameContext::FrameContext()
{
}

FrameContext::~FrameContext()
{
    if (frame_) {
        delete frame_;
        frame_ = nullptr;
    }
}

int64_t FrameContext::getTime(const TimeBase &timeBase)
{
    if (!frame_) {
        return -1;
    }
    return frame_->getTime(timeBase);
}

int64_t FrameContext::getDuration(const TimeBase &timeBase)
{
    if (!frame_) {
        return 0;
    }
    return frame_->getDuration(timeBase);
}

}
