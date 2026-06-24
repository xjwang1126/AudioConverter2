#include "imagefilefilter.h"
#include "Model/frame.h"
#include "Model/textureframe.h"
#include "Filter/ImageFilter/metalfileshadercontext.h"
#include "Filter/ImageFilter/openglfileshadercontext.h"

namespace MediaLibrary {

ImageFileFilter::ImageFileFilter()
    : ImageFilter(ImageFilter::IMAGE_FILTER_FILE_TYPE)
{
    switch (getRenderLanguage()) {
    case ImageFilter::RENDER_OPENGL_LANGUAGE: {
        shaderCtx_ = new OpenGLFileShaderContext(this);
    }
        break;
#ifdef IOS_OS
    case ImageFilter::RENDER_METAL_LANGUAGE: {
        shaderCtx_ = new MetalFileShaderContext(this);
    }
        break;
#endif
    }
}

#ifdef IOS_OS
void ImageFileFilter::setTextureCache(void *textureCache)
{
    switch (getRenderLanguage()) {
    case RENDER_OPENGL_LANGUAGE: {
        OpenGLFileShaderContext* openGLFileShaderCtx = static_cast<OpenGLFileShaderContext*>(shaderCtx_);
        openGLFileShaderCtx->setTextureCache(textureCache);
    }
        break;
    case RENDER_METAL_LANGUAGE: {
        MetalFileShaderContext* metalFileShaderCtx = static_cast<MetalFileShaderContext*>(shaderCtx_);
        metalFileShaderCtx->setTextureCache(textureCache);
    }
        break;
    }
}
#endif

TextureFrame *ImageFileFilter::render(Frame *frame, const MSize &size, const float *position, const float *textureCoordinate, const Matrix &projectionMatrix, const Matrix &modelViewMatrix, void *context)
{
    TextureFrame* fileTextureFrame = nullptr;

    switch (getRenderLanguage()) {
    case ImageFilter::RENDER_OPENGL_LANGUAGE: {
        OpenGLFileShaderContext* openGLFileShaderCtx = static_cast<OpenGLFileShaderContext*>(shaderCtx_);
        fileTextureFrame = openGLFileShaderCtx->render(frame, size, position, textureCoordinate, projectionMatrix, modelViewMatrix);
    }
        break;
#ifdef IOS_OS
    case ImageFilter::RENDER_METAL_LANGUAGE: {
        MetalFileShaderContext* metalFileShaderCtx = static_cast<MetalFileShaderContext*>(shaderCtx_);
        fileTextureFrame = metalFileShaderCtx->render(frame, context);
    }
        break;
#endif
    }

    fileTextureFrame->setTimeBase(TimeBase(1, SECOND_TO_MILLISECOND_UNIT));
    fileTextureFrame->setTime(frame->getTime());
    fileTextureFrame->setDuration(frame->getDuration());

    return fileTextureFrame;
}

ImageFrame *ImageFileFilter::read(TextureFrame *textureFrame)
{
    MSize size = textureFrame->getSize();
    Matrix projectionMatrix;
    Matrix modelViewMatrix;
    TextureFrame* fileTextureFrame = render(textureFrame, size, nullptr, nullptr, projectionMatrix, modelViewMatrix, nullptr);
    ImageFrame* imageFrame = ImageFilter::read(fileTextureFrame);
    delete fileTextureFrame;
    fileTextureFrame = nullptr;
    return imageFrame;
}

}
