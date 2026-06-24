#include "msize.h"

#include <algorithm>

namespace MediaLibrary {

MSize::MSize(const float width, const float height)
    : width_(width)
    , height_(height)
{
}

void scaleSizeByFit(const MSize &inSize, const MSize &maxSize, MSize *outSize)
{
    const MSize defaultSize;
    if (inSize == defaultSize || maxSize == defaultSize || !outSize) {
        return;
    }
    const float inAspectRatio = inSize.getWidth() / inSize.getHeight();
    const float maxAspectRatio = maxSize.getWidth() / maxSize.getHeight();
    if (inAspectRatio < maxAspectRatio) {
        outSize->setHeight(maxSize.getHeight());
        outSize->setWidth(maxSize.getHeight() * inAspectRatio);
    } else {
        outSize->setWidth(maxSize.getWidth());
        outSize->setHeight(maxSize.getWidth() / inAspectRatio);
    }
}

void scaleSizeByFill(const MSize &inSize, const MSize &maxSize, MSize *outSize)
{
    const MSize defaultSize;
    if (inSize == defaultSize || maxSize == defaultSize || !outSize) {
        return;
    }
    const float inAspectRatio = inSize.getWidth() / inSize.getHeight();
    const float maxAspectRatio = maxSize.getWidth() / maxSize.getHeight();
    if (inAspectRatio < maxAspectRatio) {
        outSize->setWidth(maxSize.getWidth());
        outSize->setHeight(maxSize.getWidth() / inAspectRatio);
    } else {
        outSize->setHeight(maxSize.getHeight());
        outSize->setWidth(maxSize.getHeight() * inAspectRatio);
    }
}

void scaleSizeByMin(const MSize &inSize, const MSize &maxSize, MSize *outSize)
{
    const int minLength = std::min(std::min(inSize.getWidth(), inSize.getHeight()), std::min(maxSize.getWidth(), maxSize.getHeight()));
    scaleSizeByMin(inSize, minLength, outSize);
}

void scaleSizeByMin(const MSize &inSize, const int minLength, MSize *outSize)
{
    if (!outSize) {
        return;
    }
    const float aspectRatio = inSize.getWidth() / inSize.getHeight();
    if (aspectRatio < 1.f) {
        outSize->setWidth(minLength);
        outSize->setHeight(static_cast<int>(roundf(minLength / aspectRatio)));
    } else {
        outSize->setHeight(minLength);
        outSize->setWidth(static_cast<int>(roundf(minLength * aspectRatio)));
    }
    if (static_cast<int>(outSize->getWidth()) % 2 != 0) {
        outSize->setWidth(outSize->getWidth() + 1);
    }
    if (static_cast<int>(outSize->getHeight()) % 2 != 0) {
        outSize->setHeight(outSize->getHeight() + 1);
    }
}

void scaleSizeByMax(const MSize &inSize, const int maxLength, MSize *outSize)
{
    if (!outSize) {
        return;
    }
    const float aspectRatio = inSize.getWidth() / inSize.getHeight();
    if (aspectRatio < 1.f) {
        outSize->setHeight(maxLength);
        outSize->setWidth(static_cast<int>(roundf(maxLength * aspectRatio)));
    } else {
        outSize->setWidth(maxLength);
        outSize->setHeight(static_cast<int>(roundf(maxLength / aspectRatio)));
    }
    if (static_cast<int>(outSize->getWidth()) % 2 != 0) {
        outSize->setWidth(outSize->getWidth() + 1);
    }
    if (static_cast<int>(outSize->getHeight()) % 2 != 0) {
        outSize->setHeight(outSize->getHeight() + 1);
    }
}

}
