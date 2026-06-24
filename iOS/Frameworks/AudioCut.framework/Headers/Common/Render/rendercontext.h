#ifndef RENDERCONTEXT_H
#define RENDERCONTEXT_H

namespace MediaLibrary {

struct RenderContextInternal;

class RenderContext
{
public:
    RenderContext();
    ~RenderContext();

    bool checkRenderContext();
    bool initRenderContext(RenderContext* renderCtx = nullptr);
    bool makeRenderContext();
    bool unmakeRenderContext();
    void deinitRenderContext();

    void* getShareContext();

#ifdef IOS_OS
    void* getTextureCache();
#elif defined (ANDROID_OS)
    void* getDisplay();
    void* getSurface();
    void* getConfig();
    void setSurface(void* surface);
#endif

private:
    bool init();
    void deinit();

    bool checkRenderContextOpenGL();
    bool makeRenderContextOpenGL();
    bool unmakeRenderContextOpenGL();
#ifdef IOS_OS
    bool checkRenderContextMetal();
    bool makeRenderContextMetal();
    bool unmakeRenderContextMetal();
#endif

private:
    RenderContextInternal* renderCtx_{nullptr};
    RenderContext* shareRenderCtx_{nullptr};
};

}

#endif // RENDERCONTEXT_H
