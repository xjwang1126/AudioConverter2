#include "utilcommon.h"
#include "file.h"
#include "Model/packetcontext.h"
#include "Model/framecontext.h"
#include "Model/ffmpegframe.h"
#include "Model/imageframe.h"
#include "Media/mediademuxer.h"
#include "Codec/Decoder/videodecoder.h"

#include <queue>

namespace MediaLibrary {

ImageFrame *parsePicture(const string &filePath, const MSize &size)
{
    class VideoDecoderDelegateImpl : public DecoderDelegate {
    public:
        virtual ~VideoDecoderDelegateImpl() {
        }

        virtual void output(FrameContext* frameCtx) {
            std::lock_guard<std::mutex> guard(videoDecoderBufferMutex_);
            (void)guard;
            videoDecoderBuffer_.push(frameCtx);
        }

        FrameContext* get() {
            std::lock_guard<std::mutex> guard(videoDecoderBufferMutex_);
            (void)guard;
            FrameContext* frameCtx = videoDecoderBuffer_.front();
            videoDecoderBuffer_.pop();
            return frameCtx;
        }

    private:
        std::queue<FrameContext*> videoDecoderBuffer_;
        std::mutex videoDecoderBufferMutex_;
    };

    ImageFrame* imageFrame = nullptr;

    MediaDemuxer mediaDemuxer;
    VideoDecoder* videoDecoder{nullptr};
    VideoDecoderDelegateImpl* videoDecoderDelegateImpl{nullptr};

    do {
        if (!File::exist(filePath)) {
            break;
        }

        if (!mediaDemuxer.open(filePath)) {
            break;
        }
        if (mediaDemuxer.getMediaType() != MEDIA_STILL_PICTURE_TYPE) {
            break;
        }

        videoDecoderDelegateImpl = new VideoDecoderDelegateImpl();
        videoDecoder = new VideoDecoder(videoDecoderDelegateImpl);
        if (!videoDecoder->open(CODEC_SOFTWARE_TYPE, &mediaDemuxer)) {
            break;
        }

        mediaDemuxer.seek(0);

        PacketContext* packetCtx = nullptr;
        while (true) {
            if (!packetCtx) {
                packetCtx = mediaDemuxer.read();
            }
            if (!packetCtx) {
                break;
            }

            if (packetCtx->getPacketType() != PacketContext::PACKET_VIDEO_TYPE) {
                delete packetCtx;
                packetCtx = nullptr;
                continue;
            }

            if (videoDecoder->decode(packetCtx)) {
                delete packetCtx;
                packetCtx = nullptr;
            }

            FrameContext* frameCtx = videoDecoderDelegateImpl->get();
            if (!frameCtx) {
                continue;
            }

            Frame* frame = frameCtx->getFrame();
            Frame* resizeFrame = frame->resize(size);
            Frame* convertFrame = resizeFrame->convert(Frame::FRAME_IMAGE_TYPE);
            imageFrame = dynamic_cast<ImageFrame*>(convertFrame);
        }
    } while (false);

    if (videoDecoder) {
        videoDecoder->close();

        delete videoDecoder;
        videoDecoder = nullptr;
    }

    if (videoDecoderDelegateImpl) {
        delete videoDecoderDelegateImpl;
        videoDecoderDelegateImpl = nullptr;
    }

    mediaDemuxer.close();

    return imageFrame;
}

}
