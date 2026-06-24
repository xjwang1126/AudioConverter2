#include "metalfileshadercontext.h"

namespace MediaLibrary {

MetalFileShaderContext::MetalFileShaderContext(ImageFileFilter *fileFilter)
    : MetalShaderContext()
    , fileFilter_(fileFilter)
{
}

void MetalFileShaderContext::setTextureCache(void *textureCache)
{
    textureCache_ = textureCache;
}

TextureFrame *MetalFileShaderContext::render(Frame *frame, void *context)
{
    (void)frame;(void)context;
    return nullptr;
}

}
