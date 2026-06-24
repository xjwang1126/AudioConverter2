#include "audioutil.h"
#include "audioutilinternal.h"

namespace MediaLibrary {

AudioUtil::AudioUtil()
{
    internal.reset(new AudioUtilInternal());
}

void AudioUtil::setQuietLog()
{
    internal->setQuietLog();
}

bool AudioUtil::cut(const string &inFilePath, const string &outFilePath, const int64_t startTime, const int64_t endTime, const TimeMode &timeMode, const MediaUtilDelegate *delegate)
{
    return internal->editAudio(inFilePath, "", outFilePath, startTime, endTime, 0, 0, 1.0, timeMode, {}, {}, delegate) == 0;
}

int AudioUtil::cut(char *inData, int inDataSize, char **outData, int *outDataSize, char *formatName, const int64_t startTime, const int64_t endTime, const TimeMode &timeMode, const MediaUtilDelegate *delegate)
{
    return internal->editAudio(inData, inDataSize, nullptr, 0, outData, outDataSize, formatName, startTime, endTime, 0, 0, 1.0, timeMode, {}, {}, delegate);
}

bool AudioUtil::addCover(const string &inFilePath, const string &coverFilePath, const string &outFilePath, const MediaUtilDelegate *delegate)
{
    return internal->editAudio(inFilePath, coverFilePath, outFilePath, -1, -1, 0, 0, 1.0, TIME_CUT_MODE, {}, {}, delegate) == 0;
}

int AudioUtil::addCover(char *inData, int inDataSize, char *coverData, int coverDataSize, char **outData, int *outDataSize, char *formatName, const MediaUtilDelegate *delegate)
{
    return internal->editAudio(inData, inDataSize, coverData, coverDataSize, outData, outDataSize, formatName, -1, -1, 0, 0, 1.0, TIME_CUT_MODE, {}, {}, delegate);
}

bool AudioUtil::addMetadata(const string &inFilePath, const string &outFilePath, const std::map<MetadataType, string> &metadatas, const MediaUtilDelegate *delegate)
{
    return internal->editAudio(inFilePath, "", outFilePath, -1, -1, 0, 0, 1.0, TIME_CUT_MODE, {}, metadatas, delegate) == 0;
}

int AudioUtil::addMetadata(char *inData, int inDataSize, char **outData, int *outDataSize, char *formatName, const std::map<MetadataType, string> &metadatas, const MediaUtilDelegate *delegate)
{
    return internal->editAudio(inData, inDataSize, nullptr, 0, outData, outDataSize, formatName, -1, -1, 0, 0, 1.0, TIME_CUT_MODE, {}, metadatas, delegate);
}

bool AudioUtil::convert(const string &inFilePath, const string &outFilePath, const int64_t startTime, const int64_t endTime, const AudioFormat &audioFormat, const MediaUtilDelegate *delegate)
{
    return internal->editAudio(inFilePath, "", outFilePath, startTime, endTime, 0, 0, 1.0, TIME_CUT_MODE, audioFormat, {}, delegate) == 0;
}

int AudioUtil::convert(char *inData, int inDataSize, char **outData, int *outDataSize, char *formatName, const int64_t startTime, const int64_t endTime, const AudioFormat &audioFormat, const MediaUtilDelegate *delegate)
{
    return internal->editAudio(inData, inDataSize, nullptr, 0, outData, outDataSize, formatName, startTime, endTime, 0, 0, 1.0, TIME_CUT_MODE, audioFormat, {}, delegate);
}

int AudioUtil::edit(const string &inFilePath, const string &coverFilePath, const string &outFilePath, const int64_t startTime, const int64_t endTime, const int64_t fadeIn, const int64_t fadeOut, const float tempo, const TimeMode &timeMode, const AudioFormat &audioFormat, const std::map<MetadataType, string> &metadatas, const MediaUtilDelegate *delegate)
{
    return internal->editAudio(inFilePath, coverFilePath, outFilePath, startTime, endTime, fadeIn, fadeOut, tempo, timeMode, audioFormat, metadatas, delegate);
}

int AudioUtil::edit(char *inData, int inDataSize, char *coverData, int coverDataSize, char **outData, int *outDataSize, char *formatName, const int64_t startTime, const int64_t endTime, const int64_t fadeIn, const int64_t fadeOut, const float tempo, const TimeMode &timeMode, const AudioFormat &audioFormat, const std::map<MetadataType, string> &metadatas, const MediaUtilDelegate *delegate)
{
    return internal->editAudio(inData, inDataSize, coverData, coverDataSize, outData, outDataSize, formatName, startTime, endTime, fadeIn, fadeOut, tempo, timeMode, audioFormat, metadatas, delegate);
}

bool AudioUtil::waveform(const string &filePath, std::vector<float> &waveform, const MediaUtilDelegate *delegate)
{
    return internal->waveform(filePath, waveform, delegate);
}

int AudioUtil::waveform(char *inData, int inDataSize, float **waveform, int *waveformSize, const MediaUtilDelegate *delegate)
{
    return internal->waveform(inData, inDataSize, waveform, waveformSize, delegate);
}

int AudioUtil::testFile(char *data, int dataSize)
{
    return internal->testFile(data, dataSize);
}

int AudioUtil::getCover(char *inData, int inDataSize, char **coverData, int *coverDataSize, int *coverWidth, int *coverHeight, int *coverStride)
{
    std::map<MetadataType, string> metadatas;
    return internal->getInfo(inData, inDataSize, coverData, coverDataSize, coverWidth, coverHeight, coverStride, metadatas);
}

int AudioUtil::getCover(char *inData, int inDataSize, char **coverData, int *coverDataSize)
{
    std::map<MetadataType, string> metadatas;
    return internal->getInfo(inData, inDataSize, coverData, coverDataSize, metadatas, nullptr);
}

int AudioUtil::getMetadata(char *inData, int inDataSize, std::map<MetadataType, string> &metadatas)
{
    return internal->getInfo(inData, inDataSize, nullptr, nullptr, nullptr, nullptr, nullptr, metadatas);
}

int AudioUtil::getInfo(char *inData, int inDataSize, char **coverData, int *coverDataSize, int *coverWidth, int *coverHeight, int *coverStride, std::map<MetadataType, string> &metadatas)
{
    return internal->getInfo(inData, inDataSize, coverData, coverDataSize, coverWidth, coverHeight, coverStride, metadatas);
}

int AudioUtil::getInfo(char *inData, int inDataSize, char **coverData, int *coverDataSize, std::map<MetadataType, string> &metadatas, int64_t *duration)
{
    return internal->getInfo(inData, inDataSize, coverData, coverDataSize, metadatas, duration);
}

int AudioUtil::streamIO(const string &inFilePath, const string &outFilePath)
{
    return internal->streamIO(inFilePath, outFilePath);
}

void AudioUtil::cancel()
{
    internal->cancel();
}

}
