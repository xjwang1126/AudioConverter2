#ifndef AUDIOCONCAT_H
#define AUDIOCONCAT_H

#include <memory>

#include "AudioUtil/audioconcatinternal.h"

namespace MediaLibrary {

class AudioConcat
{
public:
    AudioConcat();

    int open();

    int append(char* inData, int inDataSize);
    int append2(char* inData, int inDataSize);
    void process2(bool* end, int* progress);

    void cancel();
    void close();

    void getFileData(char** data, int* dataSize);
    void getFileData(char** data, int* dataSize, bool* end, int* progress, int readDataSize);

private:
    std::unique_ptr<AudioConcatInternal> internal;
};

}

#endif // AUDIOCONCAT_H
