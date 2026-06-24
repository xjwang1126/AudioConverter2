#include "videoutil.h"
#include "videoutilinternal.h"

namespace MediaLibrary {

VideoUtil::VideoUtil()
{
    internal.reset(new VideoUtilInternal());
}

void VideoUtil::setQuietLog()
{
    internal->setQuietLog();
}

bool VideoUtil::videoToGIF(const string &inFilePath, const string &outFilePath, const int64_t startTime, const int64_t endTime, const VideoFormat &videoFormat, const GIFMode &gifMode, const MediaUtilDelegate *delegate)
{
    return internal->videoToGIF(inFilePath, outFilePath, startTime, endTime, videoFormat, gifMode, delegate);
}

int VideoUtil::videoToGIF(char *inData, int inDataSize, char **outData, int *outDataSize, const int64_t startTime, const int64_t endTime, const VideoFormat &videoFormat, const GIFMode &gifMode, const MediaUtilDelegate *delegate)
{
    return internal->videoToGIF(inData, inDataSize, outData, outDataSize, startTime, endTime, videoFormat, gifMode, delegate);
}

bool VideoUtil::imageToGIF(const std::vector<string> &inFilePaths, const string &outFilePath, const FillMode &fillMode, const VideoFormat &videoFormat, const GIFMode& gifMode, const MediaUtilDelegate *delegate)
{
    return internal->imageToGIF(inFilePaths, outFilePath, fillMode, videoFormat, gifMode, delegate);
}

int VideoUtil::imageToGIF(char *inData, int inDataSize, char **outData, int *outDataSize, const FillMode &fillMode, const VideoFormat &videoFormat, const MediaUtilDelegate *delegate)
{
    return internal->imageToGIF(inData, inDataSize, outData, outDataSize, fillMode, videoFormat, delegate);
}

int VideoUtil::imageDataToGIF(char *inData, int inDataSize, int inWidth, int inHeight, int inStride, char **outData, int *outDataSize, const FillMode &fillMode, const VideoFormat &videoFormat, const MediaUtilDelegate *delegate)
{
    return internal->imageDataToGIF(inData, inDataSize, inWidth, inHeight, inStride, outData, outDataSize, fillMode, videoFormat, delegate);
}

int VideoUtil::imageDataToPicture(char *inData, int inDataSize, int inWidth, int inHeight, int inStride, char **outData, int *outDataSize, char *outFormatName, const FillMode &fillMode, const VideoFormat &videoFormat, const MediaUtilDelegate *delegate)
{
    return internal->imageDataToPicture(inData, inDataSize, inWidth, inHeight, inStride, outData, outDataSize, outFormatName, fillMode, videoFormat, delegate);
}

int VideoUtil::imageDataToGIFImageData(char *inData, int inDataSize, int inWidth, int inHeight, int inStride, char **outData, int *outDataSize)
{
    return internal->imageDataToGIFImageData(inData, inDataSize, inWidth, inHeight, inStride, outData, outDataSize);
}

int VideoUtil::testImage(char *inData, int inDataSize, int inWidth, int inHeight, int inStride)
{
    return internal->testImage(inData, inDataSize, inWidth, inHeight, inStride);
}

int VideoUtil::getInfo(char *inData, int inDataSize, int *width, int *height, int64_t *duration, float *fps)
{
    return internal->getInfo(inData, inDataSize, width, height, duration, fps);
}

void VideoUtil::cancel()
{
    return internal->cancel();
}

int VideoUtil::edit(char *inData, int inDataSize, char **outData, int *outDataSize, char *formatName, const int64_t startTime, const int64_t endTime, const MediaUtilDelegate *delegate)
{
    return internal->edit(inData, inDataSize, outData, outDataSize, formatName, startTime, endTime, delegate);
}

}
