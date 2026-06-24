#include "audiogenerator.h"
#include "audiogeneratorinternal.h"

namespace MediaLibrary {

AudioGenerator::AudioGenerator()
{
    internal.reset(new AudioGeneratorInternal());
}

bool AudioGenerator::open(ReadCallback readCallback, SeekCallback demuxerSeekCallback, WriteCallback writeCallback, SeekCallback muxerSeekCallback, const std::string &formatName)
{
    return internal->open(readCallback, demuxerSeekCallback, writeCallback, muxerSeekCallback, formatName);
}

void AudioGenerator::start()
{
    internal->start();
}

void AudioGenerator::stop()
{
    internal->stop();
}

void AudioGenerator::close()
{
    internal->close();
}

}
