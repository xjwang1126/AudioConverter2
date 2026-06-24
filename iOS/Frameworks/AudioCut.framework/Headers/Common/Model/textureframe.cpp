#include "textureframe.h"
#include "Filter/ImageFilter/imagefilter.h"

namespace MediaLibrary {

TextureFrame::TextureFrame()
    : Frame(Frame::FRAME_TEXTURE_TYPE)
{
    switch (ImageFilter::getRenderLanguage()) {
    case ImageFilter::RENDER_OPENGL_LANGUAGE: {
        position[0] = -1;
        position[1] = -1;
        position[2] =  1;
        position[3] = -1;
        position[4] = -1;
        position[5] =  1;
        position[6] =  1;
        position[7] =  1;
    }
        break;
#ifdef IOS_OS
    case ImageFilter::RENDER_METAL_LANGUAGE: {
        position[0] = -1;
        position[1] =  1;
        position[2] =  1;
        position[3] =  1;
        position[4] = -1;
        position[5] = -1;
        position[6] =  1;
        position[7] = -1;
    }
        break;
#endif
    }

    textureCoordinate[0] = 0;
    textureCoordinate[1] = 0;
    textureCoordinate[2] = 1;
    textureCoordinate[3] = 0;
    textureCoordinate[4] = 0;
    textureCoordinate[5] = 1;
    textureCoordinate[6] = 1;
    textureCoordinate[7] = 1;

    projectionMatrix.identity();
    modelViewMatrix.identity();
}

TextureFrame::~TextureFrame()
{
    if (frame_) {
        MediaLibrary::ImageFilter::destroyTexture(frame_);
        frame_ = nullptr;
    }
}

Frame *TextureFrame::copy() const
{
    return nullptr;
}

Frame *TextureFrame::convert(const Frame::FrameType &frameType) const
{
    (void)frameType;
    return nullptr;
}

Frame *TextureFrame::resize(const MSize &size) const
{
    (void)size;
    return nullptr;
}

void TextureFrame::copy(Frame *frame)
{
    (void)frame;
}

void TextureFrame::setTime(const int64_t time, const TimeBase &timeBase)
{
    time_ = (time * timeBase.num * timeBase_.den) / (timeBase.den * timeBase_.num);
}

void TextureFrame::setDuration(const int64_t duration, const TimeBase &timeBase)
{
    duration_ = (duration * timeBase.num * timeBase_.den) / (timeBase.den * timeBase_.num);
}

int64_t TextureFrame::getTime(const TimeBase &timeBase) const
{
    return (time_ * timeBase_.num * timeBase.den) / (timeBase_.den * timeBase.num);
}

int64_t TextureFrame::getDuration(const TimeBase &timeBase) const
{
    return (duration_ * timeBase_.num * timeBase.den) / (timeBase_.den * timeBase.num);
}

void TextureFrame::setPosition(const float *value)
{
    memcpy(position, value, sizeof(float) * 8);
}

void TextureFrame::setTextureCoordinate(const float *value)
{
    memcpy(textureCoordinate, value, sizeof(float) * 8);
}

void TextureFrame::setProjectionMatrix(const Matrix &value)
{
    projectionMatrix.loadWith(value);
}

void TextureFrame::setModelViewMatrix(const Matrix &value)
{
    modelViewMatrix.loadWith(value);
}

}
