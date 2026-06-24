#include "rendercapability.h"

#ifdef IOS_OS
#include <OpenGLES/ES3/gl.h>
#include <OpenGLES/ES3/glext.h>
#elif defined(ANDROID_OS)
#include <GLES3/gl3.h>
#include <GLES2/gl2ext.h>
#endif

#include <string.h>

namespace MediaLibrary {

RenderCapability* RenderCapability::instance = nullptr;

RenderCapability *RenderCapability::getInstance()
{
    if (!instance) {
        instance = new RenderCapability();
    }
    return instance;
}

void RenderCapability::checkRenderCapability()
{
    RenderCapability::getInstance();
}

bool RenderCapability::isSupportGL_EXT_shader_framebuffer_fetch() const
{
    return supportGL_EXT_shader_framebuffer_fetch;
}

bool RenderCapability::isSupportGL_ARM_shader_framebuffer_fetch() const
{
    return supportGL_ARM_shader_framebuffer_fetch;
}

RenderCapability::RenderCapability()
    : supportGL_EXT_shader_framebuffer_fetch(false)
    , supportGL_ARM_shader_framebuffer_fetch(false)
{
    checkExtensionCapacility();
}

void RenderCapability::checkExtensionCapacility()
{
#ifndef CLOSE_NATIVE_CODE
    GLint extensionNumber = 0;
    glGetIntegerv(GL_NUM_EXTENSIONS, &extensionNumber);
    for (GLint i = 0; i < extensionNumber; i++) {
        const char* extension = reinterpret_cast<const char*>(glGetStringi(GL_EXTENSIONS, static_cast<GLuint>(i)));
        if (!strcmp(extension, "GL_EXT_shader_framebuffer_fetch")) {
            supportGL_EXT_shader_framebuffer_fetch = true;
        } else if (!strcmp(extension, "GL_ARM_shader_framebuffer_fetch")) {
            supportGL_ARM_shader_framebuffer_fetch = true;
        }
    }
#endif
}

}
