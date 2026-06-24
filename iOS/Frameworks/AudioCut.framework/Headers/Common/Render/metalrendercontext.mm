#include "metalrendercontext.h"

namespace MediaLibrary {

MetalRenderContext* MetalRenderContext::metalRenderCtx = new MetalRenderContext;

MetalRenderContext *MetalRenderContext::getInstance()
{
    return metalRenderCtx;
}

MetalRenderContext::MetalRenderContext()
{
    device = MTLCreateSystemDefaultDevice();

    commandQueue = [device newCommandQueue];

    do {
        library = [device newDefaultLibrary];
        if (library) {
            break;
        }

        NSBundle* bundle = [NSBundle bundleWithIdentifier:@"www.goobird.dev.HiVideo"];
        library = [device newDefaultLibraryWithBundle:bundle error:nil];
    } while(false);
}

MetalRenderContext::~MetalRenderContext()
{
}

id<MTLDevice> MetalRenderContext::getDevice() const
{
    return device;
}

id<MTLCommandQueue> MetalRenderContext::getCommandQueue() const
{
    return commandQueue;
}

id<MTLLibrary> MetalRenderContext::getLibrary() const
{
    return library;
}

}
