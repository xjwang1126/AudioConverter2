#ifndef VIDEOUTILINTERNAL_H
#define VIDEOUTILINTERNAL_H

#include "MediaUtil/mediautilinternal.h"
#include "VideoUtil/videoutilinfo.h"

#include <unordered_map>

namespace MediaLibrary {

class VideoUtilInternal : public MediaUtilInternal
{
public:
    VideoUtilInternal();

    bool videoToGIF(const string& inFilePath, const string& outFilePath, const int64_t startTime, const int64_t endTime, const VideoFormat& videoFormat, const GIFMode& gifMode, const MediaUtilDelegate* delegate);

    int videoToGIF(char* inData, int inDataSize, char** outData, int* outDataSize, const int64_t startTime, const int64_t endTime, const VideoFormat& videoFormat, const GIFMode& gifMode, const MediaUtilDelegate* delegate);

    bool imageToGIF(const std::vector<string>& inFilePaths, const string& outFilePath, const FillMode& fillMode, const VideoFormat& videoFormat, const GIFMode& gifMode, const MediaUtilDelegate* delegate);

    int imageToGIF(char* inData, int inDataSize, char** outData, int* outDataSize, const FillMode& fillMode, const VideoFormat& videoFormat, const MediaUtilDelegate* delegate);

    int imageDataToGIF(char* inData, int inDataSize, int inWidth, int inHeight, int inStride, char** outData, int* outDataSize, const FillMode& fillMode, const VideoFormat& videoFormat, const MediaUtilDelegate* delegate);

    int imageDataToPicture(char* inData, int inDataSize, int inWidth, int inHeight, int inStride, char** outData, int* outDataSize, char* outFormatName, const FillMode& fillMode, const VideoFormat& videoFormat, const MediaUtilDelegate* delegate);

    int imageDataToGIFImageData(char* inData, int inDataSize, int inWidth, int inHeight, int inStride, char** outData, int* outDataSize);

    int testImage(char* inData, int inDataSize, int inWidth, int inHeight, int inStride);

    std::unordered_map<int64_t, int64_t> shotDetection(const string& inFilePath, const MediaUtilDelegate* delegate);

    int getInfo(char* inData, int inDataSize, int* width, int* height, int64_t* duration, float* fps);

    int edit(char* inData, int inDataSize, char** outData, int* outDataSize, char* formatName, const int64_t startTime, const int64_t endTime, const MediaUtilDelegate* delegate);
};

}

#endif // VIDEOUTILINTERNAL_H
