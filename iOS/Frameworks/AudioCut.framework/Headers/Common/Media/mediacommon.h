#ifndef MEDIACOMMON_H
#define MEDIACOMMON_H

#include "Media/mediainfo.h"

#include <string>

using namespace std;

extern "C" {
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

namespace MediaLibrary {

AVFormatContext *openInFormatContext(const string& filePath);
AVFormatContext *openInFormatContext(const string& filePath, BufferData* bufferData);
AVFormatContext *openInFormatContext(const char* data, const int dataSize);
AVFormatContext *openInFormatContext(const char* data, const int dataSize, BufferData* bufferData, int* error);
AVFormatContext *openInFormatContext(const char* data, const int dataSize, BufferData2* bufferData);
AVFormatContext *openInFormatContext(void* opaque, int (*readPacket)(void*, uint8_t*, int), int64_t (*seek)(void*, int64_t, int));
AVFormatContext *openOutFormatContext(const string& filePath);
AVFormatContext *openOutFormatContext(const string& filePath, const FormatType& formatType, BufferData* bufferData);
AVFormatContext *openOutFormatContext(const FormatType& formatType, BufferData* bufferData);
AVFormatContext *openOutFormatContext(const FormatType& formatType, void* opaque, int (*writePacket)(void *opaque, uint8_t *buffer, int bufferSize), int64_t (*seek)(void*, int64_t, int));
void closeInFormatContext(AVFormatContext *formatCtx);
void closeOutFormatContext(AVFormatContext *formatCtx);

int getDefaultVideoIndex(const AVFormatContext* const formatCtx);
int getDefaultAudioIndex(const AVFormatContext* const formatCtx);

int64_t getVideoDuration(AVFormatContext *formatCtx);
int64_t getAudioDuration(AVFormatContext *formatCtx);

float getFPS(AVFormatContext *formatCtx);
float getRealFPS(AVFormatContext *formatCtx);

int getRotate(AVFormatContext *formatCtx);

}

#endif // MEDIACOMMON_H
