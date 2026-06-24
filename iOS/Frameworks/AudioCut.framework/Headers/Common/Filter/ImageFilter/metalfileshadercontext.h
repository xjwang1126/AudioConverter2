#ifndef METALFILESHADERCONTEXT_H
#define METALFILESHADERCONTEXT_H

#include "Filter/ImageFilter/metalshadercontext.h"

namespace MediaLibrary {

class Frame;
class ImageFileFilter;

class MetalFileShaderContext : public MetalShaderContext
{
public:
    MetalFileShaderContext(ImageFileFilter* fileFilter);

    void setTextureCache(void* textureCache);

    TextureFrame* render(Frame* frame, void* context);

private:
    ImageFileFilter* fileFilter_{nullptr};

    void* textureCache_{nullptr};
};

}

#endif // METALFILESHADERCONTEXT_H
