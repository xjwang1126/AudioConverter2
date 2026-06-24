#include "gifgenerator.h"
#include "gifgeneratorinternal.h"

namespace MediaLibrary {

GIFGenerator::GIFGenerator()
{
    internal.reset(new GIFGeneratorInternal());
}

bool GIFGenerator::open(const FillMode &fillMode, const VideoFormat &videoFormat, const MSize &mediaSize, const bool minMode, const int loop, const GIFMode &gifMode)
{
    return internal->open(fillMode, videoFormat, mediaSize, minMode, loop, gifMode);
}

void GIFGenerator::close()
{
    internal->close();
}

int GIFGenerator::addImage(char *data, int dataSize)
{
    return internal->addImage(data, dataSize);
}

int GIFGenerator::addImageData(char *data, int dataSize, int width, int height, int stride)
{
    return internal->addImageData(data, dataSize, width, height, stride);
}

int GIFGenerator::addEmptyImageData(bool *end, int *progress)
{
    return internal->addEmptyImageData(end, progress);
}

void GIFGenerator::getGIFData(char **data, int *dataSize)
{
    internal->getGIFData(data, dataSize);
}

void GIFGenerator::getGIFData(char **data, int *dataSize, bool *end, int *progress, int readDataSize)
{
    internal->getGIFData(data, dataSize, end, progress, readDataSize);
}

}
