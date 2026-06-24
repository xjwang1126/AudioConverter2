#ifndef VIDEOCONCAT_H
#define VIDEOCONCAT_H

#include <memory>

#include "VideoUtil/videoconcatinternal.h"

namespace MediaLibrary {

class VideoConcat
{
public:
    VideoConcat();

    int open();

    int append(char* inData, int inDataSize);
    int append2(char* inData, int inDataSize);
    void process2(bool* end, int* progress);

    void cancel();
    void close();

    void getFileData(char** data, int* dataSize);
    void getFileData(char** data, int* dataSize, bool* end, int* progress, int readDataSize);

private:
    std::unique_ptr<VideoConcatInternal> internal;
};

}

#endif // VIDEOCONCAT_H
