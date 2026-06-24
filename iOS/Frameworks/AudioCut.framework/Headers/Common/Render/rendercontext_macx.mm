#include "rendercontext.h"

namespace MediaLibrary {

struct RenderContextInternal {
};

bool RenderContext::init()
{
    renderCtx_ = new RenderContextInternal;
    return true;
}

void RenderContext::deinit()
{
    if (renderCtx_) {
        delete renderCtx_;
        renderCtx_ = nullptr;
    }
}

bool RenderContext::initRenderContext(RenderContext *renderCtx)
{
    return false;
}

void RenderContext::deinitRenderContext()
{
}

void *RenderContext::getShareContext()
{
    return nullptr;
}

bool RenderContext::checkRenderContextOpenGL()
{
    return false;
}

bool RenderContext::makeRenderContextOpenGL()
{
    return false;
}

bool RenderContext::unmakeRenderContextOpenGL()
{
    return false;
}

}
