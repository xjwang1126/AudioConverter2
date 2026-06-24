#ifndef IMAGEGENERATOR_H
#define IMAGEGENERATOR_H

#include "VideoUtil/imagegeneratorinternal.h"

#include "Common/Format/msize.h"

#include <memory>
#include <string>

namespace MediaLibrary {

class ImageGeneratorInternal;

class ImageGenerator
{
public:
    ImageGenerator();

    void setSize(const MSize& size, const bool minMode);

    int open(const std::string& filePath);
    int open(char* data, int dataSize);
    void close();

    void seek(const int64_t time);

    int getImageData(char** data, int* dataSize, int* width, int* height, int* stride, int64_t* time, bool* end);

private:
    std::unique_ptr<ImageGeneratorInternal> internal;
};

}

#endif // IMAGEGENERATOR_H
