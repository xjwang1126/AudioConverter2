#ifndef IMAGEBLENDFILTER_H
#define IMAGEBLENDFILTER_H

#include "imagefilter.h"

namespace MediaLibrary {

class TextureFrame;

class ImageBlendFilter : public ImageFilter
{
public:
    ImageBlendFilter();

    void setBlendType(const int blendType) { blendType_ = blendType; }
    void setBlendIntensity(const float blendIntensity) { blendIntensity_ = blendIntensity; }

    int getBlendType() const { return blendType_; }
    float getBlendIntensity() const { return blendIntensity_; }

    void begin(TextureFrame *textureFrame, const MSize& size, void* context);
    void render(TextureFrame* textureFrame, TextureFrame *externalTextureFrame, void* context);
    void end(void* context);

private:
    int blendType_{0};
    float blendIntensity_{1.0f};
};

}

#endif // IMAGEBLENDFILTER_H
