#include "shadercontext.h"

namespace MediaLibrary {

ShaderContext::ShaderContext()
{
}

ShaderContext::~ShaderContext()
{
}

TextureFrame *ShaderContext::render(const TextureFrame *textureFrame, const MSize &surfaceSize, void *context)
{
    (void)textureFrame;(void)surfaceSize;(void)context;
    return nullptr;
}

TextureFrame *ShaderContext::render(const ImageFrame *imageFrame)
{
    (void)imageFrame;
    return nullptr;
}

TextureFrame *ShaderContext::render(const ImageFrame *imageFrame, const PixelFormat &pixelFormat)
{
    (void)imageFrame;(void)pixelFormat;
    return nullptr;
}

ImageFrame *ShaderContext::read(const TextureFrame *textureFrame, const PixelFormat &pixelFormat)
{
    (void)textureFrame;(void)pixelFormat;
    return nullptr;
}

bool ShaderContext::createShader()
{
    return false;
}

void ShaderContext::setCommandBufferLabel(const string &value)
{
    (void)value;
}

}
