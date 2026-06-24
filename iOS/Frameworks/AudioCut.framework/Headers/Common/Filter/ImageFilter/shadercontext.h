#ifndef SHADERCONTEXT_H
#define SHADERCONTEXT_H

#ifndef STRINGIZE
#define STRINGIZE(x) #x
#endif
#ifndef STRINGIZE2
#define STRINGIZE2(x) STRINGIZE(x)
#endif
#ifndef SHADER_STRING
#define SHADER_STRING(text) STRINGIZE2(text)
#endif

#include "Format/msize.h"
#include "Format/pixelformat.h"
#include "Format/textureusage.h"
#include "matrix.h"

#include <string>

namespace MediaLibrary {

class TextureFrame;
class ImageFrame;

class ShaderContext
{
public:
    virtual ~ShaderContext();

    virtual TextureFrame* render(const TextureFrame* textureFrame, const MSize& surfaceSize, void* context);
    virtual TextureFrame* render(const ImageFrame* imageFrame);
    virtual TextureFrame* render(const ImageFrame* imageFrame, const PixelFormat& pixelFormat);

    virtual ImageFrame* read(const TextureFrame* textureFrame, const PixelFormat& pixelFormat);

    void setPixelFormat(const PixelFormat& pixelFormat) { pixelFormat_ = pixelFormat; }
    const PixelFormat& getPixelFormat() const { return pixelFormat_; }

    void setVertexContent(const std::string& vertexContent) { vertexContent_ = vertexContent; }
    void setFragmentContent(const std::string& fragmentContent) { fragmentContent_ = fragmentContent; }

    virtual bool createShader();

    virtual void setCommandBufferLabel(const string& value);

protected:
    ShaderContext();

protected:
    PixelFormat pixelFormat_;
    std::string vertexContent_;
    std::string fragmentContent_;
};

}

#endif // SHADERCONTEXT_H
