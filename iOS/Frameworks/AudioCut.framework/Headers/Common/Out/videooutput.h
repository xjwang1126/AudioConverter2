#ifndef VIDEOOUTPUT_H
#define VIDEOOUTPUT_H

#include "Format/msize.h"

namespace MediaLibrary {

class RenderContext;
class TextureFrame;
class ImageBlendFilter;

struct VideoOutputContext;

class VideoOutput
{
public:
    VideoOutput();
    virtual ~VideoOutput();

#ifdef IOS_OS
    void initCanvasView(const MSize& canvasSize);
    void* getCanvasView();
#endif

    void begin(const MSize& size);
    void render(TextureFrame* textureFrame);
    void end();
    void display();

    void display(TextureFrame* textureFrame);

    RenderContext* getRenderCtx() { return renderCtx_; }

    ImageBlendFilter* getBlendFilter() { return blendFilter_; }

    bool initRenderContext();
    void deinitRenderContext();

private:
    bool init();
    void deinit();

    void beginBlendCtx();
    void endBlendCtx();
    void* getBlendCtx();

    TextureFrame* getReadTextureFrame() const;
    TextureFrame* getWriteTextureFrame() const;
    void switchReadWriteTextureFrame();

private:
    VideoOutputContext* videoOutputCtx_{nullptr};

    TextureFrame* textureFrame1_{nullptr};
    TextureFrame* textureFrame2_{nullptr};
    int writeTextureIndex_{0};

    RenderContext* renderCtx_{nullptr};
    ImageBlendFilter* blendFilter_{nullptr};
};

}

#endif // VIDEOOUTPUT_H
