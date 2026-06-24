#include "rendercontext.h"
#include "Filter/ImageFilter/imagefilter.h"

namespace MediaLibrary {

RenderContext::RenderContext()
{
    init();
}

RenderContext::~RenderContext()
{
    deinit();
}

bool RenderContext::checkRenderContext()
{
    bool result = false;

    switch (ImageFilter::getRenderLanguage()) {
    case ImageFilter::RENDER_OPENGL_LANGUAGE: {
        result = checkRenderContextOpenGL();
    }
        break;
#ifdef IOS_OS
    case ImageFilter::RENDER_METAL_LANGUAGE: {
        result = checkRenderContextMetal();
    }
        break;
#endif
    }

    return result;
}

bool RenderContext::makeRenderContext()
{
    bool result = false;

    switch (ImageFilter::getRenderLanguage()) {
    case ImageFilter::RENDER_OPENGL_LANGUAGE: {
        result = makeRenderContextOpenGL();
    }
        break;
#ifdef IOS_OS
    case ImageFilter::RENDER_METAL_LANGUAGE: {
        result = makeRenderContextMetal();
    }
        break;
#endif
    }

    return result;
}

bool RenderContext::unmakeRenderContext()
{
    bool result = false;

    switch (ImageFilter::getRenderLanguage()) {
    case ImageFilter::RENDER_OPENGL_LANGUAGE: {
        result = unmakeRenderContextOpenGL();
    }
        break;
#ifdef IOS_OS
    case ImageFilter::RENDER_METAL_LANGUAGE: {
        result = unmakeRenderContextMetal();
    }
        break;
#endif
    }

    return result;
}

}
