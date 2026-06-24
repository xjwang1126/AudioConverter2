#ifndef AUDIOGENERATOR_H
#define AUDIOGENERATOR_H

#include <memory>
#include <string>
#include <functional>

#include "audiogeneratorinfo.h"
#include "AudioUtil/audiogeneratorinternal.h"

namespace MediaLibrary {

class AudioGeneratorInternal;

class AudioGenerator
{
public:
    AudioGenerator();

    bool open(ReadCallback readCallback, SeekCallback demuxerSeekCallback, WriteCallback writeCallback, SeekCallback muxerSeekCallback, const std::string& formatName);
    void start();
    void stop();
    void close();

private:
    std::unique_ptr<AudioGeneratorInternal> internal;
};

}

#endif // AUDIOGENERATOR_H
