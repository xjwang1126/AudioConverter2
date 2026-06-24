#include "mediademuxer.h"
#include "mediacommon.h"
#include "Model/imageframe.h"
#include "Model/packetcontext.h"
#include "Util/utilcommon.h"

#include <cassert>

namespace MediaLibrary {

bool MediaDemuxer::open(const string &filePath, const CheckType &checkType)
{
    filePath_ = filePath;

    AVFormatContext* formatCtx = nullptr;

    do {
        formatCtx = openInFormatContext(filePath);
        if (!formatCtx) {
            break;
        }
        formatCtx_ = formatCtx;

        checkMediaType();
        if (mediaType_ == MEDIA_NONE_TYPE) {
            break;
        }

        if (checkType & MediaDemuxer::CHECK_BASIC_TYPE) {
            checkFormatType();

            checkDuration();

            checkFPS();

            checkRealFPS();

            checkRotate();

            checkFlip();

            checkMetadatas();
        } else {
            assert(0);
        }

#ifndef CLOSE_NATIVE_CODE
        if (checkType & MediaDemuxer::CHECK_COMPLEX_TYPE) {
            workThread_ = std::thread([&](){
                checkComplex();
            });
        }
#endif

        return true;
    } while (false);

    if (formatCtx) {
        closeInFormatContext(formatCtx);
        formatCtx = nullptr;
    }

    formatCtx_ = nullptr;
    filePath_.clear();

    return false;
}

bool MediaDemuxer::open2(const string &filePath, const MediaDemuxer::CheckType &checkType)
{
    filePath_ = filePath;

    AVFormatContext* formatCtx = nullptr;

    do {
        memset(&bufferData, 0, sizeof(BufferData));
        bufferData.file = nullptr;

        formatCtx = openInFormatContext(filePath, &bufferData);
        if (!formatCtx) {
            break;
        }
        formatCtx_ = formatCtx;

        checkMediaType();
        if (mediaType_ == MEDIA_NONE_TYPE) {
            break;
        }

        if (checkType & MediaDemuxer::CHECK_BASIC_TYPE) {
            checkFormatType();

            checkDuration();

            checkFPS();

            checkRealFPS();

            checkRotate();

            checkFlip();

            checkMetadatas();
        } else {
            assert(0);
        }

#ifndef CLOSE_NATIVE_CODE
        if (checkType & MediaDemuxer::CHECK_COMPLEX_TYPE) {
            workThread_ = std::thread([&](){
                checkComplex();
            });
        }
#endif

        return true;
    } while (false);

    if (formatCtx) {
        closeInFormatContext(formatCtx);
        formatCtx = nullptr;
    }

    formatCtx_ = nullptr;
    filePath_.clear();

    return false;
}

bool MediaDemuxer::open(const char *data, const int dataSize, const MediaDemuxer::CheckType &checkType)
{
    AVFormatContext* formatCtx = nullptr;

    do {
        formatCtx = openInFormatContext(data, dataSize);
        if (!formatCtx) {
            break;
        }
        formatCtx_ = formatCtx;

        checkMediaType();
        if (mediaType_ == MEDIA_NONE_TYPE) {
            break;
        }

        if (checkType & MediaDemuxer::CHECK_BASIC_TYPE) {
            checkFormatType();

            checkDuration();

            checkFPS();

            checkRealFPS();

            checkRotate();

            checkMetadatas();

            checkFlip();
        } else {
            assert(0);
        }

#ifndef CLOSE_NATIVE_CODE
        if (checkType & MediaDemuxer::CHECK_COMPLEX_TYPE) {
            workThread_ = std::thread([&](){
                checkComplex();
            });
        }
#endif

        return true;
    } while (false);

    if (formatCtx) {
        closeInFormatContext(formatCtx);
        formatCtx = nullptr;
    }

    formatCtx_ = nullptr;

    return false;
}

bool MediaDemuxer::open(const char *data, const int dataSize, int *error, const MediaDemuxer::CheckType &checkType)
{
    AVFormatContext* formatCtx = nullptr;

    do {
        memset(&bufferData, 0, sizeof(BufferData));
        bufferData.buf = (uint8_t*)data;
        bufferData.size = dataSize;
        bufferData.ptr = bufferData.buf;
        formatCtx = openInFormatContext(data, dataSize, &bufferData, error);
        if (!formatCtx) {
            break;
        }
        formatCtx_ = formatCtx;

        checkMediaType();
        if (mediaType_ == MEDIA_NONE_TYPE) {
            break;
        }

        if (checkType & MediaDemuxer::CHECK_BASIC_TYPE) {
            checkFormatType();

            checkDuration();

            checkFPS();

            checkRealFPS();

            checkRotate();

            checkFlip();

            checkMetadatas();
        } else {
            assert(0);
        }

#ifndef CLOSE_NATIVE_CODE
        if (checkType & MediaDemuxer::CHECK_COMPLEX_TYPE) {
            workThread_ = std::thread([&](){
                checkComplex();
            });
        }
#endif

        return true;
    } while (false);

    if (formatCtx) {
        closeInFormatContext(formatCtx);
        formatCtx = nullptr;
    }

    formatCtx_ = nullptr;

    return false;
}

bool MediaDemuxer::open2(const char *data, const int dataSize, const MediaDemuxer::CheckType &checkType)
{
    AVFormatContext* formatCtx = nullptr;

    do {
        memset(&bufferData2, 0, sizeof(BufferData2));
        bufferData2.ptr = (uint8_t*)data;
        bufferData2.size = dataSize;
        bufferData2.pos = 0;
        formatCtx = openInFormatContext(data, dataSize, &bufferData2);
        if (!formatCtx) {
            break;
        }
        formatCtx_ = formatCtx;

        checkMediaType();
        if (mediaType_ == MEDIA_NONE_TYPE) {
            break;
        }

        if (checkType & MediaDemuxer::CHECK_BASIC_TYPE) {
            checkFormatType();

            checkDuration();

            checkFPS();

            checkRealFPS();

            checkRotate();

            checkMetadatas();

            checkFlip();
        } else {
            assert(0);
        }

#ifndef CLOSE_NATIVE_CODE
        if (checkType & MediaDemuxer::CHECK_COMPLEX_TYPE) {
            workThread_ = std::thread([&](){
                checkComplex();
            });
        }
#endif

        return true;
    } while (false);

    if (formatCtx) {
        closeInFormatContext(formatCtx);
        formatCtx = nullptr;
    }

    formatCtx_ = nullptr;

    return false;
}

bool MediaDemuxer::open(void *opaque, int (*readPacket)(void *, uint8_t *, int), int64_t (*seek)(void *, int64_t, int), const MediaDemuxer::CheckType &checkType)
{
    AVFormatContext* formatCtx = nullptr;

    do {
        formatCtx = openInFormatContext(opaque, readPacket, seek);
        if (!formatCtx) {
            break;
        }
        formatCtx_ = formatCtx;

        checkMediaType();
        if (mediaType_ == MEDIA_NONE_TYPE) {
            break;
        }

        if (checkType & MediaDemuxer::CHECK_BASIC_TYPE) {
            checkFormatType();

            checkDuration();

            checkFPS();

            checkRealFPS();

            checkRotate();

            checkMetadatas();

            checkFlip();
        } else {
            assert(0);
        }

#ifndef CLOSE_NATIVE_CODE
        if (checkType & MediaDemuxer::CHECK_COMPLEX_TYPE) {
            workThread_ = std::thread([&](){
                checkComplex();
            });
        }
#endif

        return true;
    } while (false);

    if (formatCtx) {
        closeInFormatContext(formatCtx);
        formatCtx = nullptr;
    }

    formatCtx_ = nullptr;

    return false;
}

void MediaDemuxer::close()
{
#ifndef CLOSE_NATIVE_CODE
    if (workThread_.get_id() != std::this_thread::get_id() && workThread_.joinable()) {
        workThread_.join();
    }
#endif

    if (bufferData.file) {
        fclose(bufferData.file);
        bufferData.file = nullptr;
    }

    switch (mediaType_) {
    case MEDIA_STILL_PICTURE_TYPE: {
        closePicture();
    }
        break;
    default:
        break;
    }

    if (!formatCtx_) {
        return;
    }
    closeInFormatContext(static_cast<AVFormatContext*>(formatCtx_));
    formatCtx_ = nullptr;
}

void MediaDemuxer::seek(const int64_t time)
{
#ifndef CLOSE_NATIVE_CODE
    if (workThread_.get_id() != std::this_thread::get_id() && workThread_.joinable()) {
        workThread_.join();
    }
#endif

    switch (mediaType_) {
    case MEDIA_STILL_PICTURE_TYPE:
        break;
    case MEDIA_MOTION_PICTURE_TYPE:
    case MEDIA_VIDEO_AUDIO_TYPE: {
        AVFormatContext* formatCtx = static_cast<AVFormatContext*>(formatCtx_);
        if (!formatCtx) {
            return;
        }

        int ret = 0;
        if ((ret = av_seek_frame(formatCtx, -1, time * MILLISECOND_TO_MICROSECOND_UNIT, AVSEEK_FLAG_BACKWARD)) < 0) {
            assert(0);
        }
    }
        break;
    default:
        break;
    }
}

PacketContext *MediaDemuxer::read()
{
#ifndef CLOSE_NATIVE_CODE
    if (workThread_.get_id() != std::this_thread::get_id() && workThread_.joinable()) {
        workThread_.join();
    }
#endif

    PacketContext* packetCtx = nullptr;

    switch (mediaType_) {
    case MEDIA_STILL_PICTURE_TYPE:
    case MEDIA_MOTION_PICTURE_TYPE:
    case MEDIA_VIDEO_AUDIO_TYPE: {
        AVFormatContext* formatCtx = static_cast<AVFormatContext*>(formatCtx_);
        if (!formatCtx) {
            break;
        }
        while (true) {
            AVPacket* packet = av_packet_alloc();
            int ret = av_read_frame(formatCtx, packet);
            if (ret < 0) {
                av_packet_free(&packet);
                packet = nullptr;
                break;
            }
            AVStream* stream = formatCtx->streams[packet->stream_index];
            if (stream->codecpar->codec_id == AV_CODEC_ID_NONE) {
                av_packet_free(&packet);
                packet = nullptr;
                continue;
            }
            if (packet->stream_index != getDefaultVideoIndex(formatCtx) && packet->stream_index != getDefaultAudioIndex(formatCtx)) {
                av_packet_free(&packet);
                packet = nullptr;
                continue;
            }
            AVRational defaultTimeBase;
            defaultTimeBase.num = 1;
            defaultTimeBase.den = AV_TIME_BASE;
            av_packet_rescale_ts(packet, stream->time_base, defaultTimeBase);
            packetCtx = new PacketContext();
            packetCtx->setPacketType(stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO ? PacketContext::PACKET_VIDEO_TYPE : PacketContext::PACKET_AUDIO_TYPE);
            packetCtx->setPacket(packet);
            break;
        }
    }
        break;
    default:
        break;
    }

    return packetCtx;
}

int64_t MediaDemuxer::getDuration() const
{
    int64_t duration = 0;

    switch (mediaType_) {
    case MEDIA_STILL_PICTURE_TYPE: {
        duration = 0;
    }
        break;
    case MEDIA_MOTION_PICTURE_TYPE: {
        duration = getVideoDuration();
    }
        break;
    case MEDIA_VIDEO_AUDIO_TYPE: {
        void* videoCodecPar = getVideoCodecPar();
        void* audioCodecPar = getAudioCodecPar();
        if (videoCodecPar && audioCodecPar) {
            duration = getVideoDuration();
        } else if (!videoCodecPar && audioCodecPar) {
            duration = getAudioDuration();
        } else if (videoCodecPar && !audioCodecPar) {
            duration = getVideoDuration();
        } else {
            duration = 0;
        }
    }
        break;
    default:
        break;
    }

    return duration;
}

const MSize MediaDemuxer::getSize() const
{
    MSize size;

    switch (mediaType_) {
    case MEDIA_STILL_PICTURE_TYPE:
    case MEDIA_MOTION_PICTURE_TYPE:
    case MEDIA_VIDEO_AUDIO_TYPE: {
        AVCodecParameters* videoCodecPar = static_cast<AVCodecParameters*>(getVideoCodecPar());
        if (videoCodecPar) {
            size = MSize(videoCodecPar->width, videoCodecPar->height);
        }
    }
        break;
    default:
        break;
    }

    return size;
}

void *MediaDemuxer::getVideoCodecPar() const
{
    void* videoCodecPar = nullptr;

    switch (mediaType_) {
    case MEDIA_STILL_PICTURE_TYPE:
    case MEDIA_MOTION_PICTURE_TYPE:
    case MEDIA_VIDEO_AUDIO_TYPE: {
        AVFormatContext* formatCtx = static_cast<AVFormatContext*>(formatCtx_);
        if (!formatCtx) {
            break;
        }
        const int defaultVideoIndex = getDefaultVideoIndex(formatCtx);
        if (defaultVideoIndex < 0) {
            break;
        }
        AVStream* videoStream = formatCtx->streams[defaultVideoIndex];
        videoCodecPar = videoStream->codecpar;
    }
        break;
    default:
        break;
    }

    return videoCodecPar;
}

void *MediaDemuxer::getAudioCodecPar() const
{
    void* audioCodecPar = nullptr;

    switch (mediaType_) {
    case MEDIA_STILL_PICTURE_TYPE:
    case MEDIA_MOTION_PICTURE_TYPE: {
    }
        break;
    case MEDIA_VIDEO_AUDIO_TYPE: {
        AVFormatContext* formatCtx = static_cast<AVFormatContext*>(formatCtx_);
        if (!formatCtx) {
            break;
        }
        const int defaultAudioIndex = getDefaultAudioIndex(formatCtx);
        if (defaultAudioIndex < 0) {
            break;
        }
        AVStream* audioStream = formatCtx->streams[defaultAudioIndex];
        audioCodecPar = audioStream->codecpar;
    }
        break;
    default:
        break;
    }

    return audioCodecPar;
}

bool MediaDemuxer::haveVideo() const
{
    switch (mediaType_) {
    case MEDIA_STILL_PICTURE_TYPE:
    case MEDIA_MOTION_PICTURE_TYPE: {
        return true;
    }
        break;
    case MEDIA_VIDEO_AUDIO_TYPE: {
        AVCodecParameters* audioCodecPar = static_cast<AVCodecParameters*>(getAudioCodecPar());
        if (audioCodecPar && audioCodecPar->codec_id == AV_CODEC_ID_MP3) {
            return false;
        } else {
            return getVideoCodecPar() ? true : false;
        }
    }
        break;
    default:
        break;
    }

    return false;
}

bool MediaDemuxer::haveAudio() const
{
    switch (mediaType_) {
    case MEDIA_STILL_PICTURE_TYPE:
    case MEDIA_MOTION_PICTURE_TYPE: {
        return false;
    }
        break;
    case MEDIA_VIDEO_AUDIO_TYPE: {
        return getAudioCodecPar() ? true : false;
    }
        break;
    default:
        break;
    }

    return false;
}

void MediaDemuxer::closePicture()
{
    if (pictureFrame_) {
        delete pictureFrame_;
        pictureFrame_ = nullptr;
    }
}

int MediaDemuxer::getAudioCodecId() const
{
    int audioCodecId = 0;

    switch (mediaType_) {
    case MEDIA_STILL_PICTURE_TYPE:
    case MEDIA_MOTION_PICTURE_TYPE: {
        audioCodecId = AV_CODEC_ID_NONE;
    }
        break;
    case MEDIA_VIDEO_AUDIO_TYPE: {
        AVFormatContext* formatCtx = static_cast<AVFormatContext*>(formatCtx_);
        if (!formatCtx) {
            break;
        }
        const int defaultAudioIndex = getDefaultAudioIndex(formatCtx);
        if (defaultAudioIndex < 0) {
            break;
        }
        AVStream* audioStream = formatCtx->streams[defaultAudioIndex];
        audioCodecId = audioStream->codecpar->codec_id;
    }
        break;
    default:
        break;
    }

    return audioCodecId;
}

void MediaDemuxer::checkFormatType()
{
    FormatType formatType = FORMAT_NONE_TYPE;

    do {
        AVFormatContext* formatCtx = static_cast<AVFormatContext*>(formatCtx_);
        if (!formatCtx) {
            break;
        }

        const std::string formatName(formatCtx->iformat->name);
        if (formatName.find("mp4") != std::string::npos) {
            formatType = FORMAT_MP4_TYPE;
        } else if (formatName.find("mp3") != std::string::npos) {
            formatType = FORMAT_MP3_TYPE;
        } else if (formatName.find("flac") != std::string::npos) {
            formatType = FORMAT_FLAC_TYPE;
        } else if (formatName.find("ogg") != std::string::npos) {
            formatType = FORMAT_OGG_TYPE;
        } else if (formatName.find("wav") != std::string::npos) {
            formatType = FORMAT_WAV_TYPE;
        } else if (formatName.find("m4a") != std::string::npos) {
            formatType = FORMAT_M4A_TYPE;
        } else if (formatName.find("aac") != std::string::npos) {
            formatType = FORMAT_AAC_TYPE;
        } else if (formatName.find("gif") != std::string::npos) {
            formatType = FORMAT_GIF_TYPE;
        } else if (formatName.find("jpeg") != std::string::npos) {
            formatType = FORMAT_JPG_TYPE;
        } else if (formatName.find("image2") != std::string::npos) {
            formatType = FORMAT_JPG_TYPE;
        } else if (formatName.find("png") != std::string::npos) {
            formatType = FORMAT_PNG_TYPE;
        } else if (formatName.find("bmp") != std::string::npos) {
            formatType = FORMAT_BMP_TYPE;
        } else if (formatName.find("webp") != std::string::npos) {
            formatType = FORMAT_WEBP_TYPE;
        } else {
            assert(0);
        }
    } while (false);

    formatType_ = formatType;
}

void MediaDemuxer::checkMediaType()
{
    MediaType _mediaType = MEDIA_NONE_TYPE;

    do {
        AVFormatContext* formatCtx = static_cast<AVFormatContext*>(formatCtx_);
        if (!formatCtx) {
            break;
        }

        bool haveUnsupportedCodec = false;
        for (unsigned int i = 0; i < formatCtx->nb_streams; i++) {
            AVStream* stream = formatCtx->streams[i];
            AVCodecID codecId = stream->codecpar->codec_id;
            AVMediaType mediaType = stream->codecpar->codec_type;
            int64_t framesNumber = stream->nb_frames;
            switch (mediaType) {
            case AVMEDIA_TYPE_VIDEO:
            {
                switch (codecId) {
                case AV_CODEC_ID_PNG:
                case AV_CODEC_ID_BMP:
                case AV_CODEC_ID_WEBP:
                {
                    if (formatCtx->nb_streams != 1) {
                        break;
                    }
                    _mediaType = MEDIA_STILL_PICTURE_TYPE;
                }
                    break;
                case AV_CODEC_ID_MJPEG:
                {
                    if (framesNumber == 0) {
                        if (formatCtx->nb_streams != 1) {
                            break;
                        }
                        _mediaType = MEDIA_STILL_PICTURE_TYPE;
                    } else if (framesNumber > 0) {
                        //iOS support MJPEG codec hardware decode, Android not support.
#ifdef IOS_OS
                        _mediaType = MEDIA_VIDEO_AUDIO_TYPE;
#elif defined(ANDROID_OS)
                        haveUnsupportedCodec = true;
#endif
                    } else {
                        assert(0);
                    }
                }
                    break;
                case AV_CODEC_ID_H264:
                case AV_CODEC_ID_HEVC:
                case AV_CODEC_ID_MPEG4:
                {
                    _mediaType = MEDIA_VIDEO_AUDIO_TYPE;
                }
                    break;
                case AV_CODEC_ID_PRORES:
                {
                    //iOS support PRORES codec hardware decode, Android not support.
#if 0
#ifdef IOS_OS
                    if (VideoDecoder::checkHardwareSupportForCodecPRORES(stream->codecpar->codec_tag)) {
                        _mediaType = MEDIA_VIDEO_AUDIO_TYPE;
                    } else {
                        haveUnsupportedCodec = true;
                    }
#elif defined(ANDROID_OS)
                    haveUnsupportedCodec = true;
#endif
#else
                    assert(0);
#endif
                }
                    break;
                case AV_CODEC_ID_GIF:
                {
                    _mediaType = MEDIA_MOTION_PICTURE_TYPE;
                }
                    break;
                default:
                {
                    haveUnsupportedCodec = true;
                }
                    break;
                }
            }
                break;
            case AVMEDIA_TYPE_AUDIO:
            {
                if (avcodec_find_decoder(codecId)) {
                    _mediaType = MEDIA_VIDEO_AUDIO_TYPE;
                } else {
                    haveUnsupportedCodec = true;
                }
            }
                break;
            default:
                break;
            }

            if (haveUnsupportedCodec) {
                break;
            }
        }

        if (haveUnsupportedCodec) {
            _mediaType = MEDIA_NONE_TYPE;
        }
    } while (false);

    mediaType_ = _mediaType;
}

void MediaDemuxer::checkDuration()
{
    int64_t videoDuration = 0;
    int64_t audioDuration = 0;

    do {
        switch (mediaType_) {
        case MEDIA_STILL_PICTURE_TYPE: {
            videoDuration = 0;
            audioDuration = 0;
        }
            break;
        case MEDIA_MOTION_PICTURE_TYPE: {
            seek(0);
            while (true) {
                PacketContext* packetCtx = read();
                if (!packetCtx) {
                    break;
                }
                if (packetCtx->getPacketType() != PacketContext::PACKET_VIDEO_TYPE) {
                    delete packetCtx;
                    packetCtx = nullptr;
                    continue;
                }
                videoDuration += packetCtx->getDuration();
                delete packetCtx;
                packetCtx = nullptr;
            }
            seek(0);
            audioDuration = 0;
        }
            break;
        case MEDIA_VIDEO_AUDIO_TYPE: {
            AVFormatContext* formatCtx = static_cast<AVFormatContext*>(formatCtx_);
            if (!formatCtx) {
                break;
            }
            videoDuration = MediaLibrary::getVideoDuration(formatCtx);
            audioDuration = MediaLibrary::getAudioDuration(formatCtx);
        }
            break;
        default:
            break;
        }
    } while (false);

    videoDuration_ = videoDuration;
    audioDuration_ = audioDuration;
}

void MediaDemuxer::checkFPS()
{
    float FPS = 0;

    do {
        switch (mediaType_) {
        case MEDIA_STILL_PICTURE_TYPE: {
            FPS = 0;
        }
            break;
        case MEDIA_MOTION_PICTURE_TYPE:
        case MEDIA_VIDEO_AUDIO_TYPE: {
            AVFormatContext* formatCtx = static_cast<AVFormatContext*>(formatCtx_);
            if (!formatCtx) {
                break;
            }
            FPS = MediaLibrary::getFPS(formatCtx);
        }
            break;
        default:
            break;
        }
    } while (false);

    FPS_ = FPS;
}

void MediaDemuxer::checkRealFPS()
{
    float realFPS = 0;

    do {
        switch (mediaType_) {
        case MEDIA_STILL_PICTURE_TYPE: {
            realFPS = 0;
        }
            break;
        case MEDIA_MOTION_PICTURE_TYPE: {
            AVFormatContext* formatCtx = static_cast<AVFormatContext*>(formatCtx_);
            if (!formatCtx) {
                break;
            }
            realFPS = MediaLibrary::getFPS(formatCtx);
        }
            break;
        case MEDIA_VIDEO_AUDIO_TYPE: {
            AVFormatContext* formatCtx = static_cast<AVFormatContext*>(formatCtx_);
            if (!formatCtx) {
                break;
            }
            realFPS = MediaLibrary::getRealFPS(formatCtx);
        }
            break;
        default:
            break;
        }
    } while (false);

    realFPS_ = realFPS;
}

void MediaDemuxer::checkRotate()
{
    int rotate = 0;

    do {
        AVFormatContext* formatCtx = static_cast<AVFormatContext*>(formatCtx_);
        if (!formatCtx) {
            break;
        }
        rotate = MediaLibrary::getRotate(formatCtx);
    } while (false);

    rotate_ = rotate;
}

void MediaDemuxer::checkFlip()
{
}

void MediaDemuxer::checkMetadatas()
{
    std::map<MetadataType, string> metadatas;

    do {
        AVFormatContext* formatCtx = static_cast<AVFormatContext*>(formatCtx_);
        if (!formatCtx) {
            break;
        }

        const AVDictionary* metadata = formatCtx->metadata;
        const int metadataNumber = av_dict_count(metadata);
        if (!metadataNumber) {
            break;
        }

        AVDictionaryEntry *entry = nullptr;
        const std::vector<std::string> metadataKeys = {"title", "artist", "album", "comment", "date", "track", "genre"};
        for (int i = 0; i < (int)metadataKeys.size(); i++) {
            entry = av_dict_get(metadata, "", entry, AV_DICT_IGNORE_SUFFIX);
            if (!entry) {
                continue;
            }
            if (strcmp(entry->key, "title") == 0) {
                metadatas[METADATA_TITLE_TYPE] = entry->value;
            } else if (strcmp(entry->key, "artist") == 0) {
                metadatas[METADATA_ARTIST_TYPE] = entry->value;
            } else if (strcmp(entry->key, "album") == 0) {
                metadatas[METADATA_ALBUM_TYPE] = entry->value;
            } else if (strcmp(entry->key, "comment") == 0) {
                metadatas[METADATA_COMMENT_TYPE] = entry->value;
            } else if (strcmp(entry->key, "date") == 0) {
                metadatas[METADATA_DATE_TYPE] = entry->value;
            } else if (strcmp(entry->key, "track") == 0) {
                metadatas[METADATA_TRACK_TYPE] = entry->value;
            } else if (strcmp(entry->key, "genre") == 0) {
                metadatas[METADATA_GENRE_TYPE] = entry->value;
            }
        }
    } while (false);

    metadatas_ = metadatas;
}

void MediaDemuxer::checkComplex()
{
    if (mediaType_ == MEDIA_STILL_PICTURE_TYPE) {
        return;
    }

    seek(0);

    while (true) {
        PacketContext* packetCtx = read();
        if (!packetCtx) {
            break;
        }

        if (packetCtx->getPacketType() != PacketContext::PACKET_VIDEO_TYPE) {
            delete packetCtx;
            packetCtx = nullptr;
            continue;
        }

        AVPacket* packet = packetCtx->getPacket();
        if (packet->flags & AV_PKT_FLAG_KEY) {
            const int64_t keyframeTime = packetCtx->getTime();
            keyframeTimes_.push_back(keyframeTime);
        }

        delete packetCtx;
        packetCtx = nullptr;
    }

    seek(0);
}

bool MediaDemuxer::openPicture(const MSize &size)
{
    pictureFrame_ = parsePicture(filePath_, size);
    return true;
}

}
