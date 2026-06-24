#include "mediamuxer.h"
#include "mediademuxer.h"
#include "mediacommon.h"
#include "Model/packetcontext.h"

extern "C" {
#include <libavutil/opt.h>
}

#include <cassert>

namespace MediaLibrary {

MediaMuxer::MediaMuxer()
{
    memset(&bufferData, 0, sizeof(BufferData));
}

bool MediaMuxer::open(const string &filePath)
{
    filePath_ = filePath;
    File file(filePath_);

    const string extension = file.getExtension();
    if (extension == ".mp4") {
        formatType_ = FORMAT_MP4_TYPE;
    } else if (extension == ".mp3") {
        formatType_ = FORMAT_MP3_TYPE;
    } else if (extension == ".flac") {
        formatType_ = FORMAT_FLAC_TYPE;
    } else if (extension == ".ogg") {
        formatType_ = FORMAT_OGG_TYPE;
    } else if (extension == ".wav") {
        formatType_ = FORMAT_WAV_TYPE;
    } else if (extension == ".m4a") {
        formatType_ = FORMAT_M4A_TYPE;
    } else if (extension == ".gif") {
        formatType_ = FORMAT_GIF_TYPE;
    } else if (extension == ".jpg") {
        formatType_ = FORMAT_JPG_TYPE;
    } else if (extension == ".png") {
        formatType_ = FORMAT_PNG_TYPE;
    } else if (extension == ".bmp") {
        formatType_ = FORMAT_BMP_TYPE;
    } else if (extension == ".webp") {
        formatType_ = FORMAT_WEBP_TYPE;
    } else {
        assert(0);
    }

    switch (formatType_) {
    case FORMAT_MP4_TYPE:
    case FORMAT_MP3_TYPE:
    case FORMAT_FLAC_TYPE:
    case FORMAT_OGG_TYPE:
    case FORMAT_WAV_TYPE:
    case FORMAT_M4A_TYPE:
        mediaType_ = MEDIA_VIDEO_AUDIO_TYPE;
        break;
    case FORMAT_GIF_TYPE:
        mediaType_ = MEDIA_MOTION_PICTURE_TYPE;
        break;
    case FORMAT_JPG_TYPE:
    case FORMAT_PNG_TYPE:
    case FORMAT_BMP_TYPE:
    case FORMAT_WEBP_TYPE:
        mediaType_ = MEDIA_STILL_PICTURE_TYPE;
        break;
    default:
        break;
    }

    switch (muxerType_) {
    case MUXER_FFMPEG_TYPE: {
        AVFormatContext* formatCtx = openOutFormatContext(filePath_);
        if (!formatCtx) {
            return false;
        }
        formatCtx_ = formatCtx;
        writeTrailer_ = false;
    }
        break;
    case MUXER_NATIVE_TYPE: {
    }
        break;
    default:
        assert(0);
        break;
    }

    return true;
}

bool MediaMuxer::open2(const string &filePath)
{
    filePath_ = filePath;
    File file(filePath_);

    const string extension = file.getExtension();
    if (extension == ".mp4") {
        formatType_ = FORMAT_MP4_TYPE;
    } else if (extension == ".mp3") {
        formatType_ = FORMAT_MP3_TYPE;
    } else if (extension == ".flac") {
        formatType_ = FORMAT_FLAC_TYPE;
    } else if (extension == ".ogg") {
        formatType_ = FORMAT_OGG_TYPE;
    } else if (extension == ".wav") {
        formatType_ = FORMAT_WAV_TYPE;
    } else if (extension == ".m4a") {
        formatType_ = FORMAT_M4A_TYPE;
    } else if (extension == ".gif") {
        formatType_ = FORMAT_GIF_TYPE;
    } else if (extension == ".jpg") {
        formatType_ = FORMAT_JPG_TYPE;
    } else if (extension == ".png") {
        formatType_ = FORMAT_PNG_TYPE;
    } else if (extension == ".bmp") {
        formatType_ = FORMAT_BMP_TYPE;
    } else if (extension == ".webp") {
        formatType_ = FORMAT_WEBP_TYPE;
    } else {
        assert(0);
    }

    switch (formatType_) {
    case FORMAT_MP4_TYPE:
    case FORMAT_MP3_TYPE:
    case FORMAT_FLAC_TYPE:
    case FORMAT_OGG_TYPE:
    case FORMAT_WAV_TYPE:
    case FORMAT_M4A_TYPE:
        mediaType_ = MEDIA_VIDEO_AUDIO_TYPE;
        break;
    case FORMAT_GIF_TYPE:
        mediaType_ = MEDIA_MOTION_PICTURE_TYPE;
        break;
    case FORMAT_JPG_TYPE:
    case FORMAT_PNG_TYPE:
    case FORMAT_BMP_TYPE:
    case FORMAT_WEBP_TYPE:
        mediaType_ = MEDIA_STILL_PICTURE_TYPE;
        break;
    default:
        break;
    }

    switch (muxerType_) {
    case MUXER_FFMPEG_TYPE: {
        memset(&bufferData, 0, sizeof(BufferData));
        AVFormatContext* formatCtx = openOutFormatContext(filePath_, formatType_, &bufferData);
        if (!formatCtx) {
            return false;
        }
        formatCtx_ = formatCtx;
        writeTrailer_ = false;
    }
        break;
    case MUXER_NATIVE_TYPE: {
    }
        break;
    default:
        assert(0);
        break;
    }

    return true;
}

bool MediaMuxer::open(const FormatType &formatType)
{
    formatType_ = formatType;

    switch (formatType_) {
    case FORMAT_MP4_TYPE:
    case FORMAT_MP3_TYPE:
    case FORMAT_FLAC_TYPE:
    case FORMAT_OGG_TYPE:
    case FORMAT_WAV_TYPE:
    case FORMAT_M4A_TYPE:
        mediaType_ = MEDIA_VIDEO_AUDIO_TYPE;
        break;
    case FORMAT_GIF_TYPE:
        mediaType_ = MEDIA_MOTION_PICTURE_TYPE;
        break;
    case FORMAT_JPG_TYPE:
    case FORMAT_PNG_TYPE:
    case FORMAT_BMP_TYPE:
    case FORMAT_WEBP_TYPE:
        mediaType_ = MEDIA_STILL_PICTURE_TYPE;
        break;
    default:
        break;
    }

    switch (muxerType_) {
    case MUXER_FFMPEG_TYPE: {
        memset(&bufferData, 0, sizeof(BufferData));
        AVFormatContext* formatCtx = openOutFormatContext(formatType_, &bufferData);
        if (!formatCtx) {
            return false;
        }
        formatCtx_ = formatCtx;
        writeTrailer_ = false;
    }
        break;
    case MUXER_NATIVE_TYPE: {
    }
        break;
    default:
        assert(0);
        break;
    }

    return true;
}

bool MediaMuxer::open(const FormatType &formatType, void *opaque, int (*writePacket)(void *, uint8_t *, int), int64_t (*seek)(void*, int64_t, int))
{
    formatType_ = formatType;

    switch (formatType_) {
    case FORMAT_MP4_TYPE:
    case FORMAT_MP3_TYPE:
    case FORMAT_FLAC_TYPE:
    case FORMAT_OGG_TYPE:
    case FORMAT_WAV_TYPE:
    case FORMAT_M4A_TYPE:
        mediaType_ = MEDIA_VIDEO_AUDIO_TYPE;
        break;
    case FORMAT_GIF_TYPE:
        mediaType_ = MEDIA_MOTION_PICTURE_TYPE;
        break;
    case FORMAT_JPG_TYPE:
    case FORMAT_PNG_TYPE:
    case FORMAT_BMP_TYPE:
    case FORMAT_WEBP_TYPE:
        mediaType_ = MEDIA_STILL_PICTURE_TYPE;
        break;
    default:
        break;
    }

    switch (muxerType_) {
    case MUXER_FFMPEG_TYPE: {
        AVFormatContext* formatCtx = openOutFormatContext(formatType_, opaque, writePacket, seek);
        if (!formatCtx) {
            return false;
        }
        formatCtx_ = formatCtx;
        writeTrailer_ = false;
    }
        break;
    case MUXER_NATIVE_TYPE: {
    }
        break;
    default:
        assert(0);
        break;
    }

    return true;
}

void MediaMuxer::addStream(MediaDemuxer *mediaDemuxer, const int streamIndex)
{
    switch (muxerType_) {
    case MuxerType::MUXER_FFMPEG_TYPE: {
        if (!formatCtx_) {
            break;
        }
        AVFormatContext* inFormatCtx = static_cast<AVFormatContext*>(mediaDemuxer->getFormatContext());
        for (unsigned int i = 0; i < inFormatCtx->nb_streams; i++) {
            if (i != static_cast<unsigned int>(streamIndex)) {
                continue;
            }
            AVStream* inStream = inFormatCtx->streams[i];
            AVFormatContext* formatCtx = static_cast<AVFormatContext*>(formatCtx_);
            AVStream* stream = avformat_new_stream(formatCtx, nullptr);
            avcodec_parameters_copy(stream->codecpar, inStream->codecpar);

            switch (formatType_) {
            case FORMAT_FLAC_TYPE: {
                if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                    stream->disposition |= AV_DISPOSITION_ATTACHED_PIC;
                }
            }
                break;
            default:
                break;
            }
        }
    }
        break;
    default:
        break;
    }
}

bool MediaMuxer::start(const int loop)
{
    bool result = false;

    switch (muxerType_) {
    case MuxerType::MUXER_FFMPEG_TYPE: {
        if (!formatCtx_) {
            break;
        }
        int ret = 0;
        AVFormatContext* formatCtx = static_cast<AVFormatContext*>(formatCtx_);
        if (!filePath_.empty()) {
            if ((ret = avio_open(&formatCtx->pb, filePath_.c_str(), AVIO_FLAG_WRITE)) < 0) {
                break;
            }
        }

        AVDictionary *opts = nullptr;
        switch (formatType_) {
        case FORMAT_GIF_TYPE: {
            av_dict_set(&opts, "loop", std::to_string(loop).c_str(), 0);
        }
            break;
        case FORMAT_MP3_TYPE: {
            av_opt_set_int(formatCtx, "write_xing", 0, AV_OPT_SEARCH_CHILDREN);
        }
            break;
        default:
            break;
        }
        if ((ret = avformat_write_header(formatCtx, &opts)) < 0) {
            break;
        }
        (void)ret;
        result = true;
    }
        break;
    default:
        break;
    }

    return result;
}

bool MediaMuxer::finish()
{
    bool result = false;

    switch (muxerType_) {
    case MuxerType::MUXER_FFMPEG_TYPE: {
        if (writeTrailer_) {
            break;
        }
        if (!formatCtx_) {
            break;
        }
        int ret = 0;
        AVFormatContext* formatCtx = static_cast<AVFormatContext*>(formatCtx_);
        writeTrailer_ = true;
        if ((ret = av_write_trailer(formatCtx)) < 0) {
            break;
        }
        if (!((formatCtx->flags & AVFMT_NOFILE) || (formatCtx->flags & AVFMT_FLAG_CUSTOM_IO))) {
            avio_closep(&formatCtx->pb);
        }
        (void)ret;
        result = true;
    }
        break;
    default:
        break;
    }

    return result;
}

void MediaMuxer::close()
{
    switch (muxerType_) {
    case MuxerType::MUXER_FFMPEG_TYPE: {
        if (!formatCtx_) {
            break;
        }
        AVFormatContext* formatCtx = static_cast<AVFormatContext*>(formatCtx_);
        closeOutFormatContext(formatCtx);
        formatCtx_ = nullptr;

        if (bufferData.file) {
            fclose(bufferData.file);
            bufferData.file = nullptr;
        }

        if (bufferData.buf) {
            av_free(bufferData.buf);
            bufferData.buf = nullptr;
        }
    }
        break;
    default:
        break;
    }
}

bool MediaMuxer::write(PacketContext *packetCtx)
{
    bool result = false;

    switch (muxerType_) {
    case MuxerType::MUXER_FFMPEG_TYPE: {
        if (!packetCtx || !formatCtx_) {
            break;
        }
        AVFormatContext* formatCtx = static_cast<AVFormatContext*>(formatCtx_);
        const int streamIndex = [&packetCtx, &formatCtx](){
            switch (packetCtx->getPacketType()) {
            case PacketContext::PACKET_VIDEO_TYPE: {
                return getDefaultVideoIndex(formatCtx);
            }
                break;
            case PacketContext::PACKET_AUDIO_TYPE: {
                return getDefaultAudioIndex(formatCtx);
            }
                break;
            default:
                break;
            }
            return -1;
        }();
        if (streamIndex < 0) {
            assert(0);
            break;
        }
        AVPacket* packet = packetCtx->getPacket();
        packet->stream_index = streamIndex;
        AVRational defaultTimeBase;
        defaultTimeBase.num = 1;
        defaultTimeBase.den = AV_TIME_BASE;
        av_packet_rescale_ts(packet, defaultTimeBase, formatCtx->streams[streamIndex]->time_base);
        if ((av_write_frame(formatCtx, packet)) < 0) {
            assert(0);
            break;
        }
        result = true;
    }
        break;
    default:
        break;
    }

    return result;
}

int MediaMuxer::getAudioCodecId() const
{
    int audioCodecId = 0;

    switch (formatType_) {
    case FORMAT_MP4_TYPE:
        audioCodecId = AV_CODEC_ID_AAC;
        break;
    case FORMAT_MP3_TYPE:
        audioCodecId = AV_CODEC_ID_MP3;
        break;
    case FORMAT_FLAC_TYPE:
        audioCodecId = AV_CODEC_ID_FLAC;
        break;
    case FORMAT_OGG_TYPE:
        audioCodecId = AV_CODEC_ID_VORBIS;
        break;
    case FORMAT_WAV_TYPE:
        audioCodecId = AV_CODEC_ID_PCM_S16LE;
        break;
    case FORMAT_M4A_TYPE:
        audioCodecId = AV_CODEC_ID_AAC;
        break;
    case FORMAT_GIF_TYPE:
    case FORMAT_JPG_TYPE:
    case FORMAT_PNG_TYPE:
    case FORMAT_BMP_TYPE:
    case FORMAT_WEBP_TYPE:
        audioCodecId = AV_CODEC_ID_NONE;
        break;
    default:
        break;
    }

    return audioCodecId;
}

void MediaMuxer::setMetadatas(const std::map<MetadataType, string> &metadatas)
{
    switch (muxerType_) {
    case MuxerType::MUXER_FFMPEG_TYPE: {
        if (!formatCtx_) {
            break;
        }
        AVFormatContext* formatCtx = static_cast<AVFormatContext*>(formatCtx_);
        for (const auto& metadata : metadatas) {
            const MetadataType& metadataKey = metadata.first;
            const string& metadataValue = metadata.second;
            switch (metadataKey) {
            case METADATA_TITLE_TYPE: {
                av_dict_set(&formatCtx->metadata, "title", metadataValue.c_str(), AV_DICT_DONT_OVERWRITE);
            }
                break;
            case METADATA_ARTIST_TYPE: {
                av_dict_set(&formatCtx->metadata, "artist", metadataValue.c_str(), AV_DICT_DONT_OVERWRITE);
            }
                break;
            case METADATA_ALBUM_TYPE: {
                av_dict_set(&formatCtx->metadata, "album", metadataValue.c_str(), AV_DICT_DONT_OVERWRITE);
            }
                break;
            case METADATA_COMMENT_TYPE: {
                av_dict_set(&formatCtx->metadata, "comment", metadataValue.c_str(), AV_DICT_DONT_OVERWRITE);
            }
                break;
            case METADATA_DATE_TYPE: {
                av_dict_set(&formatCtx->metadata, "date", metadataValue.c_str(), AV_DICT_DONT_OVERWRITE);
            }
                break;
            case METADATA_TRACK_TYPE: {
                av_dict_set(&formatCtx->metadata, "track", metadataValue.c_str(), AV_DICT_DONT_OVERWRITE);
            }
                break;
            case METADATA_GENRE_TYPE: {
                av_dict_set(&formatCtx->metadata, "genre", metadataValue.c_str(), AV_DICT_DONT_OVERWRITE);
            }
                break;
            default:
                break;
            }
        }
    }
        break;
    default:
        break;
    }
}

bool MediaMuxer::haveVideo() const
{
    switch (formatType_) {
    case FORMAT_JPG_TYPE:
    case FORMAT_PNG_TYPE:
    case FORMAT_BMP_TYPE:
    case FORMAT_WEBP_TYPE:
    case FORMAT_GIF_TYPE:
    case FORMAT_MP4_TYPE:
        return true;
        break;
    case FORMAT_MP3_TYPE:
    case FORMAT_FLAC_TYPE:
    case FORMAT_OGG_TYPE:
    case FORMAT_WAV_TYPE:
    case FORMAT_M4A_TYPE:
        return false;
        break;
    default:
        break;
    }
    return false;
}

bool MediaMuxer::haveAudio() const
{
    switch (formatType_) {
    case FORMAT_JPG_TYPE:
    case FORMAT_PNG_TYPE:
    case FORMAT_BMP_TYPE:
    case FORMAT_WEBP_TYPE:
    case FORMAT_GIF_TYPE:
        return false;
        break;
    case FORMAT_MP4_TYPE:
    case FORMAT_MP3_TYPE:
    case FORMAT_FLAC_TYPE:
    case FORMAT_OGG_TYPE:
    case FORMAT_WAV_TYPE:
    case FORMAT_M4A_TYPE:
        return true;
        break;
    default:
        break;
    }
    return false;
}

}
