#ifndef OPENGLFILESHADERCONTEXT_H
#define OPENGLFILESHADERCONTEXT_H

#include "openglshadercontext.h"

namespace MediaLibrary {

class Frame;
class ImageFrame;
class TextureFrame;
class FFmpegFrame;
class NativeFrame;
class ImageFileFilter;

class OpenGLFileShaderContext : public OpenGLShaderContext
{
public:
    OpenGLFileShaderContext(ImageFileFilter* fileFilter);

#ifdef IOS_OS
    void setTextureCache(void* textureCache);
#endif

    TextureFrame* render(Frame* frame, const MSize& size,
                         const float* position, const float* textureCoordinate,
                         const Matrix& projectionMatrix, const Matrix& modelViewMatrix);

    virtual ImageFrame* read(const TextureFrame* textureFrame, const PixelFormat& pixelFormat) override;

private:
    void renderSingle(const FFmpegFrame* ffmepgFrame);
    void renderSingle(const NativeFrame* nativeFrame);
    void renderSingle(const TextureFrame* textureFrame);
    void renderSingle(const ImageFrame* imageFrame);

private:
    ImageFileFilter* fileFilter_{nullptr};

#ifdef IOS_OS
    void* textureCache_;
#endif

#ifndef CLOSE_NATIVE_CODE
    GLint projectionMatrixUniform_;
    GLint modelViewMatrixUniform_;
    GLint typeUniform_;

    GLint widthOffsetUniform_;

    GLint textureIndex_{0};
#endif
};

}

#endif // OPENGLFILESHADERCONTEXT_H
