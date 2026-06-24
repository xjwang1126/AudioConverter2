#ifndef MEDIAMUXER_H
#define MEDIAMUXER_H

#include "mediainfo.h"
#include "Util/file.h"
#include "Format/metadata.h"

#include <map>
#include <string>

using namespace std;

namespace MediaLibrary {

class ImageFrame;
class PacketContext;
class MediaDemuxer;

class MediaMuxer
{
public:
    enum MuxerType {
        MUXER_FFMPEG_TYPE,

        MUXER_NATIVE_TYPE,
    };

public:
    MediaMuxer();

    void setMuxerType(const MuxerType& muxerType) { muxerType_ = muxerType; }
    const MuxerType& getMuxerType() const { return muxerType_; }

    bool open(const string& filePath);
    bool open2(const string& filePath);
    bool open(const FormatType& formatType);
    bool open(const FormatType& formatType, void* opaque, int (*writePacket)(void *, uint8_t *, int), int64_t (*seek)(void *, int64_t, int));
    void addStream(MediaDemuxer* mediaDemuxer, const int streamIndex);
    bool start(const int loop = 0);
    bool finish();
    void close();
    bool write(PacketContext* packetCtx);

    int getAudioCodecId() const;

public:
    void setMetadatas(const std::map<MetadataType, string>& metadatas);

    bool savePicture(ImageFrame* imageFrame);

    const string& getFilePath() const { return filePath_; }
    const FormatType& getFormatType() const { return formatType_; }
    const MediaType& getMediaType() const { return mediaType_; }

    void* getFormatCtx() const { return formatCtx_; }
    const BufferData& getBufferData() const { return bufferData; }

    bool haveVideo() const;
    bool haveAudio() const;

private:
    string filePath_;
    FormatType formatType_{FORMAT_NONE_TYPE};
    MediaType mediaType_{MEDIA_NONE_TYPE};
    MuxerType muxerType_{MUXER_FFMPEG_TYPE};

    void* formatCtx_{nullptr};

    struct BufferData bufferData;

    bool writeTrailer_{false};
};

}

#endif // MEDIAMUXER_H
