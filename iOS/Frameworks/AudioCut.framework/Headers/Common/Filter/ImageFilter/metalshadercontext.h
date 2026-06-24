#ifndef METALSHADERCONTEXT_H
#define METALSHADERCONTEXT_H

#include "shadercontext.h"
#include "imagefilter.h"

namespace MediaLibrary {

class MetalShaderContext : public ShaderContext
{
public:
    MetalShaderContext();
    virtual ~MetalShaderContext() override;

    virtual void setCommandBufferLabel(const std::string& value) override;

    virtual TextureFrame* render(const TextureFrame* textureFrame, const MSize& surfaceSize, void* context) override;
    virtual TextureFrame* render(const ImageFrame* imageFrame) override;

    virtual ImageFrame* read(const TextureFrame* textureFrame, const PixelFormat& pixelFormat) override;

    static void* createTexture(const MSize& size, const PixelFormat& pixelFormat, const TextureUsage& textureUsage);
    static void destroyTexture(void* texture);

    virtual bool createShader() override;

    static const float* textureCoordinatesForRotation(const ImageFilter::ImageRotationMode& rotationMode);

protected:
    virtual void createUnifrom();
    virtual void destroyUniform();
    virtual void updateUniform(void *context);

    virtual void createVertex();
    virtual void destroyVertex();
    virtual void updateVertex(void *context);

    virtual void updateTexture(void *context);

    virtual void drawPrimitive(void *context);

protected:
    void* renderPassDescriptor{nullptr};
    void* renderPipelineState{nullptr};
    void* vertex{nullptr};
    void* uniform{nullptr};
    void* commandBufferLabel{nullptr};
};

}

#endif // METALSHADERCONTEXT_H
