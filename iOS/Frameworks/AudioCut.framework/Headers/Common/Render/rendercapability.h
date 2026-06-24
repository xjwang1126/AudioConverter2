#ifndef RENDERCAPABILITY_H
#define RENDERCAPABILITY_H

namespace MediaLibrary {

class RenderCapability
{
public:
    static RenderCapability* getInstance();
    static void checkRenderCapability();

    bool isSupportGL_EXT_shader_framebuffer_fetch() const;
    bool isSupportGL_ARM_shader_framebuffer_fetch() const;

private:
    RenderCapability();

    void checkExtensionCapacility();

private:
    static RenderCapability* instance;

    bool supportGL_EXT_shader_framebuffer_fetch = false;
    bool supportGL_ARM_shader_framebuffer_fetch = false;
};

}

#endif // RENDERCAPABILITY_H
