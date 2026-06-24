#include "videooutput.h"
#include "Render/rendercontext.h"
#include "Filter/ImageFilter/imageblendfilter.h"
#include "Model/textureframe.h"

namespace MediaLibrary {

VideoOutput::VideoOutput()
{
    renderCtx_ = new RenderContext;

    init();
}

VideoOutput::~VideoOutput()
{
    deinit();

    if (renderCtx_) {
        delete renderCtx_;
        renderCtx_ = nullptr;
    }
}

void VideoOutput::begin(const MSize &size)
{
    PixelFormat pixelFormat = PIXEL_FORMAT_INVALID;
    TextureUsage textureUsage;
    switch (ImageFilter::getRenderLanguage()) {
    case ImageFilter::RENDER_OPENGL_LANGUAGE: {
        pixelFormat = PIXEL_FORMAT_RGBA;
        textureUsage = TEXTURE_USAGE_NONE;
    }
        break;
#ifdef IOS_OS
    case ImageFilter::RENDER_METAL_LANGUAGE: {
        pixelFormat = PIXEL_FORMAT_BGRA;
        textureUsage = static_cast<TextureUsage>(TEXTURE_USAGE_RENDER_TARGET | TEXTURE_USAGE_READ);
    }
        break;
#endif
    }
    if (!textureFrame1_ || (textureFrame1_->getSize() != size)) {
        if (textureFrame1_) {
            delete textureFrame1_;
            textureFrame1_ = nullptr;
        }
        textureFrame1_ = blendFilter_->createTextureFrame(size, pixelFormat, textureUsage);
    }
    if (!textureFrame2_ || (textureFrame2_->getSize() != size)) {
        if (textureFrame2_) {
            delete textureFrame2_;
            textureFrame2_ = nullptr;
        }
        textureFrame2_ = blendFilter_->createTextureFrame(size, pixelFormat, textureUsage);
    }

    void* context = getBlendCtx();
    writeTextureIndex_ = 2;
    TextureFrame* writeTextureFrame = getWriteTextureFrame();
    switchReadWriteTextureFrame();
    blendFilter_->begin(writeTextureFrame, size, context);
}

void VideoOutput::render(TextureFrame *textureFrame)
{
    TextureFrame* readTextureFrame = getReadTextureFrame();
    void* context = getBlendCtx();
    blendFilter_->render(textureFrame, readTextureFrame, context);
}

void VideoOutput::end()
{
    void* context = getBlendCtx();
    blendFilter_->end(context);
}

TextureFrame* VideoOutput::getReadTextureFrame() const
{
    TextureFrame* readTextureFrame = nullptr;
    if (writeTextureIndex_ == 1) {
        readTextureFrame = textureFrame1_;
    } else {
        readTextureFrame = textureFrame2_;
    }
    return readTextureFrame;
}

TextureFrame* VideoOutput::getWriteTextureFrame() const
{
    TextureFrame* writeTextureFrame = nullptr;
    if (writeTextureIndex_ == 1) {
        writeTextureFrame = textureFrame2_;
    } else {
        writeTextureFrame = textureFrame1_;
    }
    return writeTextureFrame;
}

void VideoOutput::switchReadWriteTextureFrame()
{
    if (writeTextureIndex_ == 1) {
        writeTextureIndex_ = 2;
    } else {
        writeTextureIndex_ = 1;
    }
}

}
