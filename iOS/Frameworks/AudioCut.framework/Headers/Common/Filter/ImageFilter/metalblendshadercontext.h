#ifndef METALBLENDSHADERCONTEXT_H
#define METALBLENDSHADERCONTEXT_H

#include "metalshadercontext.h"

namespace MediaLibrary {

class ImageBlendFilter;

class MetalBlendShaderContext : public MetalShaderContext
{
public:
    MetalBlendShaderContext(ImageBlendFilter* blendFilter);

    void begin(TextureFrame* textureFrame, const MSize& size, void* context);
    void render( TextureFrame* textureFrame, TextureFrame* externalTextureFrame, void* context);
    void end(void* context);

private:
    ImageBlendFilter* blendFilter_{nullptr};
};

}

#endif // METALBLENDSHADERCONTEXT_H
