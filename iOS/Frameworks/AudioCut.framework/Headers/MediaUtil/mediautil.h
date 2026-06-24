#ifndef MEDIAUTIL_H
#define MEDIAUTIL_H

#include "mediautilinfo.h"

#include <map>
#include <memory>
#include <string>
#include <cstdint>

using namespace std;

namespace MediaLibrary {

class MediaUtilDelegate;
class MediaUtilInternal;

class MediaUtil
{
public:
    MediaUtil();

    bool transcode(const string& inFilePath, const string& outFilePath, const int64_t startTime, const int64_t endTime, const VideoFormat& videoFormat, const AudioFormat& audioFormat, const std::map<TaskType, int> &extraTasks, const MediaUtilDelegate* delegate);

    bool reverse(const string& inFilePath, const string& outFilePath, const MediaUtilDelegate* delegate);

    bool memoryFile(const string& inFilePath, const string& outFilePath, const MediaUtilDelegate* delegate);

    const string checkSupportFormatCodec();

    MediaInfo checkMediaInfo(const string& filePath);

    void cancel();

private:
    std::unique_ptr<MediaUtilInternal> internal;
};

}

#endif // MEDIAUTIL_H
