#ifndef METALRENDERCONTEXT_H
#define METALRENDERCONTEXT_H

#import <MetalKit/MetalKit.h>

namespace MediaLibrary {

class MetalRenderContext
{
public:
    static MetalRenderContext* getInstance();

    ~MetalRenderContext();

    id<MTLDevice> getDevice() const;
    id<MTLCommandQueue> getCommandQueue() const;
    id<MTLLibrary> getLibrary() const;

private:
    MetalRenderContext();

private:
    id<MTLDevice> device = nullptr;
    id<MTLCommandQueue> commandQueue = nullptr;
    id<MTLLibrary> library = nullptr;

    static MetalRenderContext* metalRenderCtx;
};

}

#endif // METALRENDERCONTEXT_H
