#include "videooutput.h"

namespace MediaLibrary {

void VideoOutput::display()
{
}

void VideoOutput::display(TextureFrame *textureFrame)
{
}

bool VideoOutput::initRenderContext()
{
    return false;
}

void VideoOutput::deinitRenderContext()
{
}

bool VideoOutput::init()
{
    return false;
}

void VideoOutput::deinit()
{
}

void VideoOutput::beginBlendCtx()
{
}

void VideoOutput::endBlendCtx()
{
}

void *VideoOutput::getBlendCtx()
{
    return nullptr;
}

}
