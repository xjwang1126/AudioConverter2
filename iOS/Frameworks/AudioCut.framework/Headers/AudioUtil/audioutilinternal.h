#ifndef AUDIOUTILINTERNAL_H
#define AUDIOUTILINTERNAL_H

#include "MediaUtil/mediautilinternal.h"
#include "AudioUtil/audioutilinfo.h"

#include <string>

namespace MediaLibrary {

class AudioUtilInternal : public MediaUtilInternal
{
public:
    AudioUtilInternal();

    int editAudio(const string& inFilePath, const string& coverFilePath, const string& outFilePath, const int64_t startTime, const int64_t endTime, const int64_t fadeIn, const int64_t fadeOut, const float tempo, const TimeMode& timeMode, const AudioFormat& audioFormat, const std::map<MetadataType, string>& metadatas, const MediaUtilDelegate* delegate);

    int editAudio(char* inData, int inDataSize, char* coverData, int coverDataSize, char** outData, int* outDataSize, char* formatName, const int64_t startTime, const int64_t endTime, const int64_t fadeIn, const int64_t fadeOut, const float tempo, const TimeMode& timeMode, const AudioFormat& audioFormat, const std::map<MetadataType, string>& metadatas, const MediaUtilDelegate* delegate);

    bool waveform(const string& filePath, std::vector<float>& waveform, const MediaUtilDelegate* delegate);

    int waveform(char* inData, int inDataSize, float** waveform, int* waveformSize, const MediaUtilDelegate* delegate);

    int testFile(char* data, int dataSize);

    int getInfo(char* inData, int inDataSize, char** coverData, int* coverDataSize, int* coverWidth, int* coverHeight, int* coverStride, std::map<MetadataType, string>& metadatas);

    int getInfo(char* inData, int inDataSize, char** coverData, int* coverDataSize, std::map<MetadataType, string>& metadatas, int64_t* duration);

    int streamIO(const string& inFilePath, const string& outFilePath);
};

}

#endif // AUDIOUTILINTERNAL_H
