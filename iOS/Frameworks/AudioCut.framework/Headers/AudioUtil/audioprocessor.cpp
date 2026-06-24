#include "audioprocessor.h"

namespace MediaLibrary {

AudioProcessor::AudioProcessor()
{
    internal.reset(new AudioProcessorInternal());
}

int AudioProcessor::open(char *inData, int inDataSize, char *coverData, int coverDataSize, char *formatName, const int64_t startTime, const int64_t endTime, const int64_t fadeIn, const int64_t fadeOut, const float tempo, const float volume, const bool reverse, const TimeMode &timeMode, const AudioFormat &audioFormat, const std::map<MetadataType, string> &metadatas)
{
    return internal->open(inData, inDataSize, coverData, coverDataSize, formatName, startTime, endTime, fadeIn, fadeOut, tempo, volume, reverse, timeMode, audioFormat, metadatas);
}

void AudioProcessor::process()
{
    internal->process();
}

void AudioProcessor::process(bool *end, int *progress)
{
    internal->process(end, progress);
}

void AudioProcessor::process(bool *end, int *progress, char **data, int *dataSize)
{
    internal->process(end, progress, data, dataSize);
}

void AudioProcessor::cancel()
{
    internal->cancel();
}

void AudioProcessor::close()
{
    internal->close();
}

void AudioProcessor::getFileData(char **data, int *dataSize)
{
    internal->getFileData(data, dataSize);
}

void AudioProcessor::getFileData(char **data, int *dataSize, bool *end, int *progress, int readDataSize)
{
    internal->getFileData(data, dataSize, end, progress, readDataSize);
}

}
