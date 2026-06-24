#include "videooutput.h"
#include "Filter/ImageFilter/imageblendfilter.h"
#include "Render/metalrendercontext.h"
#include "Render/rendercontext.h"
#include "Filter/ImageFilter/openglshadercontext.h"

#include <UIKit/UIKit.h>
#include <MetalKit/MetalKit.h>

@interface CanvasView : UIView
{
    CAEAGLLayer* layer;
}

@end

@implementation CanvasView

+ (Class)layerClass {
    return [CAEAGLLayer class];
}

- (float)getScaleFactor {
    float scaleFactor = static_cast<float>([[UIScreen mainScreen] scale]);
    if (scaleFactor > 2.f) {
        scaleFactor = 2.f;
    }
    return scaleFactor;
}

- (id)initWithFrame:(CGRect)frame {
    self = [super initWithFrame:frame];
    if (self) {
        @synchronized (self) {
            self.contentScaleFactor = static_cast<CGFloat>([self getScaleFactor]);

            layer = static_cast<CAEAGLLayer*>(self.layer);
            layer.drawableProperties = [NSDictionary dictionaryWithObjectsAndKeys:
                    [NSNumber numberWithBool:NO], kEAGLDrawablePropertyRetainedBacking,
                    kEAGLColorFormatRGBA8, kEAGLDrawablePropertyColorFormat,
                    nil];
            layer.opaque = NO;
        }
    }
    return self;
}

- (CAEAGLLayer*)getLayer {
    return layer;
}

@end

namespace MediaLibrary {

struct VideoOutputContext{
    CanvasView* canvasView{nullptr};

    GLuint renderbuffer;
    GLuint framebuffer;
    GLuint msaaRenderbuffer;
    GLuint msaaFramebuffer;
    GLint backingWidth;
    GLint backingHeight;

    MTKView* mtkView{nullptr};
    id<MTLCommandBuffer> commandBuffer{nullptr};
    id<CAMetalDrawable> drawable{nullptr};
};

void VideoOutput::initCanvasView(const MSize &canvasSize)
{
    switch (ImageFilter::getRenderLanguage()) {
    case ImageFilter::RENDER_OPENGL_LANGUAGE: {
        if ([NSThread isMainThread]) {
            videoOutputCtx_->canvasView = [[CanvasView alloc] initWithFrame:CGRectMake(0, 0, static_cast<CGFloat>(canvasSize.getWidth()), static_cast<CGFloat>(canvasSize.getHeight()))];
        } else {
            dispatch_async(dispatch_get_main_queue(), ^{
                videoOutputCtx_->canvasView = [[CanvasView alloc] initWithFrame:CGRectMake(0, 0, static_cast<CGFloat>(canvasSize.getWidth()), static_cast<CGFloat>(canvasSize.getHeight()))];
            });
        }
    }
        break;
    case ImageFilter::RENDER_METAL_LANGUAGE: {
    }
        break;
    }
}

void *VideoOutput::getCanvasView()
{
    switch (ImageFilter::getRenderLanguage()) {
    case ImageFilter::RENDER_OPENGL_LANGUAGE: {
        return videoOutputCtx_->canvasView;
    }
        break;
    case ImageFilter::RENDER_METAL_LANGUAGE: {
        return nullptr;
    }
        break;
    }
}

void VideoOutput::display()
{
    switch (ImageFilter::getRenderLanguage()) {
    case ImageFilter::RENDER_OPENGL_LANGUAGE: {
        glBindFramebuffer(GL_FRAMEBUFFER, videoOutputCtx_->msaaFramebuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, videoOutputCtx_->msaaRenderbuffer);

        glViewport(0, 0, videoOutputCtx_->backingWidth, videoOutputCtx_->backingHeight);

        glClearColor(0.f, 0.f, 0.f, 0.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        TextureFrame* readTextureFrame = getReadTextureFrame();
        blendFilter_->render(readTextureFrame, nullptr, nullptr);

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, videoOutputCtx_->framebuffer);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, videoOutputCtx_->msaaFramebuffer);

        glBlitFramebuffer(0, 0, videoOutputCtx_->backingWidth, videoOutputCtx_->backingHeight, 0, 0, videoOutputCtx_->backingWidth, videoOutputCtx_->backingHeight, GL_COLOR_BUFFER_BIT, GL_NEAREST);

        GLenum attachments[] = {GL_COLOR_ATTACHMENT0};
        glInvalidateFramebuffer(GL_READ_FRAMEBUFFER, 1, attachments);

        glBindRenderbuffer(GL_RENDERBUFFER, videoOutputCtx_->renderbuffer);

        EAGLContext* context = static_cast<EAGLContext*>(renderCtx_->getShareContext());
        [context presentRenderbuffer:GL_RENDERBUFFER];
    }
        break;
    case ImageFilter::RENDER_METAL_LANGUAGE: {
        MTKView* mtkView = videoOutputCtx_->mtkView;
        CAMetalLayer* metalLayer = static_cast<CAMetalLayer*>(mtkView.layer);
        videoOutputCtx_->drawable = metalLayer.nextDrawable;

//        TextureFrame* readTextureFrame = getReadTextureFrame();
//        blendFilter_->render(readTextureFrame, videoOutputCtx_->drawable.texture, videoOutputCtx_->commandBuffer);
        assert(0);

        [videoOutputCtx_->commandBuffer presentDrawable:videoOutputCtx_->drawable];

        [videoOutputCtx_->commandBuffer commit];

        videoOutputCtx_->commandBuffer = nullptr;
    }
        break;
    }
}

void VideoOutput::display(TextureFrame *textureFrame)
{
    switch (ImageFilter::getRenderLanguage()) {
    case ImageFilter::RENDER_OPENGL_LANGUAGE: {
        glBindFramebuffer(GL_FRAMEBUFFER, videoOutputCtx_->msaaFramebuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, videoOutputCtx_->msaaRenderbuffer);

        glViewport(0, 0, videoOutputCtx_->backingWidth, videoOutputCtx_->backingHeight);

        glClearColor(0.f, 0.f, 0.f, 0.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        if (textureFrame) {
            blendFilter_->render(textureFrame, nullptr, nullptr);
        }

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, videoOutputCtx_->framebuffer);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, videoOutputCtx_->msaaFramebuffer);

        glBlitFramebuffer(0, 0, videoOutputCtx_->backingWidth, videoOutputCtx_->backingHeight, 0, 0, videoOutputCtx_->backingWidth, videoOutputCtx_->backingHeight, GL_COLOR_BUFFER_BIT, GL_NEAREST);

        GLenum attachments[] = {GL_COLOR_ATTACHMENT0};
        glInvalidateFramebuffer(GL_READ_FRAMEBUFFER, 1, attachments);

        glBindRenderbuffer(GL_RENDERBUFFER, videoOutputCtx_->renderbuffer);

        EAGLContext* context = static_cast<EAGLContext*>(renderCtx_->getShareContext());
        [context presentRenderbuffer:GL_RENDERBUFFER];
    }
        break;
    case ImageFilter::RENDER_METAL_LANGUAGE: {
    }
        break;
    }
}

bool VideoOutput::initRenderContext()
{
    bool result = false;

    switch (ImageFilter::getRenderLanguage()) {
    case ImageFilter::RENDER_OPENGL_LANGUAGE: {
        do {
            glGenRenderbuffers(1, &videoOutputCtx_->renderbuffer);
            glBindRenderbuffer(GL_RENDERBUFFER, videoOutputCtx_->renderbuffer);
            EAGLContext* context = static_cast<EAGLContext*>(renderCtx_->getShareContext());
            if (![context renderbufferStorage:GL_RENDERBUFFER fromDrawable:[videoOutputCtx_->canvasView getLayer]]) {
                break;
            }
            glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &videoOutputCtx_->backingWidth);
            glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &videoOutputCtx_->backingHeight);
            if (videoOutputCtx_->backingWidth == 0 || videoOutputCtx_->backingHeight == 0) {
                break;
            }

            glGenFramebuffers(1, &videoOutputCtx_->framebuffer);
            glBindFramebuffer(GL_FRAMEBUFFER, videoOutputCtx_->framebuffer);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, videoOutputCtx_->renderbuffer);

            glGenFramebuffers(1, &videoOutputCtx_->msaaFramebuffer);
            glBindFramebuffer(GL_FRAMEBUFFER, videoOutputCtx_->msaaFramebuffer);

            glGenRenderbuffers(1, &videoOutputCtx_->msaaRenderbuffer);
            glBindRenderbuffer(GL_RENDERBUFFER, videoOutputCtx_->msaaRenderbuffer);
            glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_RGBA8, videoOutputCtx_->backingWidth, videoOutputCtx_->backingHeight);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, videoOutputCtx_->msaaRenderbuffer);

            GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
            if (status != GL_FRAMEBUFFER_COMPLETE) {
                break;
            }

            blendFilter_ = new ImageBlendFilter;

            result = true;
        } while (false);
    }
        break;
    case ImageFilter::RENDER_METAL_LANGUAGE: {
        do {
            blendFilter_ = new ImageBlendFilter;

            result = true;
        } while (false);
    }
        break;
    }

    return result;
}

void VideoOutput::deinitRenderContext()
{
    if (textureFrame1_) {
        delete textureFrame1_;
        textureFrame1_ = nullptr;
    }
    if (textureFrame2_) {
        delete textureFrame2_;
        textureFrame2_ = nullptr;
    }
    switch (ImageFilter::getRenderLanguage()) {
    case ImageFilter::RENDER_OPENGL_LANGUAGE: {
        if (blendFilter_) {
            delete blendFilter_;
            blendFilter_ = nullptr;
        }

        if (videoOutputCtx_->msaaFramebuffer) {
            glDeleteFramebuffers(1, &videoOutputCtx_->msaaFramebuffer);
            videoOutputCtx_->msaaFramebuffer = 0;
        }

        if (videoOutputCtx_->msaaRenderbuffer) {
            glDeleteRenderbuffers(1, &videoOutputCtx_->msaaRenderbuffer);
            videoOutputCtx_->msaaRenderbuffer = 0;
        }

        if (videoOutputCtx_->framebuffer) {
            glDeleteFramebuffers(1, &videoOutputCtx_->framebuffer);
            videoOutputCtx_->framebuffer = 0;
        }

        if (videoOutputCtx_->renderbuffer) {
            glDeleteRenderbuffers(1, &videoOutputCtx_->renderbuffer);
            videoOutputCtx_->renderbuffer = 0;
        }

        if (videoOutputCtx_->canvasView) {
            [videoOutputCtx_->canvasView release];
            videoOutputCtx_->canvasView = nullptr;
        }
    }
        break;
    case ImageFilter::RENDER_METAL_LANGUAGE: {
        if (blendFilter_) {
            delete blendFilter_;
            blendFilter_ = nullptr;
        }
    }
        break;
    }
}

void VideoOutput::beginBlendCtx()
{
    switch (ImageFilter::getRenderLanguage()) {
    case ImageFilter::RENDER_OPENGL_LANGUAGE: {
    }
        break;
    case ImageFilter::RENDER_METAL_LANGUAGE: {
        MetalRenderContext* metalRenderCtx = MetalRenderContext::getInstance();
        videoOutputCtx_->commandBuffer = [metalRenderCtx->getCommandQueue() commandBuffer];
        videoOutputCtx_->commandBuffer.label = @"Blend Command Buffer";
    }
        break;
    }
}

void VideoOutput::endBlendCtx()
{
    switch (ImageFilter::getRenderLanguage()) {
    case ImageFilter::RENDER_OPENGL_LANGUAGE: {
    }
        break;
    case ImageFilter::RENDER_METAL_LANGUAGE: {
        videoOutputCtx_->commandBuffer = nullptr;
    }
        break;
    }
}

void *VideoOutput::getBlendCtx()
{
    switch (ImageFilter::getRenderLanguage()) {
    case ImageFilter::RENDER_OPENGL_LANGUAGE: {
        return nullptr;
    }
        break;
    case ImageFilter::RENDER_METAL_LANGUAGE: {
        return videoOutputCtx_->commandBuffer;
    }
        break;
    }
}

bool VideoOutput::init()
{
    videoOutputCtx_ = new VideoOutputContext();
    return true;
}

void VideoOutput::deinit()
{
    if (videoOutputCtx_) {
        delete videoOutputCtx_;
        videoOutputCtx_ = nullptr;
    }
}

}
