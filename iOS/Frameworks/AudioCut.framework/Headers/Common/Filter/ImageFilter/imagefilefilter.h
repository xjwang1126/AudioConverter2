#ifndef IMAGEFILEFILTER_H
#define IMAGEFILEFILTER_H

#include "imagefilter.h"
#include "Filter/ImageFilter/matrix.h"
#include "Format/msize.h"

namespace MediaLibrary {

class Frame;
class TextureFrame;
class ImageFrame;

class ImageFileFilter : public ImageFilter
{
public:
    ImageFileFilter();

#ifdef IOS_OS
    void setTextureCache(void* textureCache);
#endif

    TextureFrame* render(Frame* frame, const MSize& size,
                         const float* position, const float* textureCoordinate,
                         const Matrix& projectionMatrix, const Matrix& modelViewMatrix,
                         void* context);

    ImageFrame* read(TextureFrame* textureFrame);

private:
};

}

#endif // IMAGEFILEFILTER_H
