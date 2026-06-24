#include "videooutput.h"
#include "Filter/ImageFilter/imageblendfilter.h"

#ifndef CLOSE_NATIVE_CODE
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#endif

namespace MediaLibrary {

void VideoOutput::display()
{
    switch (ImageFilter::getRenderLanguage()) {
    case ImageFilter::RENDER_OPENGL_LANGUAGE: {
#ifndef CLOSE_NATIVE_CODE
        glClearColor(0.f, 0.f, 0.f, 0.f);
        glClear(GL_COLOR_BUFFER_BIT);

        blendFilter_->setBlendType(0);
        blendFilter_->setBlendIntensity(1.f);
        TextureFrame* readTextureFrame = getReadTextureFrame();
        blendFilter_->render(readTextureFrame, nullptr, nullptr);

        glFinish();

        EGLDisplay display = renderCtx_->getDisplay();
        EGLSurface surface = renderCtx_->getSurface();
        eglSwapBuffers(display, surface);
#endif
    }
        break;
    }
}

void VideoOutput::display(TextureFrame *textureFrame)
{
    switch (ImageFilter::getRenderLanguage()) {
    case ImageFilter::RENDER_OPENGL_LANGUAGE: {
#ifndef CLOSE_NATIVE_CODE
        glClearColor(0.f, 0.f, 0.f, 0.f);
        glClear(GL_COLOR_BUFFER_BIT);

        if (textureFrame) {
            blendFilter_->setBlendType(0);
            blendFilter_->setBlendIntensity(1.f);
            blendFilter_->render(textureFrame, nullptr, nullptr);
        }

        glFinish();

        EGLDisplay display = renderCtx_->getDisplay();
        EGLSurface surface = renderCtx_->getSurface();
        eglSwapBuffers(display, surface);
#endif
    }
        break;
    }
}

bool VideoOutput::initRenderContext()
{
    return false;
}

void VideoOutput::deinitRenderContext()
{
}

bool VideoOutput::init()
{
    return false;
}

void VideoOutput::deinit()
{
}

void VideoOutput::beginBlendCtx()
{
}

void VideoOutput::endBlendCtx()
{
}

void *VideoOutput::getBlendCtx()
{
    return nullptr;
}

}
