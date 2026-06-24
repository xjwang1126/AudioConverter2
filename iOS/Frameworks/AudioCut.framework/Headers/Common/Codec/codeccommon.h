#ifndef CODECCOMMON_H
#define CODECCOMMON_H

#include "codecinfo.h"

extern "C" {
#include <libavformat/avformat.h>
}

#include <map>

namespace MediaLibrary {

AVCodecContext* openDecoder(const AVCodecParameters* codecPar);
void closeDecoder(AVCodecContext* decoder);

AVCodecContext* openVideoEncoder(AVFormatContext* formatCtx, const std::map<CodecKey, CodecValue>& codecInfos);
void closeVideoEncoder(AVCodecContext* videoEncoder);
AVCodecContext* openAudioEncoder(AVFormatContext* formatCtx, const std::map<CodecKey, CodecValue>& codecInfos);
void closeAudioEncoder(AVCodecContext* audioEncoder);

}

#endif // CODECCOMMON_H
