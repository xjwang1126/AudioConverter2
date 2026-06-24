#include "imageblendfilter.h"
#include "Filter/ImageFilter/openglblendshadercontext.h"
#include "Filter/ImageFilter/metalblendshadercontext.h"

namespace MediaLibrary {

ImageBlendFilter::ImageBlendFilter()
    : ImageFilter(IMAGE_FILTER_BLEND_TYPE)
{
    switch (getRenderLanguage()) {
    case ImageFilter::RENDER_OPENGL_LANGUAGE: {
        shaderCtx_ = new OpenGLBlendShaderContext(this);
    }
        break;
#ifdef IOS_OS
    case ImageFilter::RENDER_METAL_LANGUAGE: {
        shaderCtx_ = new MetalBlendShaderContext(this);
    }
        break;
#endif
    }

    setBlendType(0);
    setBlendIntensity(1.f);
}

void ImageBlendFilter::begin(TextureFrame *textureFrame, const MSize &size, void *context)
{
    switch (getRenderLanguage()) {
    case ImageFilter::RENDER_OPENGL_LANGUAGE: {
        OpenGLBlendShaderContext* openGLBlendShaderCtx = static_cast<OpenGLBlendShaderContext*>(shaderCtx_);
        openGLBlendShaderCtx->begin(textureFrame, size);
    }
        break;
#ifdef IOS_OS
    case ImageFilter::RENDER_METAL_LANGUAGE: {
        MetalBlendShaderContext* metalBlendShaderCtx = static_cast<MetalBlendShaderContext*>(shaderCtx_);
        metalBlendShaderCtx->begin(textureFrame, size, context);
    }
        break;
#endif
    }
}

void ImageBlendFilter::render(TextureFrame *textureFrame, TextureFrame *externalTextureFrame, void *context)
{
    switch (getRenderLanguage()) {
    case ImageFilter::RENDER_OPENGL_LANGUAGE: {
        OpenGLBlendShaderContext* openGLBlendShaderCtx = static_cast<OpenGLBlendShaderContext*>(shaderCtx_);
        openGLBlendShaderCtx->render(textureFrame);
    }
        break;
#ifdef IOS_OS
    case ImageFilter::RENDER_METAL_LANGUAGE: {
        MetalBlendShaderContext* metalBlendShaderCtx = static_cast<MetalBlendShaderContext*>(shaderCtx_);
        metalBlendShaderCtx->render(textureFrame, externalTextureFrame, context);
    }
        break;
#endif
    }
}

void ImageBlendFilter::end(void *context)
{
    switch (getRenderLanguage()) {
    case ImageFilter::RENDER_OPENGL_LANGUAGE: {
        OpenGLBlendShaderContext* openGLBlendShaderCtx = static_cast<OpenGLBlendShaderContext*>(shaderCtx_);
        openGLBlendShaderCtx->end();
    }
        break;
#ifdef IOS_OS
    case ImageFilter::RENDER_METAL_LANGUAGE: {
        MetalBlendShaderContext* metalBlendShaderCtx = static_cast<MetalBlendShaderContext*>(shaderCtx_);
        metalBlendShaderCtx->end(context);
    }
        break;
#endif
    }
}

}
