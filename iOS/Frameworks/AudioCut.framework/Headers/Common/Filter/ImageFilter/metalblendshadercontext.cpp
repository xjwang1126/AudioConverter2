#include "metalblendshadercontext.h"

namespace MediaLibrary {

MetalBlendShaderContext::MetalBlendShaderContext(ImageBlendFilter *blendFilter)
    : blendFilter_(blendFilter)
{
}

void MetalBlendShaderContext::begin(TextureFrame *textureFrame, const MSize &size, void *context)
{
    (void)textureFrame;(void)size;(void)context;
}

void MetalBlendShaderContext::render(TextureFrame *textureFrame, TextureFrame *externalTextureFrame, void *context)
{
    (void)textureFrame;(void)externalTextureFrame;(void)context;
}

void MetalBlendShaderContext::end(void *context)
{
    (void)context;
}

}
