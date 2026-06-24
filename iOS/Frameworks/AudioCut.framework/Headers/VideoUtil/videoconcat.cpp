#include "videoconcat.h"

namespace MediaLibrary {

VideoConcat::VideoConcat()
{
    internal.reset(new VideoConcatInternal());
}

int VideoConcat::open()
{
    return internal->open();
}

int VideoConcat::append(char *inData, int inDataSize)
{
    return internal->append(inData, inDataSize);
}

int VideoConcat::append2(char *inData, int inDataSize)
{
    return internal->append2(inData, inDataSize);
}

void VideoConcat::process2(bool *end, int *progress)
{
    internal->process2(end, progress);
}

void VideoConcat::cancel()
{
    internal->cancel();
}

void VideoConcat::close()
{
    internal->close();
}

void VideoConcat::getFileData(char **data, int *dataSize)
{
    internal->getFileData(data, dataSize);
}

void VideoConcat::getFileData(char **data, int *dataSize, bool *end, int *progress, int readDataSize)
{
    internal->getFileData(data, dataSize, end, progress, readDataSize);
}

}
