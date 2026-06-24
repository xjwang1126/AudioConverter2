#ifndef MEDIADEMUXER_H
#define MEDIADEMUXER_H

#include "Format/msize.h"
#include "Format/metadata.h"
#include "mediainfo.h"

#include <map>
#include <thread>
#include <string>
#include <cstdint>
#include <vector>

using namespace std;

namespace MediaLibrary {

class ImageFrame;
class PacketContext;

class MediaDemuxer
{
public:
    enum CheckType {
        CHECK_BASIC_TYPE = 0x0001,

        CHECK_COMPLEX_TYPE = 0x0002,
    };

public:
    MediaDemuxer() = default;

    bool open(const string& filePath, const CheckType& checkType = static_cast<CheckType>(CHECK_BASIC_TYPE | CHECK_COMPLEX_TYPE));
    bool open2(const string& filePath, const CheckType& checkType = static_cast<CheckType>(CHECK_BASIC_TYPE | CHECK_COMPLEX_TYPE));
    bool open(const char* data, const int dataSize, const CheckType& checkType = static_cast<CheckType>(CHECK_BASIC_TYPE | CHECK_COMPLEX_TYPE));
    bool open(const char* data, const int dataSize, int* error, const CheckType& checkType = static_cast<CheckType>(CHECK_BASIC_TYPE | CHECK_COMPLEX_TYPE));
    bool open2(const char* data, const int dataSize, const CheckType& checkType = static_cast<CheckType>(CHECK_BASIC_TYPE | CHECK_COMPLEX_TYPE));
    bool open(void* opaque, int (*readPacket)(void*, uint8_t*, int), int64_t (*seek)(void*, int64_t, int), const CheckType& checkType = static_cast<CheckType>(CHECK_BASIC_TYPE | CHECK_COMPLEX_TYPE));
    void close();
    void seek(const int64_t time);
    PacketContext* read();

public:
    const string& getFilePath() const { return filePath_; }
    const FormatType& getFormatType() const { return formatType_; }
    const MediaType& getMediaType() const { return mediaType_; }
    int64_t getDuration() const;
    int64_t getVideoDuration() const { return videoDuration_; }
    int64_t getAudioDuration() const { return audioDuration_; }
    float getFPS() const { return FPS_; }
    float getRealFPS() const { return realFPS_; }
    int getRotate() const { return rotate_; }
    int getFlip() const { return flip_; }
    int getRotation() const;
    const MSize getSize() const;

    void* getFormatContext() const { return formatCtx_; }
    void* getVideoCodecPar() const;
    void* getAudioCodecPar() const;

    bool haveVideo() const;
    bool haveAudio() const;

    bool getVideoHardwareDecode() const { return videoHardwareDecode_; }
    bool getAudioHardwareDecode() const { return audioHardwareDecode_; }
    void setVideoHardwareDecode(const bool videoHardwareDecode) { videoHardwareDecode_ = videoHardwareDecode; }
    void setAudioHardwareDecode(const bool audioHardwareDecode) { audioHardwareDecode_ = audioHardwareDecode; }

    const std::vector<int64_t>& getkeyframeTimes() { return keyframeTimes_; }

    bool openPicture(const MSize& size);
    void closePicture();
    ImageFrame* getPicture() const { return pictureFrame_; }

    const std::map<MetadataType, string>& getMetadatas() const { return metadatas_; }

    int getAudioCodecId() const;

    int getError() const { return error_; }

private:
    void checkFormatType();
    void checkMediaType();
    void checkDuration();
    void checkFPS();
    void checkRealFPS();
    void checkRotate();
    void checkFlip();
    void checkMetadatas();
    void checkComplex();

private:
    string filePath_;
    void* formatCtx_{nullptr};
    FormatType formatType_{FORMAT_NONE_TYPE};
    MediaType mediaType_{MEDIA_NONE_TYPE};
    int64_t videoDuration_{0};
    int64_t audioDuration_{0};
    float FPS_{0.f};
    float realFPS_{0.f};
    int rotate_{0};
    int flip_{0};
    bool videoHardwareDecode_{false};
    bool audioHardwareDecode_{false};
    ImageFrame* pictureFrame_{nullptr};
    std::map<MetadataType, string> metadatas_;

#ifndef CLOSE_NATIVE_CODE
    std::thread workThread_;
    std::mutex workMutex_;
#endif
    std::vector<int64_t> keyframeTimes_;

    struct BufferData bufferData;

    struct BufferData2 bufferData2;

    mutable int error_{0};
};

}

#endif // MEDIADEMUXER_H
