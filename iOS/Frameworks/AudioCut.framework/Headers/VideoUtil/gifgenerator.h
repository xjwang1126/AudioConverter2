#ifndef GIFGENERATOR_H
#define GIFGENERATOR_H

#include "MediaUtil/mediautilinfo.h"
#include "VideoUtil/videoutilinfo.h"
#include "VideoUtil/gifgeneratorinternal.h"

#include "Format/msize.h"

#include <memory>

namespace MediaLibrary {

class GIFGeneratorInternal;

class GIFGenerator
{
public:
    GIFGenerator();

    bool open(const FillMode& fillMode, const VideoFormat& videoFormat, const MSize& mediaSize, const bool minMode, const int loop, const GIFMode& gifMode);
    void close();

    int addImage(char* data, int dataSize);
    int addImageData(char *data, int dataSize, int width, int height, int stride);
    int addEmptyImageData(bool* end, int* progress);
    void getGIFData(char** data, int* dataSize);
    void getGIFData(char** data, int* dataSize, bool* end, int* progress, int readDataSize);

private:
    std::unique_ptr<GIFGeneratorInternal> internal;
};

}

#endif // GIFGENERATOR_H
