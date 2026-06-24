#include "rendercontext.h"

#ifndef CLOSE_NATIVE_CODE
#include <EGL/egl.h>
#endif

#include <assert.h>

namespace MediaLibrary {

struct RenderContextInternal {
#ifndef CLOSE_NATIVE_CODE
    EGLDisplay display{nullptr};
    EGLSurface surface{nullptr};
    EGLContext context{nullptr};
    EGLConfig config{nullptr};
#endif
};

bool RenderContext::init()
{
    renderCtx_ = new RenderContextInternal;
    return true;
}

void RenderContext::deinit()
{
    if (renderCtx_) {
        delete renderCtx_;
        renderCtx_ = nullptr;
    }
}

bool RenderContext::initRenderContext(RenderContext *renderCtx)
{
#ifndef CLOSE_NATIVE_CODE
    do {
        EGLContext* shareEGLCtx = nullptr;
        if (renderCtx) {
            shareRenderCtx_ = renderCtx;
            shareEGLCtx = static_cast<EGLContext*>(shareRenderCtx_->getShareContext());
        }

        const EGLint displayAttribs[] = {
            EGL_RENDERABLE_TYPE,    EGL_OPENGL_ES2_BIT,
            EGL_SURFACE_TYPE,       EGL_PBUFFER_BIT,
            EGL_BLUE_SIZE,          8,
            EGL_GREEN_SIZE,         8,
            EGL_RED_SIZE,           8,
            EGL_ALPHA_SIZE,         8,
            EGL_DEPTH_SIZE,         0,
            EGL_STENCIL_SIZE,       0,
            EGL_SAMPLE_BUFFERS,     1,
            EGL_SAMPLES,            MSAA_SAMPLE_COUNT,
            EGL_NONE
        };
        EGLDisplay display;
        EGLConfig  config;
        EGLint     numConfigs;
        EGLint format;
        EGLContext context;
        EGLSurface surface;

        if ((display = eglGetDisplay(EGL_DEFAULT_DISPLAY)) == EGL_NO_DISPLAY) {
            break;
        }

        if (!eglInitialize(display, nullptr, nullptr)) {
            break;
        }

        if (!eglChooseConfig(display, displayAttribs, &config, 1, &numConfigs)) {
            break;
        }

        if (!eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format)) {
            break;
        }

        EGLint contextAttribs[] =
        {
            EGL_CONTEXT_CLIENT_VERSION, OPENGLES_VERSION,
            EGL_NONE
        };
        if (!(context = eglCreateContext(display, config, shareEGLCtx, contextAttribs))) {
            break;
        }

        EGLint surfaceAttribs[] =
        {
            EGL_WIDTH, 1,
            EGL_HEIGHT, 1,
            EGL_NONE
        };
        if (!(surface = eglCreatePbufferSurface(display, config, surfaceAttribs))) {
            break;
        }

        renderCtx_->display = display;
        renderCtx_->context = context;
        renderCtx_->surface = surface;
        renderCtx_->config = config;

        return true;
    } while(false);

    assert(0);

    return false;
#else
    return false;
#endif
}

void RenderContext::deinitRenderContext()
{
#ifndef CLOSE_NATIVE_CODE
    do {
        if (renderCtx_->display != EGL_NO_DISPLAY) {
            if (renderCtx_->context != EGL_NO_CONTEXT) {
                if (eglDestroyContext(renderCtx_->display, renderCtx_->context) != EGL_TRUE) {
                    break;
                }
                renderCtx_->context = EGL_NO_CONTEXT;
            }
            if (renderCtx_->surface != EGL_NO_SURFACE) {
                if (eglDestroySurface(renderCtx_->display, renderCtx_->surface) != EGL_TRUE) {
                    break;
                }
                renderCtx_->surface = EGL_NO_SURFACE;
            }

            eglMakeCurrent(renderCtx_->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
            if (eglTerminate(renderCtx_->display) != EGL_TRUE) {
                break;
            }
            renderCtx_->display = EGL_NO_DISPLAY;
        }

        return;
    } while(false);

    assert(0);
#endif
}

void *RenderContext::getShareContext()
{
#ifndef CLOSE_NATIVE_CODE
    if (!renderCtx_->context) {
        assert(0);
    }
    return renderCtx_->context;
#else
    return nullptr;
#endif
}

#ifndef CLOSE_NATIVE_CODE
void *RenderContext::getDisplay()
{
    return renderCtx_->display;
}

void *RenderContext::getSurface()
{
    return renderCtx_->surface;
}

void *RenderContext::getConfig()
{
    return renderCtx_->config;
}

void RenderContext::setSurface(void *surface)
{
    renderCtx_->surface = surface;
}
#endif

bool RenderContext::checkRenderContextOpenGL()
{
#ifndef CLOSE_NATIVE_CODE
    return (eglGetCurrentContext() != nullptr);
#else
    return false;
#endif
}

bool RenderContext::makeRenderContextOpenGL()
{
#ifndef CLOSE_NATIVE_CODE
    bool result = false;
    do {
        if (!eglMakeCurrent(renderCtx_->display, renderCtx_->surface, renderCtx_->surface, renderCtx_->context)) {
            break;
        }

        result = true;
    } while(false);

    return result;
#else
    return false;
#endif
}

bool RenderContext::unmakeRenderContextOpenGL()
{
#ifndef CLOSE_NATIVE_CODE
    bool result = false;
    do {
        if (!eglMakeCurrent(renderCtx_->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT)) {
            break;
        }

        result = true;
    } while(false);

    return result;
#else
    return false;
#endif
}

}
