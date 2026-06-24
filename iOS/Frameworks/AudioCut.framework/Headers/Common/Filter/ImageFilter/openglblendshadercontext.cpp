#include "openglblendshadercontext.h"
#include "Render/rendercapability.h"
#include "Model/textureframe.h"
#include "Filter/ImageFilter/imageblendfilter.h"

#include <cassert>

namespace MediaLibrary {

static const char* blendVertexShaderString = SHADER_STRING
(
attribute vec4 position;
attribute vec4 inputTextureCoordinate;

uniform mat4 projectionMatrix;
uniform mat4 modelViewMatrix;

varying vec2 textureCoordinate;

void main()
{
    gl_Position = projectionMatrix * modelViewMatrix * position;
    textureCoordinate = inputTextureCoordinate.xy;
}
);

OpenGLBlendShaderContext::OpenGLBlendShaderContext(ImageBlendFilter *blendFilter)
    : OpenGLShaderContext()
    , blendFilter_(blendFilter)
{
    RenderCapability* renderCapability = RenderCapability::getInstance();
    bool supportGL_EXT_shader_framebuffer_fetch = renderCapability->isSupportGL_EXT_shader_framebuffer_fetch();
    bool supportGL_ARM_shader_framebuffer_fetch = renderCapability->isSupportGL_ARM_shader_framebuffer_fetch();

    const char* blendFragmentShaderString = nullptr;
    if (supportGL_EXT_shader_framebuffer_fetch) {
        blendFragmentShaderString =
            "#extension GL_EXT_shader_framebuffer_fetch : require\n"
            "varying highp vec2 textureCoordinate;"
            "uniform sampler2D inputImageTexture;"
            "uniform lowp float alpha;"
            "uniform lowp float intensity;"
            "uniform lowp int blendType;"
            "lowp float softlight(lowp float base, lowp float blend)"
            "{"
            "   return (blend<0.5)?(2.0*base*blend+base*base*(1.0-2.0*blend)):(sqrt(base)*(2.0*blend-1.0)+2.0*base*(1.0-blend));"
            "}"
            "lowp float overlay(lowp float base, lowp float blend)"
            "{"
            "   return base<0.5?(2.0*base*blend):(1.0-2.0*(1.0-base)*(1.0-blend));"
            "} "
            "lowp float multiply(lowp float base, lowp float blend)"
            "{"
            "   return base*blend;"
            "}"
            "lowp float screen(lowp float base, lowp float blend)"
            "{"
            "   return 1.0-((1.0-base)*(1.0-blend));"
            "}"
            "lowp vec3 normal(lowp vec3 base, lowp vec3 blend)"
            "{"
            "   return blend;"
            "}"
            "lowp vec3 lighten(lowp vec3 base, lowp vec3 blend)"
            "{"
            "   return vec3(max(base.r, blend.r), max(base.g, blend.g), max(base.b, blend.b));"
            "}"
            "lowp vec3 darken(lowp vec3 base, lowp vec3 blend)"
            "{"
            "   return vec3(min(base.r, blend.r), min(base.g, blend.g), min(base.b, blend.b));"
            "}"
            "lowp vec3 softlight(lowp vec3 base, lowp vec3 blend)"
            "{"
            "   return vec3(softlight(base.r, blend.r), softlight(base.g, blend.g), softlight(base.b, blend.b));"
            "}"
            "lowp vec3 overlay(lowp vec3 base, lowp vec3 blend)"
            "{"
            "   return vec3(overlay(base.r, blend.r), overlay(base.g, blend.g), overlay(base.b, blend.b));"
            "}"
            "lowp vec3 multiply(lowp vec3 base, lowp vec3 blend)"
            "{"
            "   return vec3(multiply(base.r, blend.r), multiply(base.g, blend.g), multiply(base.b, blend.b));"
            "}"
            "lowp vec3 screen(lowp vec3 base, lowp vec3 blend)"
            "{"
            "   return vec3(screen(base.r, blend.r), screen(base.g, blend.g), screen(base.b, blend.b));"
            "}"
            "lowp vec3 hardlight(lowp vec3 base, lowp vec3 blend)"
            "{"
            "   return vec3(overlay(blend.r, base.r), overlay(blend.g, base.g), overlay(blend.b, base.b));"
            "}"
            "void main()"
            "{"
            "   lowp vec4 base = gl_LastFragData[0];"
            "   lowp vec4 blend = texture2D(inputImageTexture, textureCoordinate);"
            "   if (blend.a == 0.0) {"
            "       discard;"
            "   }"
            "   lowp vec3 textureColor = vec3(0);"
            "   lowp float opacity = alpha * intensity * blend.a;"
            "   if (blendType == 1) {"
            "       textureColor = lighten(base.rgb, blend.rgb) * opacity + base.rgb * (1.0 - opacity);"
            "   } else if (blendType == 2) {"
            "       textureColor = darken(base.rgb, blend.rgb) * opacity + base.rgb * (1.0 - opacity);"
            "   } else if (blendType == 3) {"
            "       textureColor = softlight(base.rgb, blend.rgb) * opacity + base.rgb * (1.0 - opacity);"
            "   } else if (blendType == 4) {"
            "       textureColor = overlay(base.rgb, blend.rgb) * opacity + base.rgb * (1.0 - opacity);"
            "   } else if (blendType == 5) {"
            "       textureColor = multiply(base.rgb, blend.rgb) * opacity + base.rgb * (1.0 - opacity);"
            "   } else if (blendType == 6) {"
            "       textureColor = screen(base.rgb, blend.rgb) * opacity + base.rgb * (1.0 - opacity);"
            "   } else if (blendType == 7) {"
            "       textureColor = hardlight(base.rgb, blend.rgb) * opacity + base.rgb * (1.0 - opacity);"
            "   } else {"
            "       opacity = alpha * blend.a;"
            "       textureColor = normal(base.rgb, blend.rgb) * opacity + base.rgb * (1.0 - opacity);"
            "   }"
            "   gl_FragColor = vec4(textureColor, 1.0);"
            "}";
    } else if (supportGL_ARM_shader_framebuffer_fetch) {
        blendFragmentShaderString =
            "#extension GL_ARM_shader_framebuffer_fetch : require\n"
            "varying highp vec2 textureCoordinate;"
            "uniform sampler2D inputImageTexture;"
            "uniform lowp float alpha;"
            "uniform lowp float intensity;"
            "uniform lowp int blendType;"
            "lowp float softlight(lowp float base, lowp float blend)"
            "{"
            "   return (blend<0.5)?(2.0*base*blend+base*base*(1.0-2.0*blend)):(sqrt(base)*(2.0*blend-1.0)+2.0*base*(1.0-blend));"
            "}"
            "lowp float overlay(lowp float base, lowp float blend)"
            "{"
            "   return base<0.5?(2.0*base*blend):(1.0-2.0*(1.0-base)*(1.0-blend));"
            "} "
            "lowp float multiply(lowp float base, lowp float blend)"
            "{"
            "   return base*blend;"
            "}"
            "lowp float screen(lowp float base, lowp float blend)"
            "{"
            "   return 1.0-((1.0-base)*(1.0-blend));"
            "}"
            "lowp vec3 normal(lowp vec3 base, lowp vec3 blend)"
            "{"
            "   return blend;"
            "}"
            "lowp vec3 lighten(lowp vec3 base, lowp vec3 blend)"
            "{"
            "   return vec3(max(base.r, blend.r), max(base.g, blend.g), max(base.b, blend.b));"
            "}"
            "lowp vec3 darken(lowp vec3 base, lowp vec3 blend)"
            "{"
            "   return vec3(min(base.r, blend.r), min(base.g, blend.g), min(base.b, blend.b));"
            "}"
            "lowp vec3 softlight(lowp vec3 base, lowp vec3 blend)"
            "{"
            "   return vec3(softlight(base.r, blend.r), softlight(base.g, blend.g), softlight(base.b, blend.b));"
            "}"
            "lowp vec3 overlay(lowp vec3 base, lowp vec3 blend)"
            "{"
            "   return vec3(overlay(base.r, blend.r), overlay(base.g, blend.g), overlay(base.b, blend.b));"
            "}"
            "lowp vec3 multiply(lowp vec3 base, lowp vec3 blend)"
            "{"
            "   return vec3(multiply(base.r, blend.r), multiply(base.g, blend.g), multiply(base.b, blend.b));"
            "}"
            "lowp vec3 screen(lowp vec3 base, lowp vec3 blend)"
            "{"
            "   return vec3(screen(base.r, blend.r), screen(base.g, blend.g), screen(base.b, blend.b));"
            "}"
            "lowp vec3 hardlight(lowp vec3 base, lowp vec3 blend)"
            "{"
            "   return vec3(overlay(blend.r, base.r), overlay(blend.g, base.g), overlay(blend.b, base.b));"
            "}"
            "void main()"
            "{"
            "   lowp vec4 base = gl_LastFragColorARM;"
            "   lowp vec4 blend = texture2D(inputImageTexture, textureCoordinate);"
            "   if (blend.a == 0.0) {"
            "       discard;"
            "   }"
            "   lowp vec3 textureColor = vec3(0);"
            "   lowp float opacity = alpha * intensity * blend.a;"
            "   if (blendType == 1) {"
            "       textureColor = lighten(base.rgb, blend.rgb) * opacity + base.rgb * (1.0 - opacity);"
            "   } else if (blendType == 2) {"
            "       textureColor = darken(base.rgb, blend.rgb) * opacity + base.rgb * (1.0 - opacity);"
            "   } else if (blendType == 3) {"
            "       textureColor = softlight(base.rgb, blend.rgb) * opacity + base.rgb * (1.0 - opacity);"
            "   } else if (blendType == 4) {"
            "       textureColor = overlay(base.rgb, blend.rgb) * opacity + base.rgb * (1.0 - opacity);"
            "   } else if (blendType == 5) {"
            "       textureColor = multiply(base.rgb, blend.rgb) * opacity + base.rgb * (1.0 - opacity);"
            "   } else if (blendType == 6) {"
            "       textureColor = screen(base.rgb, blend.rgb) * opacity + base.rgb * (1.0 - opacity);"
            "   } else if (blendType == 7) {"
            "       textureColor = hardlight(base.rgb, blend.rgb) * opacity + base.rgb * (1.0 - opacity);"
            "   } else {"
            "       opacity = alpha * blend.a;"
            "       textureColor = normal(base.rgb, blend.rgb) * opacity + base.rgb * (1.0 - opacity);"
            "   }"
            "   gl_FragColor = vec4(textureColor, 1.0);"
            "}";
    } else {
        blendFragmentShaderString =
            "varying highp vec2 textureCoordinate;"
            "uniform sampler2D inputImageTexture;"
            "uniform lowp float alpha;"
            "uniform lowp float intensity;"
            "uniform lowp int blendType;"
            "lowp float softlight(lowp float base, lowp float blend)"
            "{"
            "   return (blend<0.5)?(2.0*base*blend+base*base*(1.0-2.0*blend)):(sqrt(base)*(2.0*blend-1.0)+2.0*base*(1.0-blend));"
            "}"
            "lowp float overlay(lowp float base, lowp float blend)"
            "{"
            "   return base<0.5?(2.0*base*blend):(1.0-2.0*(1.0-base)*(1.0-blend));"
            "} "
            "lowp float multiply(lowp float base, lowp float blend)"
            "{"
            "   return base*blend;"
            "}"
            "lowp float screen(lowp float base, lowp float blend)"
            "{"
            "   return 1.0-((1.0-base)*(1.0-blend));"
            "}"
            "lowp vec3 normal(lowp vec3 base, lowp vec3 blend)"
            "{"
            "   return blend;"
            "}"
            "lowp vec3 lighten(lowp vec3 base, lowp vec3 blend)"
            "{"
            "   return vec3(max(base.r, blend.r), max(base.g, blend.g), max(base.b, blend.b));"
            "}"
            "lowp vec3 darken(lowp vec3 base, lowp vec3 blend)"
            "{"
            "   return vec3(min(base.r, blend.r), min(base.g, blend.g), min(base.b, blend.b));"
            "}"
            "lowp vec3 softlight(lowp vec3 base, lowp vec3 blend)"
            "{"
            "   return vec3(softlight(base.r, blend.r), softlight(base.g, blend.g), softlight(base.b, blend.b));"
            "}"
            "lowp vec3 overlay(lowp vec3 base, lowp vec3 blend)"
            "{"
            "   return vec3(overlay(base.r, blend.r), overlay(base.g, blend.g), overlay(base.b, blend.b));"
            "}"
            "lowp vec3 multiply(lowp vec3 base, lowp vec3 blend)"
            "{"
            "   return vec3(multiply(base.r, blend.r), multiply(base.g, blend.g), multiply(base.b, blend.b));"
            "}"
            "lowp vec3 screen(lowp vec3 base, lowp vec3 blend)"
            "{"
            "   return vec3(screen(base.r, blend.r), screen(base.g, blend.g), screen(base.b, blend.b));"
            "}"
            "lowp vec3 hardlight(lowp vec3 base, lowp vec3 blend)"
            "{"
            "   return vec3(overlay(blend.r, base.r), overlay(blend.g, base.g), overlay(blend.b, base.b));"
            "}"
            "void main()"
            "{"
            "   lowp vec4 base = vec4(0.0);"
            "   lowp vec4 blend = texture2D(inputImageTexture, textureCoordinate);"
            "   if (blend.a == 0.0) {"
            "       discard;"
            "   }"
            "   lowp vec3 textureColor = vec3(0);"
            "   lowp float opacity = alpha * intensity * blend.a;"
            "   if (blendType == 1) {"
            "       textureColor = lighten(base.rgb, blend.rgb) * opacity + base.rgb * (1.0 - opacity);"
            "   } else if (blendType == 2) {"
            "       textureColor = darken(base.rgb, blend.rgb) * opacity + base.rgb * (1.0 - opacity);"
            "   } else if (blendType == 3) {"
            "       textureColor = softlight(base.rgb, blend.rgb) * opacity + base.rgb * (1.0 - opacity);"
            "   } else if (blendType == 4) {"
            "       textureColor = overlay(base.rgb, blend.rgb) * opacity + base.rgb * (1.0 - opacity);"
            "   } else if (blendType == 5) {"
            "       textureColor = multiply(base.rgb, blend.rgb) * opacity + base.rgb * (1.0 - opacity);"
            "   } else if (blendType == 6) {"
            "       textureColor = screen(base.rgb, blend.rgb) * opacity + base.rgb * (1.0 - opacity);"
            "   } else if (blendType == 7) {"
            "       textureColor = hardlight(base.rgb, blend.rgb) * opacity + base.rgb * (1.0 - opacity);"
            "   } else {"
            "       opacity = alpha * blend.a;"
            "       textureColor = normal(base.rgb, blend.rgb) * opacity + base.rgb * (1.0 - opacity);"
            "   }"
            "   gl_FragColor = vec4(textureColor, 1.0);"
            "}";
    }

    string vertexContent = blendVertexShaderString;
    string fragmentContent = blendFragmentShaderString;
    setVertexContent(vertexContent);
    setFragmentContent(fragmentContent);
    createShader();

#ifndef CLOSE_NATIVE_CODE
    projectionMatrixUniform = uniformIndex("projectionMatrix");
    modelViewMatrixUniform = uniformIndex("modelViewMatrix");
    blendTypeUniform = uniformIndex("blendType");
    blendIntensityUniform = uniformIndex("intensity");
    alphaUniform = uniformIndex("alpha");
#endif
}

bool OpenGLBlendShaderContext::createShader()
{
#ifndef CLOSE_NATIVE_CODE
    GLuint vertexShader = 0, fragmentShader = 0, shaderProgram = 0;

    do {
        vertexShader = createShader(GL_VERTEX_SHADER, vertexContent_);
        if (!vertexShader) {
            break;
        }

        fragmentShader = createShader(GL_FRAGMENT_SHADER, fragmentContent_);
        if (!fragmentShader) {
            break;
        }

        shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, vertexShader);
        glAttachShader(shaderProgram, fragmentShader);

        glBindAttribLocation(shaderProgram, static_cast<GLuint>(glGetAttribLocation(shaderProgram, "position")), "position");
        glBindAttribLocation(shaderProgram, static_cast<GLuint>(glGetAttribLocation(shaderProgram, "inputTextureCoordinate")), "inputTextureCoordinate");
        glBindAttribLocation(shaderProgram, static_cast<GLuint>(glGetAttribLocation(shaderProgram, "filterInputTextureCoordinate")), "filterInputTextureCoordinate");
        glBindAttribLocation(shaderProgram, static_cast<GLuint>(glGetAttribLocation(shaderProgram, "coverInputTextureCoordinate")), "coverInputTextureCoordinate");

        glLinkProgram(shaderProgram);

        if (vertexShader) {
            glDetachShader(shaderProgram, vertexShader);
            glDeleteShader(vertexShader);
            vertexShader = 0;
        }

        if (fragmentShader) {
            glDetachShader(shaderProgram, fragmentShader);
            glDeleteShader(fragmentShader);
            fragmentShader = 0;
        }

        GLint result = GL_FALSE;
        glGetProgramiv(shaderProgram, GL_LINK_STATUS, &result);
        if (result == GL_FALSE) {
            char log[1024];
            int len = 0;
            glGetProgramInfoLog(shaderProgram, sizeof(log), &len, log);
            break;
        }

        GLint positionLocation = glGetAttribLocation(shaderProgram, "position");
        assert(positionLocation >= 0);
        GLint inputTextureCoordinateLocation = glGetAttribLocation(shaderProgram, "inputTextureCoordinate");
        assert(inputTextureCoordinateLocation >= 0);
        GLint inputImageTextureUniform = glGetUniformLocation(shaderProgram, "inputImageTexture");

        shaderProgram_ = shaderProgram;
        positionLocation_ = static_cast<GLuint>(positionLocation);
        textureCoordinateLocation_ = static_cast<GLuint>(inputTextureCoordinateLocation);
        imageTextureUniform_ = inputImageTextureUniform;

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
        if (shaderProgram) {
            glDetachShader(shaderProgram, vertexShader);
        }
        glDeleteShader(vertexShader);
    }

    if (fragmentShader) {
        if (shaderProgram) {
            glDetachShader(shaderProgram, fragmentShader);
        }
        glDeleteShader(fragmentShader);
    }

    if (shaderProgram) {
        glDeleteProgram(shaderProgram);
    }

    assert(0);

    return false;
#else
    return false;
#endif
}

void OpenGLBlendShaderContext::begin(TextureFrame *textureFrame, const MSize &size)
{
#ifndef CLOSE_NATIVE_CODE
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    GLuint texture = *static_cast<GLuint*>(textureFrame->getFrame());
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

    assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

    glViewport(0, 0, static_cast<GLsizei>(size.getWidth()), static_cast<GLsizei>(size.getHeight()));

    glClearColor(0.f, 0.f, 0.f, 0.f);
    glClear(GL_COLOR_BUFFER_BIT);

    glEnable(GL_BLEND);
#endif
}

void OpenGLBlendShaderContext::render(TextureFrame *textureFrame)
{
#ifndef CLOSE_NATIVE_CODE
    glUseProgram(shaderProgram_);

    const float* position = textureFrame->getPosition();
    const float* textureCoordinate = textureFrame->getTextureCoordinate();
    const GLuint texture = *static_cast<GLuint*>(textureFrame->getFrame());
    const Matrix& projectionMatrix = textureFrame->getProjectionMatrix();
    const Matrix& modelViewMatrix = textureFrame->getModelViewMatrix();
    const float alpha = 1.0f;

    setInt(shaderProgram_, blendTypeUniform, blendFilter_->getBlendType());
    setFloat(shaderProgram_, blendIntensityUniform, blendFilter_->getBlendIntensity());
    setFloat(shaderProgram_, alphaUniform, alpha);
    setMatrix4f(shaderProgram_, projectionMatrixUniform, projectionMatrix.mData);
    setMatrix4f(shaderProgram_, modelViewMatrixUniform, modelViewMatrix.mData);

#ifdef IOS_OS
    glBindBuffer(GL_ARRAY_BUFFER, positionVBO_);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(GL_FLOAT) * 2 * 4, position);
    glVertexAttribPointer(positionLocation_, 2, GL_FLOAT, 0, 2 * sizeof(GL_FLOAT), nullptr);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glEnableVertexAttribArray(positionLocation_);

    glBindBuffer(GL_ARRAY_BUFFER, textureCoordinateVBO_);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(GL_FLOAT) * 2 * 4, textureCoordinate);
    glVertexAttribPointer(textureCoordinateLocation_, 2, GL_FLOAT, 0, 2 * sizeof(GL_FLOAT), nullptr);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glEnableVertexAttribArray(textureCoordinateLocation_);
#elif defined(ANDROID_OS)
    glEnableVertexAttribArray(positionLocation_);
    glBindBuffer(GL_ARRAY_BUFFER, positionVBO_);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(GL_FLOAT) * 2 * 4, position);
    glVertexAttribPointer(positionLocation_, 2, GL_FLOAT, 0, 2 * sizeof(GL_FLOAT), nullptr);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glEnableVertexAttribArray(textureCoordinateLocation_);
    glBindBuffer(GL_ARRAY_BUFFER, textureCoordinateVBO_);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(GL_FLOAT) * 2 * 4, textureCoordinate);
    glVertexAttribPointer(textureCoordinateLocation_, 2, GL_FLOAT, 0, 2 * sizeof(GL_FLOAT), nullptr);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
#endif

    glActiveTexture(GL_TEXTURE0);

    glBindTexture(GL_TEXTURE_2D, texture);

    glUniform1i(imageTextureUniform_, 0);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glBindTexture(GL_TEXTURE_2D, 0);

#if defined(IOS_OS) || defined(ANDROID_OS)
    glDisableVertexAttribArray(positionLocation_);
    glDisableVertexAttribArray(textureCoordinateLocation_);
#endif
#endif
}

void OpenGLBlendShaderContext::end()
{
#ifndef CLOSE_NATIVE_CODE
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glDeleteFramebuffers(1, &framebuffer);
    framebuffer = 0;

    glBindTexture(GL_TEXTURE_2D, 0);

    glDisable(GL_BLEND);
#endif
}

}
