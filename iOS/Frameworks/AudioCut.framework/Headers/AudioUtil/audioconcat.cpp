#include "audioconcat.h"

namespace MediaLibrary {

AudioConcat::AudioConcat()
{
    internal.reset(new AudioConcatInternal());
}

int AudioConcat::open()
{
    return internal->open();
}

int AudioConcat::append(char *inData, int inDataSize)
{
    return internal->append(inData, inDataSize);
}

int AudioConcat::append2(char *inData, int inDataSize)
{
    return internal->append2(inData, inDataSize);
}

void AudioConcat::process2(bool *end, int *progress)
{
    internal->process2(end, progress);
}

void AudioConcat::cancel()
{
    internal->cancel();
}

void AudioConcat::close()
{
    internal->close();
}

void AudioConcat::getFileData(char **data, int *dataSize)
{
    internal->getFileData(data, dataSize);
}

void AudioConcat::getFileData(char **data, int *dataSize, bool *end, int *progress, int readDataSize)
{
    internal->getFileData(data, dataSize, end, progress, readDataSize);
}

}
