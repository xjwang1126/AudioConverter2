#include "imagefilter.h"
#include "shadercontext.h"
#include "Filter/ImageFilter/openglshadercontext.h"
#include "Filter/ImageFilter/metalshadercontext.h"
#include "Model/textureframe.h"

namespace MediaLibrary {

ImageFilter::RenderLanguage ImageFilter::renderLanguage_ = ImageFilter::RENDER_OPENGL_LANGUAGE;

ImageFilter::ImageFilter(const ImageFilterType &imageFilterType)
    : imageFilterType_(imageFilterType)
{
}

void ImageFilter::setVertexContent(const std::string &value)
{
    shaderCtx_->setVertexContent(value);
}

void ImageFilter::setFragmentContent(const std::string &value)
{
    shaderCtx_->setFragmentContent(value);
}

bool ImageFilter::createShader()
{
    return shaderCtx_->createShader();
}

ImageFilter::~ImageFilter()
{
}

void ImageFilter::checkRenderLanguage()
{
    renderLanguage_ = ImageFilter::RENDER_OPENGL_LANGUAGE;
}

ImageFrame *ImageFilter::read(const TextureFrame *textureFrame)
{
    return shaderCtx_->read(textureFrame, PIXEL_FORMAT_RGBA);
}

void *ImageFilter::createTexture(const MSize &size, const PixelFormat &pixelFormat, const TextureUsage &textureUsage)
{
    void* texture = nullptr;
    switch (getRenderLanguage()) {
    case ImageFilter::RENDER_OPENGL_LANGUAGE: {
        texture = OpenGLShaderContext::createTexture(size, pixelFormat, textureUsage);
    }
        break;
#ifdef IOS_OS
    case ImageFilter::RENDER_METAL_LANGUAGE: {
        texture = MetalShaderContext::createTexture(size, pixelFormat, textureUsage);
    }
        break;
#endif
    }
    return texture;
}

void ImageFilter::destroyTexture(void *texture)
{
    switch (getRenderLanguage()) {
    case ImageFilter::RENDER_OPENGL_LANGUAGE: {
        OpenGLShaderContext::destroyTexture(texture);
    }
        break;
#ifdef IOS_OS
    case ImageFilter::RENDER_METAL_LANGUAGE: {
        MetalShaderContext::destroyTexture(texture);
    }
        break;
#endif
    }
}

TextureFrame *ImageFilter::createTextureFrame(const MSize &size, const PixelFormat &pixelFormat, const TextureUsage &textureUsage)
{
    void* texture = ImageFilter::createTexture(size, pixelFormat, textureUsage);

    TextureFrame* textureFrame = new TextureFrame;
    textureFrame->setTimeBase(TimeBase(1, SECOND_TO_MICROSECOND_UNIT));
    textureFrame->setTime(-1);
    textureFrame->setDuration(0);
    textureFrame->setFrame(texture);
    textureFrame->setSize(size);

    return textureFrame;
}

const float *ImageFilter::textureCoordinatesForRotation(const ImageFilter::ImageRotationMode &rotationMode)
{
    const float* textureCoordinates = nullptr;
    switch (getRenderLanguage()) {
    case ImageFilter::RENDER_OPENGL_LANGUAGE: {
        textureCoordinates = OpenGLShaderContext::textureCoordinatesForRotation(rotationMode);
    }
        break;
#ifdef IOS_OS
    case ImageFilter::RENDER_METAL_LANGUAGE: {
        textureCoordinates = MetalShaderContext::textureCoordinatesForRotation(rotationMode);
    }
        break;
#endif
    }
    return textureCoordinates;
}

}
