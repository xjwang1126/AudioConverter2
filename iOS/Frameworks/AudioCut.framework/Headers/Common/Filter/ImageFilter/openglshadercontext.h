#ifndef OPENGLSHADERCONTEXT_H
#define OPENGLSHADERCONTEXT_H

#include "shadercontext.h"
#include "imagefilter.h"

#ifndef CLOSE_NATIVE_CODE
#ifdef IOS_OS
#include <OPENGLES/ES3/gl.h>
#include <OpenGLES/ES3/glext.h>
#elif defined(ANDROID_OS)
#include <GLES3/gl3.h>
#include <GLES2/gl2ext.h>
#endif
#endif

namespace MediaLibrary {

extern const char* _vertexShaderString;
extern const char* _fragmentShaderString;

const float defaultPosition[] = {
    -1.0f, -1.0f,
     1.0f, -1.0f,
    -1.0f,  1.0f,
     1.0f,  1.0f,
};

const float defaultTextureCoordinate[] = {
    0.0f, 0.0f,
    1.0f, 0.0f,
    0.0f, 1.0f,
    1.0f, 1.0f,
};

class OpenGLShaderContext : public ShaderContext
{
public:
    OpenGLShaderContext();

    virtual ImageFrame* read(const TextureFrame* textureFrame, const PixelFormat& pixelFormat) override;

    static void* createTexture(const MSize& size, const PixelFormat& pixelFormat, const TextureUsage& textureUsage);
    static void destroyTexture(void* texture);

    virtual bool createShader() override;

    static const float* textureCoordinatesForRotation(const ImageFilter::ImageRotationMode& rotationMode);

protected:
#ifndef CLOSE_NATIVE_CODE
    GLuint createShader(const GLenum& shaderType, const string& shaderContent);
#endif

    int uniformIndex(const string& uniform);
    int uniformIndex(const unsigned int programe, const string& uniform);

    void setInt(const int index, const int value);
    void setInt(const unsigned int program, const int index, const int value);
    void setFloat(const int index, const float value);
    void setFloat(const unsigned int program, const int index, const float value);
    void setVec2(const int index, const ImageFilter::GPUVector2& value);
    void setVec2(const unsigned int program, const int index, const ImageFilter::GPUVector2& value);
    void setVec3(const int index, const ImageFilter::GPUVector3& value);
    void setVec3(const unsigned int program, const int index, const ImageFilter::GPUVector3& value);
    void setVec4(const int index, const ImageFilter::GPUVector4& value);
    void setVec4(const unsigned int program, const int index, const ImageFilter::GPUVector4& value);
    void setMatrix3f(const int index, const float* value);
    void setMatrix3f(const unsigned int program, const int index, const float* value);
    void setMatrix4f(const int index, const float* value);
    void setMatrix4f(const unsigned int program, const int index, const float* value);

#ifndef CLOSE_NATIVE_CODE
    GLuint createPBO(const uint32_t dataSize);
    void destroyPBO(const GLuint PBO);
#endif

protected:
#ifndef CLOSE_NATIVE_CODE
    GLuint shaderProgram_;

    GLuint positionVBO_;
    GLuint defaultPositionVBO_;
    GLuint positionLocation_;

    GLuint textureCoordinateVBO_;
    GLuint defaultTextureCoordinateVBO_;
    GLuint textureCoordinateLocation_;

    GLint imageTextureUniform_;

    GLint imageHelpTextureUniform_;

    GLuint imagePBO_{0};
#endif
};

}

#endif // OPENGLSHADERCONTEXT_H
