#include "imagegenerator.h"
#include "imagegeneratorinternal.h"

namespace MediaLibrary {

ImageGenerator::ImageGenerator()
{
    internal.reset(new ImageGeneratorInternal());
}

void ImageGenerator::setSize(const MSize &size, const bool minMode)
{
    internal->setSize(size, minMode);
}

int ImageGenerator::open(const std::string &filePath)
{
    return internal->open(filePath);
}

int ImageGenerator::open(char *data, int dataSize)
{
    return internal->open(data, dataSize);
}

void ImageGenerator::close()
{
    internal->close();
}

void ImageGenerator::seek(const int64_t time)
{
    internal->seek(time);
}

int ImageGenerator::getImageData(char **data, int *dataSize, int *width, int *height, int *stride, int64_t *time, bool *end)
{
    return internal->getImageData(data, dataSize, width, height, stride, time, end);
}

}
