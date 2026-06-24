#ifndef AUDIOPROCESSOR_H
#define AUDIOPROCESSOR_H

#include <memory>

#include "AudioUtil/audioprocessorinternal.h"

namespace MediaLibrary {

class AudioProcessor
{
public:
    AudioProcessor();

    int open(char* inData, int inDataSize, char* coverData, int coverDataSize, char* formatName, const int64_t startTime, const int64_t endTime, const int64_t fadeIn, const int64_t fadeOut, const float tempo, const float volume, const bool reverse, const TimeMode& timeMode, const AudioFormat& audioFormat, const std::map<MetadataType, string>& metadatas);
    void process();
    void process(bool* end, int* progress);
    void process(bool* end, int* progress, char** data, int* dataSize);
    void cancel();
    void close();

    void getFileData(char** data, int* dataSize);
    void getFileData(char** data, int* dataSize, bool* end, int* progress, int readDataSize);

private:
    std::unique_ptr<AudioProcessorInternal> internal;
};

}

#endif // AUDIOPROCESSOR_H
