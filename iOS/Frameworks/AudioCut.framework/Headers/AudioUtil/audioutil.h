#ifndef AUDIOUTIL_H
#define AUDIOUTIL_H

#include "Common/Format/metadata.h"

#include "MediaUtil/mediautilinfo.h"
#include "AudioUtil/audioutilinfo.h"

#include <map>
#include <memory>
#include <string>
#include <cstdint>

using namespace std;

namespace MediaLibrary {

class MediaUtilDelegate;
class AudioUtilInternal;

class AudioUtil
{
public:
    AudioUtil();
    ~AudioUtil();

    void setQuietLog();

    bool addCover(const string& inFilePath, const string& coverFilePath, const string& outFilePath, const MediaUtilDelegate* delegate);

    int addCover(char* inData, int inDataSize, char* coverData, int coverDataSize, char** outData, int* outDataSize, char* formatName, const MediaUtilDelegate* delegate);

    bool addMetadata(const string& inFilePath, const string& outFilePath, const std::map<MetadataType, string>& metadatas, const MediaUtilDelegate* delegate);

    int addMetadata(char* inData, int inDataSize, char** outData, int* outDataSize, char* formatName, const std::map<MetadataType, string>& metadatas, const MediaUtilDelegate* delegate);

    bool cut(const string& inFilePath, const string& outFilePath, const int64_t startTime, const int64_t endTime, const TimeMode& timeMode, const MediaUtilDelegate* delegate);

    int cut(char* inData, int inDataSize, char** outData, int* outDataSize, char* formatName, const int64_t startTime, const int64_t endTime, const TimeMode& timeMode, const MediaUtilDelegate* delegate);

    bool convert(const string& inFilePath, const string& outFilePath, const int64_t startTime, const int64_t endTime, const AudioFormat& audioFormat, const MediaUtilDelegate* delegate);

    int convert(char* inData, int inDataSize, char** outData, int* outDataSize, char* formatName, const int64_t startTime, const int64_t endTime, const AudioFormat& audioFormat, const MediaUtilDelegate* delegate);

    int edit(const string& inFilePath, const string& coverFilePath, const string& outFilePath, const int64_t startTime, const int64_t endTime, const int64_t fadeIn, const int64_t fadeOut, const float tempo, const TimeMode& timeMode, const AudioFormat& audioFormat, const std::map<MetadataType, string>& metadatas, const MediaUtilDelegate* delegate);

    int edit(char* inData, int inDataSize, char* coverData, int coverDataSize, char** outData, int* outDataSize, char* formatName, const int64_t startTime, const int64_t endTime, const int64_t fadeIn, const int64_t fadeOut, const float tempo, const TimeMode& timeMode, const AudioFormat& audioFormat, const std::map<MetadataType, string>& metadatas, const MediaUtilDelegate* delegate);

    bool waveform(const string& filePath, std::vector<float>& waveform, const MediaUtilDelegate* delegate);

    int waveform(char* inData, int inDataSize, float** waveform, int* waveformSize, const MediaUtilDelegate* delegate);

    int testFile(char* data, int dataSize);

    int getCover(char* inData, int inDataSize, char** coverData, int* coverDataSize, int* coverWidth, int* coverHeight, int* coverStride);

    int getCover(char* inData, int inDataSize, char** coverData, int* coverDataSize);

    int getMetadata(char* inData, int inDataSize, std::map<MetadataType, string>& metadatas);

    int getInfo(char* inData, int inDataSize, char** coverData, int* coverDataSize, int* coverWidth, int* coverHeight, int* coverStride, std::map<MetadataType, string>& metadatas);

    int getInfo(char* inData, int inDataSize, char** coverData, int* coverDataSize, std::map<MetadataType, string>& metadatas, int64_t* duration);

    int streamIO(const string& inFilePath, const string& outFilePath);

    void cancel();

private:
    std::unique_ptr<AudioUtilInternal> internal;
};

}

#endif // AUDIOUTIL_H
