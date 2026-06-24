#ifndef OPENGLBLENDSHADERCONTEXT_H
#define OPENGLBLENDSHADERCONTEXT_H

#include "openglshadercontext.h"

namespace MediaLibrary {

class ImageBlendFilter;

class OpenGLBlendShaderContext : public OpenGLShaderContext
{
public:
    OpenGLBlendShaderContext(ImageBlendFilter* blendFilter);

    using OpenGLShaderContext::createShader;

    virtual bool createShader() override;

    void begin(TextureFrame* textureFrame, const MSize& size);
    void render(TextureFrame *textureFrame);
    void end();

private:
    ImageBlendFilter* blendFilter_{nullptr};
#ifndef CLOSE_NATIVE_CODE
    GLuint framebuffer{0};

    GLint projectionMatrixUniform;
    GLint modelViewMatrixUniform;
    GLint blendTypeUniform;
    GLint blendIntensityUniform;
    GLint alphaUniform;
#endif
};

}

#endif // OPENGLBLENDSHADERCONTEXT_H
