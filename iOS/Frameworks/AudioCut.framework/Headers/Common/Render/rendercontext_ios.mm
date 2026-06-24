#include "rendercontext.h"
#include "metalrendercontext.h"
#include "Filter/ImageFilter/imagefilter.h"

#import <UIKit/UIKit.h>

namespace MediaLibrary {

struct RenderContextInternal {
    EAGLContext* context{nullptr};
    CVOpenGLESTextureCacheRef openGLTextureCache{nullptr};
    CVMetalTextureCacheRef metalTextureCache{nullptr};
    bool initRenderCtx = false;
};

bool RenderContext::init()
{
    renderCtx_ = new RenderContextInternal;
}

void RenderContext::deinit()
{
    if (renderCtx_) {
        delete renderCtx_;
        renderCtx_ = nullptr;
    }
}

bool RenderContext::initRenderContext(RenderContext* renderCtx)
{
    bool result = false;

    switch (ImageFilter::getRenderLanguage()) {
    case ImageFilter::RENDER_OPENGL_LANGUAGE: {
        if (renderCtx_->context) {
            result = true;
            break;
        }

        do {
            EAGLSharegroup* sharegroup = nullptr;
            if (renderCtx) {
                shareRenderCtx_ = renderCtx;
                EAGLContext* context = static_cast<EAGLContext*>(shareRenderCtx_->getShareContext());
                sharegroup = static_cast<EAGLSharegroup*>(context.sharegroup);
            }

            renderCtx_->context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES3 sharegroup:sharegroup];

            CVReturn err = CVOpenGLESTextureCacheCreate(kCFAllocatorDefault, nullptr, renderCtx_->context, nullptr, &renderCtx_->openGLTextureCache);
            if (err != noErr) {
                break;
            }

            result = true;
        } while (false);
    }
        break;
    case ImageFilter::RENDER_METAL_LANGUAGE: {
        (void)renderCtx;
        if (renderCtx_->metalTextureCache) {
            result = true;
            break;
        }

        do {
            CVMetalTextureCacheRef textureCache = nullptr;
            MetalRenderContext* metalRenderCtx = MetalRenderContext::getInstance();
            id<MTLDevice> device = metalRenderCtx->getDevice();
            CVReturn res = CVMetalTextureCacheCreate(kCFAllocatorDefault, nullptr, device, nullptr, &textureCache);
            if (res != kCVReturnSuccess) {
                break;
            }
            renderCtx_->metalTextureCache = textureCache;

            result = true;
        } while (false);
    }
        break;
    }

    return result;
}

void RenderContext::deinitRenderContext()
{
    switch (ImageFilter::getRenderLanguage()) {
    case ImageFilter::RENDER_OPENGL_LANGUAGE: {
        if (renderCtx_->openGLTextureCache) {
            CVOpenGLESTextureCacheFlush(renderCtx_->openGLTextureCache, 0);
            CFRelease(renderCtx_->openGLTextureCache);
            renderCtx_->openGLTextureCache = nullptr;
        }

        if ([EAGLContext currentContext])
            [EAGLContext setCurrentContext:nil];

        if (renderCtx_->context) {
            [renderCtx_->context release];
            renderCtx_->context = nullptr;
        }
    }
        break;
    case ImageFilter::RENDER_METAL_LANGUAGE: {
        if (renderCtx_->metalTextureCache) {
            CFRelease(renderCtx_->metalTextureCache);
            renderCtx_->metalTextureCache = nullptr;
        }

        renderCtx_->initRenderCtx = false;
    }
        break;
    }
}

void *RenderContext::getShareContext()
{
    if (ImageFilter::getRenderLanguage() == ImageFilter::RENDER_OPENGL_LANGUAGE) {
        return renderCtx_->context;
    }
    return nullptr;
}

void *RenderContext::getTextureCache()
{
    switch (ImageFilter::getRenderLanguage()) {
    case ImageFilter::RENDER_OPENGL_LANGUAGE: {
        return renderCtx_->openGLTextureCache;
    }
        break;
    case ImageFilter::RENDER_METAL_LANGUAGE: {
        return renderCtx_->metalTextureCache;
    }
        break;
    }

    return nullptr;
}

bool RenderContext::checkRenderContextOpenGL()
{
    return ([EAGLContext currentContext] != nil);
}

bool RenderContext::makeRenderContextOpenGL()
{
    [EAGLContext setCurrentContext:renderCtx_->context];

    return true;
}

bool RenderContext::unmakeRenderContextOpenGL()
{
    [EAGLContext setCurrentContext:nil];

    return true;
}

bool RenderContext::checkRenderContextMetal()
{
    return renderCtx_->initRenderCtx;
}

bool RenderContext::makeRenderContextMetal()
{
    renderCtx_->initRenderCtx = true;

    return true;
}

bool RenderContext::unmakeRenderContextMetal()
{
    renderCtx_->initRenderCtx = false;

    return true;
}

}
