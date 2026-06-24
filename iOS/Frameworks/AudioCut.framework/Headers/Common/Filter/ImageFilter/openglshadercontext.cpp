#include "openglshadercontext.h"
#include "Render/rendercontext.h"
#include "Model/textureframe.h"
#include "Model/imageframe.h"

#include <cassert>

namespace MediaLibrary {

OpenGLShaderContext::OpenGLShaderContext()
{
}

ImageFrame *OpenGLShaderContext::read(const TextureFrame *textureFrame, const PixelFormat &pixelFormat)
{
#ifndef CLOSE_NATIVE_CODE
    ImageFrame* imageFrame = nullptr;

    const GLuint texture = *static_cast<GLuint*>(textureFrame->getFrame());
    const MSize size = textureFrame->getSize();
    const int width = size.getWidth();
    const int height = size.getHeight();

    GLuint framebuffer;
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

    uint8_t* data = nullptr;
    switch (pixelFormat) {
    case PIXEL_FORMAT_RGBA:
    {
        const int dataSize = width * height * 4;
        data = new uint8_t[dataSize];
        glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);
    }
        break;
    case PIXEL_FORMAT_RGB:
    {
        const int dataSize = width * height * 3;
        data = new uint8_t[dataSize];
        glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, data);
    }
        break;
    default:
        assert(0);
        break;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glDeleteFramebuffers(1, &framebuffer);
    framebuffer = 0;

    imageFrame = new ImageFrame;
    imageFrame->setTimeBase(TimeBase(1, SECOND_TO_MILLISECOND_UNIT));
    imageFrame->setTime(textureFrame->getTime());
    imageFrame->setDuration(textureFrame->getDuration());
    imageFrame->setFrame(data);
    imageFrame->setSize(size);

    return imageFrame;
#else
    (void)textureFrame;(void)pixelFormat;
    return nullptr;
#endif
}

void *OpenGLShaderContext::createTexture(const MSize &size, const PixelFormat &pixelFormat, const TextureUsage &textureUsage)
{
#ifndef CLOSE_NATIVE_CODE
    (void)pixelFormat;(void)textureUsage;
    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    switch (pixelFormat) {
    case PIXEL_FORMAT_RGBA: {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, static_cast<int>(size.getWidth()), static_cast<int>(size.getHeight()), 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    }
        break;
    case PIXEL_FORMAT_BGRA: {
        assert(0);
    }
        break;
    case PIXEL_FORMAT_ALPHA: {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, static_cast<int>(size.getWidth()), static_cast<int>(size.getHeight()), 0, GL_ALPHA, GL_UNSIGNED_BYTE, nullptr);
    }
        break;
    default:
        assert(0);
        break;
    }

    GLuint* renderTexture = new GLuint(texture);
    return renderTexture;
#else
    (void)size;(void)pixelFormat;(void)textureUsage;
    return nullptr;
#endif
}

void OpenGLShaderContext::destroyTexture(void *texture)
{
#ifndef CLOSE_NATIVE_CODE
    if (!texture) {
        return;
    }
    GLuint* texturePointer = static_cast<GLuint*>(texture);
    if (!texturePointer) {
        return;
    }
    GLuint textureId = *texturePointer;
    if (!textureId) {
        return;
    }
    RenderContext renderCtx;
    if (!renderCtx.checkRenderContext()) {
        assert(0);
    }
    glDeleteTextures(1, reinterpret_cast<GLuint*>(&textureId));
    textureId = 0;

    delete texturePointer;
    texturePointer = nullptr;
#else
    (void)texture;
#endif
}

bool OpenGLShaderContext::createShader()
{
#ifndef CLOSE_NATIVE_CODE
    GLuint vertexShader = 0, fragmentShader = 0, _shaderProgram = 0;

    do {
        vertexShader = createShader(GL_VERTEX_SHADER, vertexContent_.c_str());
        if (!vertexShader) {
            break;
        }

        fragmentShader = createShader(GL_FRAGMENT_SHADER, fragmentContent_.c_str());
        if (!fragmentShader) {
            break;
        }

        _shaderProgram  = glCreateProgram();
        glAttachShader(_shaderProgram, vertexShader);
        glAttachShader(_shaderProgram, fragmentShader);

        glBindAttribLocation(_shaderProgram, static_cast<GLuint>(glGetAttribLocation(_shaderProgram, "position")), "position");
        glBindAttribLocation(_shaderProgram, static_cast<GLuint>(glGetAttribLocation(_shaderProgram, "inputTextureCoordinate")), "inputTextureCoordinate");

        glLinkProgram(_shaderProgram);

        if (vertexShader) {
            glDetachShader(_shaderProgram, vertexShader);
            glDeleteShader(vertexShader);
            vertexShader = 0;
        }

        if (fragmentShader) {
            glDetachShader(_shaderProgram, fragmentShader);
            glDeleteShader(fragmentShader);
            fragmentShader = 0;
        }

#if !defined(QT_NO_DEBUG)
        GLint result = GL_FALSE;
        glGetProgramiv(_shaderProgram, GL_LINK_STATUS, &result);
        if (result == GL_FALSE) {
            char log[1024];
            int len = 0;
            glGetProgramInfoLog(_shaderProgram, sizeof(log), &len, log);
            break;
        }
#endif

        GLint _positionLocation = glGetAttribLocation(_shaderProgram, "position");
        assert(_positionLocation >= 0);
        GLuint __positionLocation = static_cast<GLuint>(_positionLocation);
        GLint _inputTextureCoordinateLocation = glGetAttribLocation(_shaderProgram, "inputTextureCoordinate");
        assert(_inputTextureCoordinateLocation >= 0);
        GLuint __inputTextureCoordinateLocation = static_cast<GLuint>(_inputTextureCoordinateLocation);
        GLint _inputImageTextureUniform = glGetUniformLocation(_shaderProgram, "inputImageTexture");
        GLint _inputImageHelpTextureUniform = glGetUniformLocation(_shaderProgram, "inputImageHelpTexture");

        shaderProgram_ = _shaderProgram;
        positionLocation_ = __positionLocation;
        textureCoordinateLocation_ = __inputTextureCoordinateLocation;
        imageTextureUniform_ = _inputImageTextureUniform;
        imageHelpTextureUniform_ = _inputImageHelpTextureUniform;

        glGenBuffers(1, &defaultPositionVBO_);
        glBindBuffer(GL_ARRAY_BUFFER, defaultPositionVBO_);
        glBufferData(GL_ARRAY_BUFFER, sizeof(defaultPosition), defaultPosition, GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glGenBuffers(1, &defaultTextureCoordinateVBO_);
        glBindBuffer(GL_ARRAY_BUFFER, defaultTextureCoordinateVBO_);
        glBufferData(GL_ARRAY_BUFFER, sizeof(defaultTextureCoordinate), defaultTextureCoordinate, GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glGenBuffers(1, &positionVBO_);
        glBindBuffer(GL_ARRAY_BUFFER, positionVBO_);
        glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 2 * 4, nullptr, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glGenBuffers(1, &textureCoordinateVBO_);
        glBindBuffer(GL_ARRAY_BUFFER, textureCoordinateVBO_);
        glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 2 * 4, nullptr, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        return true;
    } while (false);

    if (vertexShader) {
        if (_shaderProgram) {
            glDetachShader(_shaderProgram, vertexShader);
        }
        glDeleteShader(vertexShader);
    }

    if (fragmentShader) {
        if (_shaderProgram) {
            glDetachShader(_shaderProgram, fragmentShader);
        }
        glDeleteShader(fragmentShader);
    }

    if (_shaderProgram) {
        glDeleteProgram(_shaderProgram);
    }

    assert(0);

    return false;
#else
    return false;
#endif
}

const float *OpenGLShaderContext::textureCoordinatesForRotation(const ImageFilter::ImageRotationMode &rotationMode)
{
#ifndef CLOSE_NATIVE_CODE
    static const GLfloat noRotationTextureCoordinates[] = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        0.0f, 1.0f,
        1.0f, 1.0f,
    };

    static const GLfloat rotateLeftTextureCoordinates[] = {
        1.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 0.0f,
        0.0f, 1.0f,
    };

    static const GLfloat rotateRightTextureCoordinates[] = {
        0.0f, 1.0f,
        0.0f, 0.0f,
        1.0f, 1.0f,
        1.0f, 0.0f,
    };

    static const GLfloat verticalFlipTextureCoordinates[] = {
        0.0f, 1.0f,
        1.0f, 1.0f,
        0.0f,  0.0f,
        1.0f,  0.0f,
    };

    static const GLfloat horizontalFlipTextureCoordinates[] = {
        1.0f, 0.0f,
        0.0f, 0.0f,
        1.0f,  1.0f,
        0.0f,  1.0f,
    };

    static const GLfloat rotateRightVerticalFlipTextureCoordinates[] = {
        0.0f, 0.0f,
        0.0f, 1.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
    };

    static const GLfloat rotateRightHorizontalFlipTextureCoordinates[] = {
        1.0f, 1.0f,
        1.0f, 0.0f,
        0.0f, 1.0f,
        0.0f, 0.0f,
    };

    static const GLfloat rotate180TextureCoordinates[] = {
        1.0f, 1.0f,
        0.0f, 1.0f,
        1.0f, 0.0f,
        0.0f, 0.0f,
    };

    switch(rotationMode)
    {
        case ImageFilter::IMAGE_NoRotation: return noRotationTextureCoordinates;
        case ImageFilter::IMAGE_ROTATE_LEFT: return rotateLeftTextureCoordinates;
        case ImageFilter::IMAGE_ROTATE_RIGHT: return rotateRightTextureCoordinates;
        case ImageFilter::IMAGE_FLIP_VERTICAL: return verticalFlipTextureCoordinates;
        case ImageFilter::IMAGE_FLIP_HORIZONAL: return horizontalFlipTextureCoordinates;
        case ImageFilter::IMAGE_ROTATE_RIGHTFLIPVERTICAL: return rotateRightVerticalFlipTextureCoordinates;
        case ImageFilter::IMAGE_ROTATE_RIGHTFLIPHORIZONTAL: return rotateRightHorizontalFlipTextureCoordinates;
        case ImageFilter::IMAGE_ROTATE180: return rotate180TextureCoordinates;
    }
    return nullptr;
#else
    (void)rotationMode;
    return nullptr;
#endif
}

#ifndef CLOSE_NATIVE_CODE
GLuint OpenGLShaderContext::createShader(const GLenum &shaderType, const string &shaderContent)
{
    GLuint shader = 0;

    shader = glCreateShader(shaderType);
    const char* _shaderContent = shaderContent.c_str();
    glShaderSource(shader, 1, &_shaderContent, nullptr);
    glCompileShader(shader);

#if !defined (QT_NO_DEBUG)
    GLint result = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &result);
    if (!result) {
        char log[1024];
        glGetShaderInfoLog(shader, sizeof (log), nullptr, log);
        return 0;
    }
#endif

    return shader;
}
#endif

int OpenGLShaderContext::uniformIndex(const string &uniform)
{
#ifndef CLOSE_NATIVE_CODE
    return glGetUniformLocation(shaderProgram_, uniform.c_str());
#else
    (void)uniform;
    return -1;
#endif
}

int OpenGLShaderContext::uniformIndex(const unsigned int programe, const string &uniform)
{
#ifndef CLOSE_NATIVE_CODE
    glUseProgram(programe);
    return glGetUniformLocation(programe, uniform.c_str());
#else
    (void)programe;(void)uniform;
    return -1;
#endif
}

void OpenGLShaderContext::setInt(const int index, const int value)
{
#ifndef CLOSE_NATIVE_CODE
    glUniform1i(index, value);
#else
    (void)index;(void)value;
#endif
}

void OpenGLShaderContext::setInt(const unsigned int program, const int index, const int value)
{
#ifndef CLOSE_NATIVE_CODE
    glUseProgram(program);
    glUniform1i(index, value);
#else
    (void)program;(void)index;(void)value;
#endif
}

void OpenGLShaderContext::setFloat(const int index, const float value)
{
#ifndef CLOSE_NATIVE_CODE
    glUniform1f(index, value);
#else
    (void)index;(void)value;
#endif
}

void OpenGLShaderContext::setFloat(const unsigned int program, const int index, const float value)
{
#ifndef CLOSE_NATIVE_CODE
    glUseProgram(program);
    glUniform1f(index, value);
#else
    (void)program;(void)index;(void)value;
#endif
}

void OpenGLShaderContext::setVec2(const int index, const ImageFilter::GPUVector2 &value)
{
#ifndef CLOSE_NATIVE_CODE
    glUniform2fv(index, 1, reinterpret_cast<GLfloat*>(const_cast<ImageFilter::GPUVector2*>(&value)));
#else
    (void)index;(void)value;
#endif
}

void OpenGLShaderContext::setVec2(const unsigned int program, const int index, const ImageFilter::GPUVector2 &value)
{
#ifndef CLOSE_NATIVE_CODE
    glUseProgram(program);
    glUniform2fv(index, 1, reinterpret_cast<GLfloat*>(const_cast<ImageFilter::GPUVector2*>(&value)));
#else
    (void)program;(void)index;(void)value;
#endif
}

void OpenGLShaderContext::setVec3(const int index, const ImageFilter::GPUVector3 &value)
{
#ifndef CLOSE_NATIVE_CODE
    glUniform3fv(index, 1, reinterpret_cast<GLfloat*>(const_cast<ImageFilter::GPUVector3*>(&value)));
#else
    (void)index;(void)value;
#endif
}

void OpenGLShaderContext::setVec3(const unsigned int program, const int index, const ImageFilter::GPUVector3 &value)
{
#ifndef CLOSE_NATIVE_CODE
    glUseProgram(program);
    glUniform3fv(index, 1, reinterpret_cast<GLfloat*>(const_cast<ImageFilter::GPUVector3*>(&value)));
#else
    (void)program;(void)index;(void)value;
#endif
}

void OpenGLShaderContext::setVec4(const int index, const ImageFilter::GPUVector4 &value)
{
#ifndef CLOSE_NATIVE_CODE
    glUniform4fv(index, 1, reinterpret_cast<GLfloat*>(const_cast<ImageFilter::GPUVector4*>(&value)));
#else
    (void)index;(void)value;
#endif
}

void OpenGLShaderContext::setVec4(const unsigned int program, const int index, const ImageFilter::GPUVector4 &value)
{
#ifndef CLOSE_NATIVE_CODE
    glUseProgram(program);
    glUniform4fv(index, 1, reinterpret_cast<GLfloat*>(const_cast<ImageFilter::GPUVector4*>(&value)));
#else
    (void)program;(void)index;(void)value;
#endif
}

void OpenGLShaderContext::setMatrix3f(const int index, const float *value)
{
#ifndef CLOSE_NATIVE_CODE
    glUniformMatrix3fv(index, 1, GL_FALSE, value);
#else
    (void)index;(void)value;
#endif
}

void OpenGLShaderContext::setMatrix3f(const unsigned int program, const int index, const float *value)
{
#ifndef CLOSE_NATIVE_CODE
    glUseProgram(program);
    glUniformMatrix3fv(index, 1, GL_FALSE, value);
#else
    (void)program;(void)index;(void)value;
#endif
}

void OpenGLShaderContext::setMatrix4f(const int index, const float *value)
{
#ifndef CLOSE_NATIVE_CODE
    glUniformMatrix4fv(index, 1, GL_FALSE, value);
#else
    (void)index;(void)value;
#endif
}

void OpenGLShaderContext::setMatrix4f(const unsigned int program, const int index, const float *value)
{
#ifndef CLOSE_NATIVE_CODE
    glUseProgram(program);
    glUniformMatrix4fv(index, 1, GL_FALSE, value);
#else
    (void)program;(void)index;(void)value;
#endif
}

#ifndef CLOSE_NATIVE_CODE
GLuint OpenGLShaderContext::createPBO(const uint32_t dataSize)
{
    if (!dataSize) {
        return 0;
    }
    GLuint PBO{0};
    glGenBuffers(1, &PBO);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, PBO);
    glBufferData(GL_PIXEL_UNPACK_BUFFER, dataSize, nullptr, GL_STREAM_DRAW);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    return PBO;
}

void OpenGLShaderContext::destroyPBO(const GLuint PBO)
{
    if (!PBO) {
        return;
    }
    glDeleteBuffers(1, &PBO);
}
#endif

}
