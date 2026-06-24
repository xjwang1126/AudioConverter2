#ifndef IMAGEFILTER_H
#define IMAGEFILTER_H

#include <string>

#include "Format/msize.h"
#include "Format/pixelformat.h"
#include "Format/textureusage.h"

namespace MediaLibrary {

class ShaderContext;
class TextureFrame;
class ImageFrame;

class ImageFilter
{
public:
    enum RenderLanguage {
        RENDER_OPENGL_LANGUAGE,
        RENDER_METAL_LANGUAGE,
    };

    enum ImageFilterType {
        IMAGE_FILTER_NONE_TYPE = 0,

        IMAGE_FILTER_FILE_TYPE,
        IMAGE_FILTER_BLEND_TYPE,
    };

    enum ImageRotationMode {
        IMAGE_NoRotation = 0,
        IMAGE_ROTATE_LEFT,
        IMAGE_ROTATE_RIGHT,
        IMAGE_FLIP_VERTICAL,
        IMAGE_FLIP_HORIZONAL,
        IMAGE_ROTATE_RIGHTFLIPVERTICAL,
        IMAGE_ROTATE_RIGHTFLIPHORIZONTAL,
        IMAGE_ROTATE180,
    };

    struct GPUVector4 {
        float one;
        float two;
        float three;
        float four;
    };

    struct GPUVector3 {
        float one;
        float two;
        float three;
    };

    struct GPUVector2 {
        float one;
        float two;
    };

public:
    virtual ~ImageFilter();

    static void checkRenderLanguage();
    static ImageFilter::RenderLanguage getRenderLanguage() { return renderLanguage_; }

    const ImageFilterType& getImageFilterType() const { return imageFilterType_; }

    ImageFrame* read(const TextureFrame* textureFrame);

    static void* createTexture(const MSize& size, const PixelFormat& pixelFormat, const TextureUsage& textureUsage);
    static void destroyTexture(void* texture);

    static TextureFrame* createTextureFrame(const MSize& size, const PixelFormat& pixelFormat, const TextureUsage& textureUsage);

    static const float* textureCoordinatesForRotation(const ImageFilter::ImageRotationMode& rotationMode);

protected:
    ImageFilter(const ImageFilterType& imageFilterType);

    void setVertexContent(const std::string& value);
    void setFragmentContent(const std::string& value);

    bool createShader();

    ShaderContext* getShaderCtx() const { return shaderCtx_; }

protected:
    static RenderLanguage renderLanguage_;

    ShaderContext* shaderCtx_ = nullptr;

    ImageFilterType imageFilterType_{IMAGE_FILTER_NONE_TYPE};
};

}

#endif // IMAGEFILTER_H
