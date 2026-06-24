#include "openglfileshadercontext.h"
#include "Model/frame.h"
#include "Model/ffmpegframe.h"
#include "Model/imageframe.h"
#include "Model/textureframe.h"

#include <cassert>

namespace MediaLibrary {

#ifndef CLOSE_NATIVE_CODE
// Color Conversion Constants (YUV to RGB) including adjustment from 16-235/16-240 (video range)

// BT.601, which is the standard for SDTV.
static GLfloat colorConversion601Default[] = {
    1.164f,  1.164f, 1.164f,
    0.0f, -0.392f, 2.017f,
    1.596f, -0.813f,   0.0f,
};

// BT.601 full range (ref: http://www.equasys.de/colorconversion.html)
static GLfloat colorConversion601FullRangeDefault[] = {
    1.0f,    1.0f,    1.0f,
    0.0f,    -0.343f, 1.765f,
    1.4f,    -0.711f, 0.0f,
};

// BT.709, which is the standard for HDTV.
static GLfloat colorConversion709Default[] = {
    1.164f,  1.164f, 1.164f,
    0.0f, -0.213f, 2.112f,
    1.793f, -0.533f,   0.0f,
};

GLfloat *colorConversion_601 = colorConversion601Default;
GLfloat *colorConversion_601FullRange = colorConversion601FullRangeDefault;
GLfloat *colorConversion_709 = colorConversion709Default;
#endif

static const char* fileVertexShaderString = SHADER_STRING
(
attribute vec4 position;
attribute vec4 inputTextureCoordinate;

varying vec2 textureCoordinate;

uniform mat4 projectionMatrix;
uniform mat4 modelViewMatrix;

void main()
{
    gl_Position = projectionMatrix * modelViewMatrix * position;
    textureCoordinate = inputTextureCoordinate.xy;
}
);

static const char* fileFragmentShaderString = SHADER_STRING
(
varying highp vec2 textureCoordinate;

uniform lowp int type;

uniform sampler2D inputImageTexture;

uniform sampler2D luminanceTexture;
uniform sampler2D chrominanceTexture;
uniform sampler2D chrominanceUTexture;
uniform sampler2D chrominanceVTexture;
uniform mediump mat3 colorConversionMatrix;

uniform mediump float widthOffset;

highp vec4 sampleTexture(highp vec2 _textureCoordinate)
{
    highp vec4 textureColor = vec4(0.0);
    if (type == 0) {
        textureColor = texture2D(inputImageTexture, _textureCoordinate);
    } else if (type == 2) {
        mediump vec3 yuv;
        lowp vec3 rgb;

        yuv.x = texture2D(luminanceTexture, _textureCoordinate).r - (16.0/255.0);
        yuv.yz = texture2D(chrominanceTexture, _textureCoordinate).ra - vec2(0.5, 0.5);
        rgb = colorConversionMatrix * yuv;

        textureColor = vec4(rgb, 1);
    } else if (type == 3) {
        mediump vec3 yuv;
        lowp vec3 rgb;

        yuv.x = texture2D(luminanceTexture, _textureCoordinate).r;
        yuv.y = texture2D(chrominanceUTexture, _textureCoordinate).r - 0.5;
        yuv.z = texture2D(chrominanceVTexture, _textureCoordinate).r - 0.5;
        rgb.r = 1.164 * (yuv.x - 16.0/255.0) + 1.596 * yuv.z;
        rgb.g = 1.164 * (yuv.x - 16.0/255.0) - 0.813 * yuv.z - 0.391 * yuv.y;
        rgb.b = 1.164 * (yuv.x - 16.0/255.0) + 2.018 * yuv.y;

        textureColor = vec4(rgb, 1);
    } else if (type == 4) {
        mediump vec3 yuv;
        lowp vec3 rgb;

        yuv.x = texture2D(luminanceTexture, _textureCoordinate).r - 16.0/256.0;
        yuv.y = texture2D(chrominanceTexture, _textureCoordinate).r - 128.0/256.0;
        yuv.z = texture2D(chrominanceTexture, _textureCoordinate).a - 128.0/256.0;
        rgb = mat3(1,       1,          1,
                   0,       -0.39465,   2.03211,
                   1.13983, -0.58060,   0) * yuv;

        textureColor = vec4(rgb, 1);
    } else if (type == 5) {
        highp vec3 yuv_l = vec3(texture2D(luminanceTexture, _textureCoordinate).r, texture2D(chrominanceUTexture, _textureCoordinate).r, texture2D(chrominanceVTexture, _textureCoordinate).r);
        highp vec3 yuv_h = vec3(texture2D(luminanceTexture, _textureCoordinate).a, texture2D(chrominanceUTexture, _textureCoordinate).a, texture2D(chrominanceVTexture, _textureCoordinate).a);
        highp vec3 yuv = (yuv_l * 255.0 + yuv_h * 255.0 * 256.0) / (1023.0) - vec3(16.0 / 255.0, 0.5, 0.5);
        highp vec3 rgb;
        rgb.r = yuv.r + 1.596 * yuv.b;
        rgb.g = yuv.r - 0.813 * yuv.b - 0.391 * yuv.g;
        rgb.b = yuv.r + 2.018 * yuv.g;

        textureColor = vec4(rgb, 1);
    } else if (type == 6) {
        mediump vec3 yuv;
        lowp vec3 rgb;

        yuv.x = texture2D(luminanceTexture, _textureCoordinate).r;
        yuv.y = texture2D(chrominanceUTexture, _textureCoordinate).r - 0.5;
        yuv.z = texture2D(chrominanceVTexture, _textureCoordinate).r - 0.5;
        rgb.r = 1.164 * (yuv.x - 16.0/255.0) + 1.596 * yuv.z;
        rgb.g = 1.164 * (yuv.x - 16.0/255.0) - 0.813 * yuv.z - 0.391 * yuv.y;
        rgb.b = 1.164 * (yuv.x - 16.0/255.0) + 2.018 * yuv.y;

        textureColor = vec4(rgb, 1);
    } else if (type == 7) {
        mediump vec3 yuv;
        lowp vec3 rgb;

        textureColor = texture2D(inputImageTexture, _textureCoordinate);
        if ((_textureCoordinate.x - widthOffset * floor(_textureCoordinate.x / widthOffset)) < (widthOffset / 2.0)) {
            yuv.x = textureColor.r;
        } else {
            yuv.x = textureColor.a;
        }
        yuv.y = textureColor.g - 0.5;
        yuv.z = textureColor.b - 0.5;
        rgb.r = 1.164 * (yuv.x - 16.0/255.0) + 1.596 * yuv.z;
        rgb.g = 1.164 * (yuv.x - 16.0/255.0) - 0.813 * yuv.z - 0.391 * yuv.y;
        rgb.b = 1.164 * (yuv.x - 16.0/255.0) + 2.018 * yuv.y;

        textureColor = vec4(rgb, 1);
    } else {
        mediump vec3 yuv;
        lowp vec3 rgb;

        yuv.x = texture2D(luminanceTexture, _textureCoordinate).r;
        yuv.yz = texture2D(chrominanceTexture, _textureCoordinate).ra - vec2(0.5, 0.5);
        rgb = colorConversionMatrix * yuv;

        textureColor = vec4(rgb, 1);
    }
    textureColor = clamp(textureColor, 0.0, 1.0);
    return textureColor;
}

void main()
{
    lowp vec4 textureColor = sampleTexture(textureCoordinate);

    gl_FragColor = textureColor;
}
);

OpenGLFileShaderContext::OpenGLFileShaderContext(ImageFileFilter *fileFilter)
    : OpenGLShaderContext()
    , fileFilter_(fileFilter)
#ifdef IOS_OS
    , textureCache_(nullptr)
#endif
{
    string vertexContent = fileVertexShaderString;
    string fragmentContent = fileFragmentShaderString;
    setVertexContent(vertexContent);
    setFragmentContent(fragmentContent);
    createShader();

#ifndef CLOSE_NATIVE_CODE
    glUseProgram(shaderProgram_);

    projectionMatrixUniform_ = uniformIndex("projectionMatrix");
    modelViewMatrixUniform_ = uniformIndex("modelViewMatrix");

    typeUniform_ = uniformIndex("type");

    widthOffsetUniform_ = uniformIndex("widthOffset");
#endif
}

#ifdef IOS_OS
void OpenGLFileShaderContext::setTextureCache(void *textureCache)
{
    textureCache_ = textureCache;
}
#endif

TextureFrame *OpenGLFileShaderContext::render(Frame *frame, const MSize &size, const float *position, const float *textureCoordinate, const Matrix &projectionMatrix, const Matrix &modelViewMatrix)
{
#ifndef CLOSE_NATIVE_CODE
    glUseProgram(shaderProgram_);

    setMatrix4f(projectionMatrixUniform_, projectionMatrix.mData);
    setMatrix4f(modelViewMatrixUniform_, modelViewMatrix.mData);

    const int surfaceWidth = static_cast<int>(size.getWidth());
    const int surfaceHeight = static_cast<int>(size.getHeight());
    glViewport(0, 0, surfaceWidth, surfaceHeight);

    GLuint fileEffectTexture = 0;
    glGenTextures(1, &fileEffectTexture);
    glBindTexture(GL_TEXTURE_2D, fileEffectTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surfaceWidth, surfaceHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    GLuint framebuffer;
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fileEffectTexture, 0);

    textureIndex_ = 0;

    glClearColor(0.f, 0.f, 0.f, 0.f);
    glClear(GL_COLOR_BUFFER_BIT);

    glEnableVertexAttribArray(positionLocation_);
    if (position) {
        glBindBuffer(GL_ARRAY_BUFFER, positionVBO_);
        glVertexAttribPointer(positionLocation_, 2, GL_FLOAT, 0, 2 * sizeof(GL_FLOAT), nullptr);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(GL_FLOAT) * 2 * 4, position);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    } else {
        glBindBuffer(GL_ARRAY_BUFFER, defaultPositionVBO_);
        glVertexAttribPointer(positionLocation_, 2, GL_FLOAT, 0, 2 * sizeof(GL_FLOAT), nullptr);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    glEnableVertexAttribArray(textureCoordinateLocation_);
    if (textureCoordinate) {
        glBindBuffer(GL_ARRAY_BUFFER, textureCoordinateVBO_);
        glVertexAttribPointer(textureCoordinateLocation_, 2, GL_FLOAT, 0, 2 * sizeof(GL_FLOAT), nullptr);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(GL_FLOAT) * 2 * 4, textureCoordinate);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    } else {
        glBindBuffer(GL_ARRAY_BUFFER, defaultTextureCoordinateVBO_);
        glVertexAttribPointer(textureCoordinateLocation_, 2, GL_FLOAT, 0, 2 * sizeof(GL_FLOAT), nullptr);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    switch (frame->getFrameType()) {
    case Frame::FRAME_FFMPEG_TYPE: {
        const FFmpegFrame* ffmpegFrame = dynamic_cast<const FFmpegFrame*>(frame);
        renderSingle(ffmpegFrame);
    }
        break;
    case Frame::FRAME_TEXTURE_TYPE: {
        const TextureFrame* textureFrame = dynamic_cast<TextureFrame*>(frame);
        renderSingle(textureFrame);
    }
        break;
    case Frame::FRAME_IMAGE_TYPE: {
        const ImageFrame* imageFrame = dynamic_cast<ImageFrame*>(frame);
        renderSingle(imageFrame);
    }
        break;
    default:
        break;
    }

    glDisableVertexAttribArray(positionLocation_);
    glDisableVertexAttribArray(textureCoordinateLocation_);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glDeleteFramebuffers(1, &framebuffer);
    framebuffer = 0;

    TextureFrame* fileTextureFrame = new TextureFrame;
    fileTextureFrame->setTimeBase(TimeBase(1, SECOND_TO_MILLISECOND_UNIT));
    fileTextureFrame->setTime(frame->getTime());
    fileTextureFrame->setDuration(frame->getDuration());
    fileTextureFrame->setFrame(new GLuint(fileEffectTexture));
    fileTextureFrame->setSize(size);

    return fileTextureFrame;
#else
    (void)frame;(void)size;(void)position;(void)textureCoordinate;(void)projectionMatrix;(void)modelViewMatrix;
    return nullptr;
#endif
}

MediaLibrary::ImageFrame *MediaLibrary::OpenGLFileShaderContext::read(const MediaLibrary::TextureFrame *textureFrame, const MediaLibrary::PixelFormat &pixelFormat)
{
#ifndef CLOSE_NATIVE_CODE
    setInt(typeUniform_, 0);
    return OpenGLShaderContext::read(textureFrame, pixelFormat);
#else
    (void)textureFrame;(void)pixelFormat;
    return nullptr;
#endif
}

void OpenGLFileShaderContext::renderSingle(const FFmpegFrame *ffmepgFrame)
{
#ifndef CLOSE_NATIVE_CODE
    GLuint luminanceTexture = 0, chrominanceUTexture = 0, chrominanceVTexture = 0, chrominanceTexture = 0;
    GLuint colorTexture = 0;

    const AVFrame* avFrame = static_cast<AVFrame*>(ffmepgFrame->getFrame());
    const int width = avFrame->width;
    const int height = avFrame->height;

    switch (static_cast<AVPixelFormat>(avFrame->format)) {
    case AV_PIX_FMT_RGBA:
    {
        GLuint rgbaTexture = 0;
        glGenTextures(1, &rgbaTexture);
        GLenum rgbaActiveTexture = GL_TEXTURE0 + textureIndex_;
        textureIndex_++;
        glActiveTexture(rgbaActiveTexture);
        glBindTexture(GL_TEXTURE_2D, rgbaTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, avFrame->data[0]);

        setInt(typeUniform_, 0);
        glActiveTexture(rgbaActiveTexture);
        glBindTexture(GL_TEXTURE_2D, rgbaTexture);
        GLint inputImageTextureLocation = glGetUniformLocation(shaderProgram_, "inputImageTexture");
        assert(inputImageTextureLocation >= 0);
        glUniform1i(inputImageTextureLocation, rgbaActiveTexture - GL_TEXTURE0);

        colorTexture = rgbaTexture;
    }
        break;
    case AV_PIX_FMT_BGRA:
    {
        GLuint bgraTexture = 0;
        glGenTextures(1, &bgraTexture);
        GLenum bgraActiveTexture = GL_TEXTURE0 + textureIndex_;
        textureIndex_++;
        glActiveTexture(bgraActiveTexture);
        glBindTexture(GL_TEXTURE_2D, bgraTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        GLenum format;
        GLint internalformat;
#ifdef IOS_OS
        format = GL_BGRA;
        internalformat = GL_RGBA;
#elif defined (ANDROID_OS)
        //Note, In Android, GL_BGRA_EXT is OpenGLES extension feature, may not be supported by some devices.
        //      see https://www.g-truc.net/post-0734.html for GL_BGRA_EXT.
        format = GL_BGRA_EXT;
        internalformat = GL_BGRA_EXT;
#endif
        const int bytesPerPixel = 4;
        glPixelStorei(GL_UNPACK_ROW_LENGTH, avFrame->linesize[0] / bytesPerPixel);
        glTexImage2D(GL_TEXTURE_2D, 0, internalformat, width, height, 0, format, GL_UNSIGNED_BYTE, avFrame->data[0]);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

        setInt(typeUniform_, 0);
        glActiveTexture(bgraActiveTexture);
        glBindTexture(GL_TEXTURE_2D, bgraTexture);
        GLint inputImageTextureLocation = glGetUniformLocation(shaderProgram_, "inputImageTexture");
        assert(inputImageTextureLocation >= 0);
        glUniform1i(inputImageTextureLocation, bgraActiveTexture - GL_TEXTURE0);

        colorTexture = bgraTexture;
    }
        break;
    case AV_PIX_FMT_RGB24:
    {
        GLuint rgb24Texture = 0;
        glGenTextures(1, &rgb24Texture);
        GLenum rgb24ActiveTexture = GL_TEXTURE0 + textureIndex_;
        textureIndex_++;
        glActiveTexture(rgb24ActiveTexture);
        glBindTexture(GL_TEXTURE_2D, rgb24Texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, avFrame->data[0]);

        setInt(typeUniform_, 0);
        glActiveTexture(rgb24ActiveTexture);
        glBindTexture(GL_TEXTURE_2D, rgb24Texture);
        GLint inputImageTextureLocation = glGetUniformLocation(shaderProgram_, "inputImageTexture");
        assert(inputImageTextureLocation >= 0);
        glUniform1i(inputImageTextureLocation, rgb24ActiveTexture - GL_TEXTURE0);

        colorTexture = rgb24Texture;
    }
        break;
    case AV_PIX_FMT_YUV420P:
    {
        glGenTextures(1, &luminanceTexture);
        GLenum luminanceActiveTexture = GL_TEXTURE0 + textureIndex_;
        textureIndex_++;
        glActiveTexture(luminanceActiveTexture);
        glBindTexture(GL_TEXTURE_2D, luminanceTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glGenTextures(1, &chrominanceUTexture);
        GLenum chrominanceUActiveTexture = GL_TEXTURE0 + textureIndex_;
        textureIndex_++;
        glActiveTexture(chrominanceUActiveTexture);
        glBindTexture(GL_TEXTURE_2D, chrominanceUTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glGenTextures(1, &chrominanceVTexture);
        GLenum chrominanceVActiveTexture = GL_TEXTURE0 + textureIndex_;
        textureIndex_++;
        glActiveTexture(chrominanceVActiveTexture);
        glBindTexture(GL_TEXTURE_2D, chrominanceVTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        setInt(typeUniform_, 3);
        glActiveTexture(luminanceActiveTexture);
        glBindTexture(GL_TEXTURE_2D, luminanceTexture);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, avFrame->linesize[0]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width, height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, avFrame->data[0]);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        GLint location = glGetUniformLocation(shaderProgram_, "luminanceTexture");
        assert(location >= 0);
        glUniform1i(location, luminanceActiveTexture - GL_TEXTURE0);

        const int halfWidth = width >> 1;
        const int halfHeight = height >> 1;
        glActiveTexture(chrominanceUActiveTexture);
        glBindTexture(GL_TEXTURE_2D, chrominanceUTexture);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, avFrame->linesize[1]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, halfWidth, halfHeight, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, avFrame->data[1]);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        location = glGetUniformLocation(shaderProgram_, "chrominanceUTexture");
        assert(location >= 0);
        glUniform1i(location, chrominanceUActiveTexture - GL_TEXTURE0);

        glActiveTexture(chrominanceVActiveTexture);
        glBindTexture(GL_TEXTURE_2D, chrominanceVTexture);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, avFrame->linesize[2]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, halfWidth, halfHeight, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, avFrame->data[2]);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        location = glGetUniformLocation(shaderProgram_, "chrominanceVTexture");
        assert(location >= 0);
        glUniform1i(location, chrominanceVActiveTexture - GL_TEXTURE0);
    }
        break;
    case AV_PIX_FMT_NV12:
    {
        glGenTextures(1, &luminanceTexture);
        GLenum luminanceActiveTexture = GL_TEXTURE0 + textureIndex_;
        textureIndex_++;
        glActiveTexture(luminanceActiveTexture);
        glBindTexture(GL_TEXTURE_2D, luminanceTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glGenTextures(1, &chrominanceTexture);
        GLenum chrominanceActiveTexture = GL_TEXTURE0 + textureIndex_;
        textureIndex_++;
        glActiveTexture(chrominanceActiveTexture);
        glBindTexture(GL_TEXTURE_2D, chrominanceTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        setInt(typeUniform_, 4);
        glActiveTexture(luminanceActiveTexture);
        glBindTexture(GL_TEXTURE_2D, luminanceTexture);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, avFrame->linesize[0]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width, height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, avFrame->data[0]);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        GLint location = glGetUniformLocation(shaderProgram_, "luminanceTexture");
        assert(location >= 0);
        glUniform1i(location, luminanceActiveTexture - GL_TEXTURE0);

        const int halfWidth = width >> 1;
        const int halfHeight = height >> 1;
        glActiveTexture(chrominanceActiveTexture);
        glBindTexture(GL_TEXTURE_2D, chrominanceTexture);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, avFrame->linesize[1] / 2);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA, halfWidth, halfHeight, 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, avFrame->data[1]);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        location = glGetUniformLocation(shaderProgram_, "chrominanceTexture");
        assert(location >= 0);
        glUniform1i(location, chrominanceActiveTexture - GL_TEXTURE0);
    }
        break;
    case AV_PIX_FMT_YUV420P10LE:
    {
        glGenTextures(1, &luminanceTexture);
        GLenum luminanceActiveTexture = GL_TEXTURE0 + textureIndex_;
        textureIndex_++;
        glActiveTexture(luminanceActiveTexture);
        glBindTexture(GL_TEXTURE_2D, luminanceTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glGenTextures(1, &chrominanceUTexture);
        GLenum chrominanceUActiveTexture = GL_TEXTURE0 + textureIndex_;
        textureIndex_++;
        glActiveTexture(chrominanceUActiveTexture);
        glBindTexture(GL_TEXTURE_2D, chrominanceUTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glGenTextures(1, &chrominanceVTexture);
        GLenum chrominanceVActiveTexture = GL_TEXTURE0 + textureIndex_;
        textureIndex_++;
        glActiveTexture(chrominanceVActiveTexture);
        glBindTexture(GL_TEXTURE_2D, chrominanceVTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        setInt(typeUniform_, 5);
        glActiveTexture(luminanceActiveTexture);
        glBindTexture(GL_TEXTURE_2D, luminanceTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA, width, height, 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, avFrame->data[0]);
        GLint location = glGetUniformLocation(shaderProgram_, "luminanceTexture");
        assert(location >= 0);
        glUniform1i(location, luminanceActiveTexture - GL_TEXTURE0);

        const int halfWidth = width >> 1;
        const int halfHeight = height >> 1;
        glActiveTexture(chrominanceUActiveTexture);
        glBindTexture(GL_TEXTURE_2D, chrominanceUTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA, halfWidth, halfHeight, 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, avFrame->data[1]);
        location = glGetUniformLocation(shaderProgram_, "chrominanceUTexture");
        assert(location >= 0);
        glUniform1i(location, chrominanceUActiveTexture - GL_TEXTURE0);

        glActiveTexture(chrominanceVActiveTexture);
        glBindTexture(GL_TEXTURE_2D, chrominanceVTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA, halfWidth, halfHeight, 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, avFrame->data[2]);
        location = glGetUniformLocation(shaderProgram_, "chrominanceVTexture");
        assert(location >= 0);
        glUniform1i(location, chrominanceVActiveTexture - GL_TEXTURE0);
    }
        break;
    default:
        assert(0);
        break;
    }

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glBindTexture(GL_TEXTURE_2D, 0);

    if (colorTexture) {
        glDeleteTextures(1, &colorTexture);
        colorTexture = 0;
    }

    if (luminanceTexture) {
        glDeleteTextures(1, &luminanceTexture);
        luminanceTexture = 0;
    }

    if (chrominanceUTexture) {
        glDeleteTextures(1, &chrominanceUTexture);
        chrominanceUTexture = 0;
    }

    if (chrominanceVTexture) {
        glDeleteTextures(1, &chrominanceVTexture);
        chrominanceVTexture = 0;
    }

    if (chrominanceTexture) {
        glDeleteTextures(1, &chrominanceTexture);
        chrominanceTexture = 0;
    }
#endif
}

void OpenGLFileShaderContext::renderSingle(const TextureFrame *textureFrame)
{
#ifndef CLOSE_NATIVE_CODE
    const GLuint texture = *static_cast<const GLuint*>(textureFrame->getFrame());

    setInt(typeUniform_, 0);
    GLenum activeTexture = GL_TEXTURE0 + textureIndex_;
    textureIndex_++;
    glActiveTexture(activeTexture);
    glBindTexture(GL_TEXTURE_2D, texture);
    GLint inputImageTextureLocation = glGetUniformLocation(shaderProgram_, "inputImageTexture");
    assert(inputImageTextureLocation >= 0);
    glUniform1i(inputImageTextureLocation, activeTexture - GL_TEXTURE0);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glBindTexture(GL_TEXTURE_2D, 0);
#endif
}

void OpenGLFileShaderContext::renderSingle(const ImageFrame *imageFrame)
{
#ifndef CLOSE_NATIVE_CODE
    GLuint texture = 0;
    glGenTextures(1, &texture);
    GLenum activeTexture = GL_TEXTURE0 + textureIndex_;
    textureIndex_++;
    glActiveTexture(activeTexture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    const MSize size = imageFrame->getSize();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.getWidth(), size.getHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE, imageFrame->getFrame());

    setInt(typeUniform_, 0);
    glActiveTexture(activeTexture);
    glBindTexture(GL_TEXTURE_2D, texture);
    GLint inputImageTextureLocation = glGetUniformLocation(shaderProgram_, "inputImageTexture");
    assert(inputImageTextureLocation >= 0);
    glUniform1i(inputImageTextureLocation, activeTexture - GL_TEXTURE0);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glBindTexture(GL_TEXTURE_2D, 0);

    glDeleteTextures(1, &texture);
    texture = 0;
#endif
}

}
