#include "mediautil.h"
#include "mediautilinternal.h"

namespace MediaLibrary {

MediaUtil::MediaUtil()
{
    internal.reset(new MediaUtilInternal());
}

bool MediaUtil::transcode(const string &inFilePath, const string &outFilePath, const int64_t startTime, const int64_t endTime, const VideoFormat &videoFormat, const AudioFormat &audioFormat, const std::map<TaskType, int>& extraTasks, const MediaUtilDelegate *delegate)
{
    return internal->transcode(inFilePath, outFilePath, startTime, endTime, videoFormat, audioFormat, extraTasks, delegate);
}

bool MediaUtil::reverse(const string &inFilePath, const string &outFilePath, const MediaUtilDelegate *delegate)
{
    return internal->reverse(inFilePath, outFilePath, delegate);
}

bool MediaUtil::memoryFile(const string &inFilePath, const string &outFilePath, const MediaUtilDelegate *delegate)
{
    return internal->memoryFile(inFilePath, outFilePath, delegate);
}

const string MediaUtil::checkSupportFormatCodec()
{
    return internal->checkSupportFormatCodec();
}

MediaInfo MediaUtil::checkMediaInfo(const string &filePath)
{
    return internal->checkMediaInfo(filePath);
}

void MediaUtil::cancel()
{
    internal->cancel();
}

}
