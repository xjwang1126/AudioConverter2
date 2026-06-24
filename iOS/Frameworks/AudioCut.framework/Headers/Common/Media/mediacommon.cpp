#include "mediacommon.h"
#include "Model/frameinfo.h"

namespace MediaLibrary {

AVFormatContext *openInFormatContext(const string &filePath)
{
    AVFormatContext *formatCtx = nullptr;
    if (filePath.empty()) {
        return nullptr;
    }

    int ret = 0;
    do {
        formatCtx = avformat_alloc_context();
        if (!formatCtx) {
            ret = AVERROR(ENOMEM);
            break;
        }

        if ((ret = avformat_open_input(&formatCtx, filePath.c_str(), nullptr, nullptr)) < 0) {
            break;
        }

        if ((ret = avformat_find_stream_info(formatCtx, nullptr)) < 0) {
            break;
        }
    } while(false);

    if (ret < 0) {
        closeInFormatContext(formatCtx);
        formatCtx = nullptr;
    }

    return formatCtx;
}

AVFormatContext *openInFormatContext(const string &filePath, BufferData *bufferData)
{
    AVFormatContext *formatCtx = nullptr;
    if (filePath.empty()) {
        return nullptr;
    }

    int ret = 0;
    do {
        auto readPacketFunc = [](void *opaque, uint8_t *buffer, int bufferSize)->int {
            BufferData* bufferData = static_cast<BufferData*>(opaque);
            size_t bytesRead = fread(buffer, 1, bufferSize, bufferData->file);
            return (int)bytesRead;
        };
        auto seekFunc = [](void *opaque, int64_t offset, int whence)->int64_t {
            BufferData* bufferData = static_cast<BufferData*>(opaque);
            int stdWhence;
            switch (whence) {
                case SEEK_SET: stdWhence = SEEK_SET; break;
                case SEEK_CUR: stdWhence = SEEK_CUR; break;
                case SEEK_END: stdWhence = SEEK_END; break;
                case AVSEEK_SIZE: return bufferData->fileSize;
                default: return -1;
            }
            if (fseek(bufferData->file, offset, stdWhence) != 0) {
                return -1;
            }
            return ftell(bufferData->file);
        };

        FILE* file = fopen(filePath.c_str(), "rb");
        if (!file) {
            break;
        }

        fseek(file, 0, SEEK_END);
        int64_t fileSize = ftell(file);
        fseek(file, 0, SEEK_SET);

        bufferData->file = file;
        bufferData->fileSize = fileSize;

        const int bufferSize = 4096;
        uint8_t *AVIOCtxBuffer = static_cast<uint8_t*>(av_malloc(bufferSize));
        if (!AVIOCtxBuffer) {
            ret = AVERROR(ENOMEM);
            break;
        }

        AVIOContext *AVIOCtx = avio_alloc_context(
            AVIOCtxBuffer, bufferSize, 0, bufferData, readPacketFunc, nullptr, seekFunc);
        if (!AVIOCtx) {
            ret = AVERROR(ENOMEM);
            break;
        }

        formatCtx = avformat_alloc_context();
        if (!formatCtx) {
            ret = AVERROR(ENOMEM);
            break;
        }

        formatCtx->pb = AVIOCtx;
        formatCtx->flags |= AVFMT_FLAG_CUSTOM_IO;

        if ((ret = avformat_open_input(&formatCtx, nullptr, nullptr, nullptr)) < 0) {
            break;
        }

        if ((ret = avformat_find_stream_info(formatCtx, nullptr)) < 0) {
            break;
        }
    } while(false);

    if (ret < 0) {
        closeInFormatContext(formatCtx);
        formatCtx = nullptr;
    }

    return formatCtx;
}

AVFormatContext *openInFormatContext(const char *data, const int dataSize)
{
    AVFormatContext* formatCtx = nullptr;
    AVIOContext* AVIOCtx = nullptr;
    if (!data || !dataSize) {
        return nullptr;
    }

    int ret = 0;
    do {
        formatCtx = avformat_alloc_context();
        if (!formatCtx) {
            ret = AVERROR(ENOMEM);
            break;
        }

        AVIOCtx = avio_alloc_context((unsigned char*)data, dataSize, 0, nullptr, nullptr, nullptr, nullptr);
        if (!AVIOCtx) {
            ret = AVERROR(ENOMEM);
            break;
        }

        formatCtx->pb = AVIOCtx;

        const char *inFilename = "dummy";
        if ((ret = avformat_open_input(&formatCtx, inFilename, nullptr, nullptr)) < 0) {
            break;
        }

        if ((ret = avformat_find_stream_info(formatCtx, nullptr)) < 0) {
            break;
        }

#if 0
        av_dump_format(formatCtx, 0, inFilename, 0);
#endif
    } while (false);

    if (ret < 0) {
        closeInFormatContext(formatCtx);
        formatCtx = nullptr;

        if (AVIOCtx) {
            avio_context_free(&AVIOCtx);
            AVIOCtx = nullptr;
        }
    }

    return formatCtx;
}

AVFormatContext *openInFormatContext(const char *data, const int dataSize, BufferData *bufferData, int *error)
{
    AVFormatContext* formatCtx = nullptr;
    AVIOContext* AVIOCtx = nullptr;
    if (!data || !dataSize) {
        *error = 41;
        return nullptr;
    }

    int ret = 0;
    do {
        formatCtx = avformat_alloc_context();
        if (!formatCtx) {
            *error = 42;
            ret = AVERROR(ENOMEM);
            break;
        }

        auto readPacketFunc = [](void *opaque, uint8_t *buffer, int bufferSize)->int {
            struct BufferData *bufferData = (struct BufferData *)opaque;
            bufferSize = std::min(bufferSize, bufferData->size);
            memcpy(buffer, bufferData->ptr, bufferSize);
            bufferData->ptr  += bufferSize;
            bufferData->size -= bufferSize;
            return bufferSize;
        };
        AVIOCtx = avio_alloc_context((unsigned char*)data, dataSize, 0, bufferData, readPacketFunc, nullptr, nullptr);
        if (!AVIOCtx) {
            *error = 43;
            ret = AVERROR(ENOMEM);
            break;
        }

        formatCtx->pb = AVIOCtx;

        const char *inFilename = "dummy";
        if ((ret = avformat_open_input(&formatCtx, inFilename, nullptr, nullptr)) < 0) {
            *error = ret;
            break;
        }

        if ((ret = avformat_find_stream_info(formatCtx, nullptr)) < 0) {
            *error = 45;
            break;
        }

#if 0
        av_dump_format(formatCtx, 0, inFilename, 0);
#endif
    } while (false);

    if (ret < 0) {
        closeInFormatContext(formatCtx);
        formatCtx = nullptr;

        if (AVIOCtx) {
            avio_context_free(&AVIOCtx);
            AVIOCtx = nullptr;
        }
    }

    return formatCtx;
}

AVFormatContext *openInFormatContext(const char *data, const int dataSize, BufferData2 *bufferData)
{
    AVFormatContext* formatCtx = nullptr;
    AVIOContext* AVIOCtx = nullptr;
    if (!data || !dataSize) {
        return nullptr;
    }

    int ret = 0;
    do {
        formatCtx = avformat_alloc_context();
        if (!formatCtx) {
            ret = AVERROR(ENOMEM);
            break;
        }

        auto readPacketFunc = [](void *opaque, uint8_t *buffer, int bufferSize)->int {
            struct BufferData2 *bufferData = (struct BufferData2 *)opaque;

            int remaining = bufferData->size - bufferData->pos;
            if (remaining <= 0) {
                return AVERROR_EOF;
            }

            int readSize = (bufferSize < remaining) ? bufferSize : remaining;
            memcpy(buffer, bufferData->ptr + bufferData->pos, readSize);
            bufferData->pos += readSize;

            return readSize;
        };
        auto seekPacketFunc = [](void *opaque, int64_t offset, int whence)->int64_t {
            struct BufferData2 *bufferData = (struct BufferData2 *)opaque;

            switch (whence) {
                case SEEK_SET:
                    bufferData->pos = offset;
                    break;
                case SEEK_CUR:
                    bufferData->pos += offset;
                    break;
                case SEEK_END:
                    bufferData->pos = bufferData->size + offset;
                    break;
                default:
                    return -1;
            }

            if (bufferData->pos < 0) bufferData->pos = 0;
            if (bufferData->pos > bufferData->size) bufferData->pos = bufferData->size;

            return bufferData->pos;
        };

        AVIOCtx = avio_alloc_context((unsigned char*)data, dataSize, 0, bufferData, readPacketFunc, nullptr, seekPacketFunc);
        if (!AVIOCtx) {
            ret = AVERROR(ENOMEM);
            break;
        }

        formatCtx->pb = AVIOCtx;

        const char *inFilename = "dummy";
        if ((ret = avformat_open_input(&formatCtx, inFilename, nullptr, nullptr)) < 0) {
            break;
        }

        if ((ret = avformat_find_stream_info(formatCtx, nullptr)) < 0) {
            break;
        }

#if 0
        av_dump_format(formatCtx, 0, inFilename, 0);
#endif
    } while (false);

    if (ret < 0) {
        closeInFormatContext(formatCtx);
        formatCtx = nullptr;

        if (AVIOCtx) {
            avio_context_free(&AVIOCtx);
            AVIOCtx = nullptr;
        }
    }

    return formatCtx;
}

AVFormatContext *openInFormatContext(void *opaque, int (*readPacket)(void *, uint8_t *, int), int64_t (*seek)(void *, int64_t, int))
{
    AVFormatContext* formatCtx = nullptr;
    AVIOContext* AVIOCtx = nullptr;

    int ret = 0;
    do {
        formatCtx = avformat_alloc_context();
        if (!formatCtx) {
            ret = AVERROR(ENOMEM);
            break;
        }

        const int bufferSize = 4096;
        uint8_t *AVIOCtxBuffer = static_cast<uint8_t*>(av_malloc(bufferSize));
        if (!AVIOCtxBuffer) {
            ret = AVERROR(ENOMEM);
            break;
        }

        AVIOCtx = avio_alloc_context(AVIOCtxBuffer, bufferSize, 0, opaque, readPacket, nullptr, seek);
        if (!AVIOCtx) {
            ret = AVERROR(ENOMEM);
            break;
        }

        formatCtx->pb = AVIOCtx;

        const char *inFilename = "dummy";
        if ((ret = avformat_open_input(&formatCtx, inFilename, nullptr, nullptr)) < 0) {
            break;
        }

        if ((ret = avformat_find_stream_info(formatCtx, nullptr)) < 0) {
            break;
        }

#if 0
        av_dump_format(formatCtx, 0, inFilename, 0);
#endif
    } while (false);

    if (ret < 0) {
        closeInFormatContext(formatCtx);
        formatCtx = nullptr;

        if (AVIOCtx) {
            avio_context_free(&AVIOCtx);
            AVIOCtx = nullptr;
        }
    }

    return formatCtx;
}

AVFormatContext *openOutFormatContext(const string &filePath)
{
    AVFormatContext *formatCtx = nullptr;
    if (filePath.empty()) {
        return nullptr;
    }

    int ret = 0;
    do {
        if ((ret = avformat_alloc_output_context2(&formatCtx, nullptr, nullptr, filePath.c_str())) < 0) {
            break;
        }
    } while(false);

    if (ret < 0) {
        closeInFormatContext(formatCtx);
        formatCtx = nullptr;
    }

    return formatCtx;
}

AVFormatContext *openOutFormatContext(const string &filePath, const FormatType &formatType, BufferData *bufferData)
{
    AVFormatContext *formatCtx = nullptr;
    if (filePath.empty()) {
        return nullptr;
    }

    int ret = 0;
    do {
        auto writePacketFunc = [](void *opaque, uint8_t *buffer, int bufferSize)->int {
            BufferData* bufferData = static_cast<BufferData*>(opaque);
            size_t bytesWrite = fwrite(buffer, 1, bufferSize, bufferData->file);
            return (int)bytesWrite;
        };

        FILE* file = fopen(filePath.c_str(), "wb");
        if (!file) {
            break;
        }
        bufferData->file = file;

        const int bufferSize = 4096;
        uint8_t *AVIOCtxBuffer = static_cast<uint8_t*>(av_malloc(bufferSize));
        if (!AVIOCtxBuffer) {
            ret = AVERROR(ENOMEM);
            break;
        }

        AVIOContext *AVIOCtx = avio_alloc_context(AVIOCtxBuffer, bufferSize, 1, bufferData, nullptr, writePacketFunc, nullptr);
        if (!AVIOCtx) {
            ret = AVERROR(ENOMEM);
            break;
        }

        string formatName;
        switch (formatType) {
        case FORMAT_MP4_TYPE:
            formatName = "mp4";
            break;
        case FORMAT_MP3_TYPE:
            formatName = "mp3";
            break;
        case FORMAT_FLAC_TYPE:
            formatName = "flac";
            break;
        case FORMAT_OGG_TYPE:
            formatName = "ogg";
            break;
        case FORMAT_WAV_TYPE:
            formatName = "wav";
            break;
        case FORMAT_M4A_TYPE:
            formatName = "ipod";
            break;
        case FORMAT_GIF_TYPE:
            formatName = "gif";
            break;
        case FORMAT_JPG_TYPE:
            formatName = "image2pipe";
            break;
        case FORMAT_PNG_TYPE:
            formatName = "image2pipe";
            break;
        case FORMAT_BMP_TYPE:
            formatName = "image2pipe";
            break;
        case FORMAT_WEBP_TYPE:
            formatName = "image2pipe";
            break;
        default:
            break;
        }
        if ((ret = avformat_alloc_output_context2(&formatCtx, nullptr, formatName.c_str(), nullptr)) < 0) {
            break;
        }

        formatCtx->pb = AVIOCtx;
        formatCtx->flags |= AVFMT_FLAG_CUSTOM_IO;
    } while(false);

    if (ret < 0) {
        closeInFormatContext(formatCtx);
        formatCtx = nullptr;
    }

    return formatCtx;
}

AVFormatContext *openOutFormatContext(const FormatType &formatType, BufferData *bufferData)
{
    AVFormatContext *formatCtx = nullptr;
    AVIOContext *AVIOCtx = nullptr;
    if (formatType == FORMAT_NONE_TYPE || !bufferData) {
        return nullptr;
    }

    const size_t bufferSize = 1024;
    const size_t outAVIOCtxBufferSize = 4096;

    int ret = 0;
    do {
        bufferData->ptr = bufferData->buf = static_cast<uint8_t*>(av_malloc(bufferSize));
        if (!bufferData->buf) {
            ret = AVERROR(ENOMEM);
            break;
        }
        bufferData->size = bufferData->room = bufferSize;

        uint8_t *outAVIOCtxBuffer = static_cast<uint8_t*>(av_malloc(outAVIOCtxBufferSize));
        if (!outAVIOCtxBuffer) {
            ret = AVERROR(ENOMEM);
            break;
        }

        auto readPacketFunc = [](void *opaque, uint8_t *buffer, int bufferSize)->int {
            (void)opaque;(void)buffer;(void)bufferSize;
            return 0;
        };
        auto writePacketFunc = [](void *opaque, uint8_t *buffer, int bufferSize)->int {
            struct BufferData *bufferData = (struct BufferData *)opaque;
            while (static_cast<size_t>(bufferSize) > bufferData->room) {
                int64_t offset = bufferData->ptr - bufferData->buf;
                bufferData->buf = static_cast<uint8_t*>(av_realloc_f(bufferData->buf, 2, bufferData->size));
                if (!bufferData->buf) {
                    return AVERROR(ENOMEM);
                }
                bufferData->size *= 2;
                bufferData->ptr = bufferData->buf + offset;
                bufferData->room = bufferData->size - offset;
            }

            memcpy(bufferData->ptr, buffer, bufferSize);
            bufferData->ptr  += bufferSize;
            bufferData->room -= bufferSize;

            return bufferSize;
        };
        auto seekFunc = [](void *opaque, int64_t offset, int whence)->int64_t {
            struct BufferData *bufferData = (struct BufferData *)opaque;
            switch(whence) {
                case SEEK_SET:
                    bufferData->ptr = bufferData->buf + offset;
                    break;
                case SEEK_CUR:
                    bufferData->ptr += offset;
                    break;
                case SEEK_END:
                    bufferData->ptr = (bufferData->buf + bufferData->size) + offset;
                    break;
                case AVSEEK_SIZE:
                    return bufferData->size;
                    break;
                default:
                   return -1;
            }
            return 1;
        };
        AVIOCtx = avio_alloc_context(outAVIOCtxBuffer, outAVIOCtxBufferSize, 1, bufferData, readPacketFunc, writePacketFunc, seekFunc);
        if (!AVIOCtx) {
            ret = AVERROR(ENOMEM);
            break;
        }

        string formatName;
        switch (formatType) {
        case FORMAT_MP4_TYPE:
            formatName = "mp4";
            break;
        case FORMAT_MP3_TYPE:
            formatName = "mp3";
            break;
        case FORMAT_FLAC_TYPE:
            formatName = "flac";
            break;
        case FORMAT_OGG_TYPE:
            formatName = "ogg";
            break;
        case FORMAT_WAV_TYPE:
            formatName = "wav";
            break;
        case FORMAT_M4A_TYPE:
            formatName = "ipod";
            break;
        case FORMAT_GIF_TYPE:
            formatName = "gif";
            break;
        case FORMAT_JPG_TYPE:
            formatName = "image2pipe";
            break;
        case FORMAT_PNG_TYPE:
            formatName = "image2pipe";
            break;
        case FORMAT_BMP_TYPE:
            formatName = "image2pipe";
            break;
        case FORMAT_WEBP_TYPE:
            formatName = "image2pipe";
            break;
        default:
            break;
        }
        if ((ret = avformat_alloc_output_context2(&formatCtx, nullptr, formatName.c_str(), nullptr)) < 0) {
            break;
        }

        formatCtx->pb = AVIOCtx;
        formatCtx->flags |= AVFMT_FLAG_CUSTOM_IO;
    } while (false);

    return formatCtx;
}

AVFormatContext *openOutFormatContext(const FormatType &formatType, void *opaque, int (*writePacket)(void *, uint8_t *, int), int64_t (*seek)(void *, int64_t, int))
{
    AVFormatContext *formatCtx = nullptr;
    AVIOContext *AVIOCtx = nullptr;
    if (formatType == FORMAT_NONE_TYPE) {
        return nullptr;
    }

    const size_t outAVIOCtxBufferSize = 4096;

    int ret = 0;
    do {
        uint8_t *outAVIOCtxBuffer = static_cast<uint8_t*>(av_malloc(outAVIOCtxBufferSize));
        if (!outAVIOCtxBuffer) {
            ret = AVERROR(ENOMEM);
            break;
        }

        AVIOCtx = avio_alloc_context(outAVIOCtxBuffer, outAVIOCtxBufferSize, 1, opaque, nullptr, writePacket, seek);
        if (!AVIOCtx) {
            ret = AVERROR(ENOMEM);
            break;
        }

        string formatName;
        switch (formatType) {
        case FORMAT_MP4_TYPE:
            formatName = "mp4";
            break;
        case FORMAT_MP3_TYPE:
            formatName = "mp3";
            break;
        case FORMAT_FLAC_TYPE:
            formatName = "flac";
            break;
        case FORMAT_OGG_TYPE:
            formatName = "ogg";
            break;
        case FORMAT_WAV_TYPE:
            formatName = "wav";
            break;
        case FORMAT_M4A_TYPE:
            formatName = "ipod";
            break;
        case FORMAT_GIF_TYPE:
            formatName = "gif";
            break;
        case FORMAT_JPG_TYPE:
            formatName = "image2pipe";
            break;
        case FORMAT_PNG_TYPE:
            formatName = "image2pipe";
            break;
        case FORMAT_BMP_TYPE:
            formatName = "image2pipe";
            break;
        case FORMAT_WEBP_TYPE:
            formatName = "image2pipe";
            break;
        default:
            break;
        }
        if ((ret = avformat_alloc_output_context2(&formatCtx, nullptr, formatName.c_str(), nullptr)) < 0) {
            break;
        }

        formatCtx->pb = AVIOCtx;
        formatCtx->flags |= AVFMT_FLAG_CUSTOM_IO;
    } while (false);

    return formatCtx;
}

void closeInFormatContext(AVFormatContext *formatCtx)
{
    if (!formatCtx) {
        return;
    }
    if (formatCtx->pb) {
        if (formatCtx->pb->buffer) {
            av_free(formatCtx->pb->buffer);
            formatCtx->pb->buffer = nullptr;
        }
        avio_context_free(&formatCtx->pb);
    }
    avformat_close_input(&formatCtx);
}

void closeOutFormatContext(AVFormatContext *formatCtx)
{
    if (!formatCtx) {
        return;
    }
    avformat_free_context(formatCtx);
}

int getDefaultVideoIndex(const AVFormatContext * const formatCtx)
{
    for (unsigned int i = 0; i < formatCtx->nb_streams; i++) {
        AVStream* stream = formatCtx->streams[i];
        if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

int getDefaultAudioIndex(const AVFormatContext* const formatCtx)
{
    for (unsigned int i = 0; i < formatCtx->nb_streams; i++) {
        AVStream* stream = formatCtx->streams[i];
        if (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

int64_t getVideoDuration(AVFormatContext *formatCtx)
{
    int64_t videoDuration = 0;

    const int defaultVideoIndex = getDefaultVideoIndex(formatCtx);
    if (defaultVideoIndex == -1) {
        return 0;
    }

    for (unsigned int i = 0; i < formatCtx->nb_streams; i++) {
        AVStream *stream = formatCtx->streams[i];
        if (i == static_cast<unsigned int>(defaultVideoIndex)) {
            videoDuration = static_cast<int64_t>(roundf((static_cast<float>(stream->duration) * stream->time_base.num / stream->time_base.den) * SECOND_TO_MILLISECOND_UNIT));
            break;
        }
    }

    return videoDuration;
}

int64_t getAudioDuration(AVFormatContext *formatCtx)
{
    int64_t audioDuration = 0;

    const int defaultAudioIndex = getDefaultAudioIndex(formatCtx);
    if (defaultAudioIndex == -1) {
        return 0;
    }

    for (unsigned int i = 0; i < formatCtx->nb_streams; i++) {
        AVStream *stream = formatCtx->streams[i];
        if (i == static_cast<unsigned int>(defaultAudioIndex)) {
            audioDuration = static_cast<int64_t>(roundf((static_cast<float>(stream->duration) * stream->time_base.num / stream->time_base.den) * SECOND_TO_MILLISECOND_UNIT));
            break;
        }
    }

    return audioDuration;
}

float getFPS(AVFormatContext *formatCtx)
{
    if (!formatCtx) {
        return -1.0f;
    }

    const int defaultVideoIndex = getDefaultVideoIndex(formatCtx);
    if (defaultVideoIndex == -1) {
        return -1.0f;
    }

    AVStream* videoStream = formatCtx->streams[defaultVideoIndex];
    AVRational frameRate = av_guess_frame_rate(formatCtx, videoStream, nullptr);
    float fps = static_cast<float>(frameRate.num) / frameRate.den;
    return fps;
}

float getRealFPS(AVFormatContext *formatCtx)
{
    if (!formatCtx) {
        return -1.0f;
    }

    const int defaultVideoIndex = getDefaultVideoIndex(formatCtx);
    if (defaultVideoIndex == -1) {
        return -1.0f;
    }

    AVStream* videoStream = formatCtx->streams[defaultVideoIndex];
    if (!videoStream) {
        return -1.0f;
    }
    float fps = videoStream->avg_frame_rate.den == 0 ? 0.f : static_cast<float>(av_q2d(videoStream->avg_frame_rate));
    return fps;
}

int getRotate(AVFormatContext *formatCtx)
{
    int rotate = 0;

    const int defaultVideoIndex = getDefaultVideoIndex(formatCtx);
    if (defaultVideoIndex != -1) {
        AVStream* videoStream = formatCtx->streams[defaultVideoIndex];
        AVDictionaryEntry* tag = av_dict_get(videoStream->metadata, "rotate", nullptr, AV_DICT_MATCH_CASE);
        if (tag) {
            rotate = atoi(tag->value);
        }
    }

    return rotate;
}

}
