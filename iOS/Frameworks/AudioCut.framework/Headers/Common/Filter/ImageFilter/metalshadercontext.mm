#include "metalshadercontext.h"
#include "Render/metalrendercontext.h"
#include "Model/textureframe.h"
#include "Model/imageframe.h"
#include "Filter/ImageFilter/shadertypes.h"
#import <MetalKit/MetalKit.h>

namespace MediaLibrary {

MetalShaderContext::MetalShaderContext()
    : ShaderContext()
    , renderPassDescriptor(nullptr)
    , renderPipelineState(nullptr)
    , vertex(nullptr)
    , uniform(nullptr)
    , commandBufferLabel(nullptr)
{
}

MetalShaderContext::~MetalShaderContext()
{
}

void MetalShaderContext::setCommandBufferLabel(const std::string &value)
{
    commandBufferLabel = [NSString stringWithUTF8String:value.c_str()];
}

TextureFrame *MetalShaderContext::render(const TextureFrame *textureFrame, const MSize &surfaceSize, void *context)
{
    @autoreleasepool {
        MetalRenderContext* metalRenderCtx = MetalRenderContext::getInstance();
        id<MTLCommandQueue> commandQueue = metalRenderCtx->getCommandQueue();

        void* texture = createTexture(surfaceSize,
                                      getPixelFormat(),
                                      static_cast<TextureUsage>(TEXTURE_USAGE_RENDER_TARGET | TEXTURE_USAGE_READ));
        id<MTLTexture> renderTexture = static_cast<id<MTLTexture> >(texture);

        MTLRenderPassDescriptor* _renderPassDescriptor = static_cast<MTLRenderPassDescriptor*>(renderPassDescriptor);
        _renderPassDescriptor.colorAttachments[0].texture = renderTexture;

        id<MTLCommandBuffer> commandBuffer = nullptr;
        if (!context) {
            commandBuffer = [commandQueue commandBuffer];
        } else {
            commandBuffer = static_cast<id<MTLCommandBuffer> >(context);
        }
        if (!commandBuffer) {
            return nullptr;
        }
        commandBuffer.label = static_cast<NSString*>(commandBufferLabel);

        id<MTLRenderCommandEncoder> renderEncoder = [commandBuffer renderCommandEncoderWithDescriptor:_renderPassDescriptor];
        if (!renderEncoder) {
            return nullptr;
        }

        MTLViewport viewport;
        viewport.originX = 0.0;
        viewport.originY = 0.0;
        viewport.width = static_cast<double>(surfaceSize.getWidth());
        viewport.height = static_cast<double>(surfaceSize.getHeight());
        viewport.znear = -1.0;
        viewport.zfar = 1.0;
        [renderEncoder setViewport:viewport];

        id<MTLRenderPipelineState> _renderPipelineState = static_cast<id<MTLRenderPipelineState> >(renderPipelineState);
        [renderEncoder setRenderPipelineState:_renderPipelineState];

        id<MTLTexture> sourceTexture = reinterpret_cast<id<MTLTexture> >(textureFrame->getFrame());
        [renderEncoder setFragmentTexture:sourceTexture atIndex:TextureIndexBaseColor];

        updateUniform(renderEncoder);

        updateVertex(renderEncoder);

        updateTexture(renderEncoder);

        drawPrimitive(renderEncoder);

        [renderEncoder endEncoding];

        if (!context) {
            [commandBuffer commit];

            [commandBuffer waitUntilScheduled];
        }

        TextureFrame* renderTextureFrame = new TextureFrame;
        renderTextureFrame->setTimeBase(TimeBase(1, SECOND_TO_MILLISECOND_UNIT));
        renderTextureFrame->setFrame(renderTexture);
        renderTextureFrame->setSize(surfaceSize);

        return renderTextureFrame;
    }
}

TextureFrame *MetalShaderContext::render(const ImageFrame *imageFrame)
{
    @autoreleasepool {
        TextureFrame* textureFrame = nullptr;

        const MSize size = imageFrame->getSize();
        void* texture = createTexture(size,
                                      PIXEL_FORMAT_RGBA,
                                      TEXTURE_USAGE_NONE);
        id<MTLTexture> mtlTexture = static_cast<id<MTLTexture> >(texture);
        int bytesPerRow = size.getWidth() * 4;
        MTLRegion region = {
            { 0, 0, 0 },
            { static_cast<NSUInteger>(size.getWidth()), static_cast<NSUInteger>(size.getHeight()), 1}
        };
        [mtlTexture replaceRegion:region mipmapLevel:0 withBytes:imageFrame->getFrame() bytesPerRow:static_cast<NSUInteger>(bytesPerRow)];

        textureFrame = new TextureFrame();
        textureFrame->setTimeBase(TimeBase(1, SECOND_TO_MILLISECOND_UNIT));
        textureFrame->setTime(imageFrame->getTime());
        textureFrame->setDuration(imageFrame->getDuration());
        textureFrame->setFrame(mtlTexture);
        textureFrame->setSize(size);

        return textureFrame;
    }
}

ImageFrame *MetalShaderContext::read(const TextureFrame *textureFrame, const PixelFormat &pixelFormat)
{
    (void)pixelFormat;
    @autoreleasepool {
        ImageFrame* imageFrame = nullptr;

        id<MTLTexture> texture = static_cast<id<MTLTexture> >(textureFrame->getFrame());

        const MSize size = textureFrame->getSize();
        uint8_t* data = new uint8_t[static_cast<int>(size.getWidth()) * 4 * static_cast<int>(size.getHeight())];

        NSUInteger bytesPerRow = static_cast<NSUInteger>(size.getWidth() * 4);

        MTLRegion region = MTLRegionMake2D(0, 0, static_cast<NSUInteger>(size.getWidth()), static_cast<NSUInteger>(size.getHeight()));

        [texture getBytes:data bytesPerRow:bytesPerRow fromRegion:region mipmapLevel:0];

        imageFrame = new ImageFrame();
        imageFrame->setTimeBase(TimeBase(1, SECOND_TO_MILLISECOND_UNIT));
        imageFrame->setTime(textureFrame->getTime());
        imageFrame->setDuration(textureFrame->getDuration());
        imageFrame->setFrame(data);
        imageFrame->setSize(size);

        return imageFrame;
    }
}

void *MetalShaderContext::createTexture(const MSize &size, const PixelFormat &pixelFormat, const TextureUsage &textureUsage)
{
    @autoreleasepool {
        MetalRenderContext* metalRenderCtx = MetalRenderContext::getInstance();
        id<MTLDevice> device = metalRenderCtx->getDevice();

        MTLPixelFormat nativePixelFormat = MTLPixelFormatInvalid;
        switch (pixelFormat) {
        case PIXEL_FORMAT_RGBA: {
            nativePixelFormat = MTLPixelFormatRGBA8Unorm;
        }
            break;
        case PIXEL_FORMAT_BGRA: {
            nativePixelFormat = MTLPixelFormatBGRA8Unorm;
        }
            break;
        case PIXEL_FORMAT_ALPHA: {
            nativePixelFormat = MTLPixelFormatA8Unorm;
        }
            break;
        default:
            break;
        }

        MTLTextureUsage usage = MTLTextureUsageUnknown;
        if (textureUsage & TEXTURE_USAGE_READ) {
            usage |= MTLTextureUsageShaderRead;
        }
        if (textureUsage & TEXTURE_USAGE_WRITE) {
            usage |= MTLTextureUsageShaderWrite;
        }
        if (textureUsage & TEXTURE_USAGE_RENDER_TARGET) {
            usage |= MTLTextureUsageRenderTarget;
        }

        MTLTextureDescriptor* textureDescriptor = [MTLTextureDescriptor new];
        textureDescriptor.textureType = MTLTextureType2D;
        textureDescriptor.width = static_cast<size_t>(size.getWidth());
        textureDescriptor.height = static_cast<size_t>(size.getHeight());
        textureDescriptor.pixelFormat = nativePixelFormat;
        textureDescriptor.usage = usage;
        id<MTLTexture> renderTexture = [device newTextureWithDescriptor:textureDescriptor];
        [textureDescriptor release];
        textureDescriptor = nullptr;

        return renderTexture;
    }
}

void MetalShaderContext::destroyTexture(void *texture)
{
    @autoreleasepool {
        id<MTLTexture> mtlTexture = static_cast<id<MTLTexture> >(texture);
        if (!mtlTexture) {
            return;
        }
        [mtlTexture release];
        mtlTexture = nullptr;
    }
}

bool MetalShaderContext::createShader()
{
    @autoreleasepool {
        MetalRenderContext* metalRenderCtx = MetalRenderContext::getInstance();
        id<MTLDevice> device = metalRenderCtx->getDevice();
        id<MTLLibrary> library = metalRenderCtx->getLibrary();

        MTLRenderPassDescriptor* _renderPassDescriptor = nullptr;
        id<MTLRenderPipelineState> _renderPipelineState = nullptr;

        do {
            _renderPassDescriptor = [MTLRenderPassDescriptor new];
            if (!_renderPassDescriptor) {
                break;
            }
            _renderPassDescriptor.colorAttachments[0].texture = nullptr;
            _renderPassDescriptor.colorAttachments[0].loadAction = MTLLoadActionClear;
            _renderPassDescriptor.colorAttachments[0].clearColor = MTLClearColorMake(0, 0, 0, 0);
            _renderPassDescriptor.colorAttachments[0].storeAction = MTLStoreActionStore;

            NSString* vertexFunctionName = [NSString stringWithUTF8String:vertexContent_.c_str()];
            id<MTLFunction> vertexFunction = [library newFunctionWithName:vertexFunctionName];
            if (!vertexFunction) {
                break;
            }
            NSString* fragmentFunctionName = [NSString stringWithUTF8String:fragmentContent_.c_str()];
            id<MTLFunction> fragmentFunction = [library newFunctionWithName:fragmentFunctionName];
            if (!fragmentFunction) {
                break;
            }
            MTLRenderPipelineDescriptor* renderPipelineDescriptor = [[MTLRenderPipelineDescriptor alloc] init];
            renderPipelineDescriptor.vertexFunction = vertexFunction;
            renderPipelineDescriptor.fragmentFunction = fragmentFunction;
            MTLPixelFormat pixelFormat = MTLPixelFormatInvalid;
            switch (getPixelFormat()) {
            case PIXEL_FORMAT_BGRA: {
                pixelFormat = MTLPixelFormatBGRA8Unorm;
            }
                break;
            case PIXEL_FORMAT_RGBA: {
                pixelFormat = MTLPixelFormatRGBA8Unorm;
            }
                break;
            default:
                assert(0);
                break;
            }
            renderPipelineDescriptor.colorAttachments[0].pixelFormat = pixelFormat;
            _renderPipelineState = [device newRenderPipelineStateWithDescriptor:renderPipelineDescriptor error:nil];
            if (!_renderPipelineState) {
                break;
            }
            [renderPipelineDescriptor release];
            renderPipelineDescriptor = nullptr;

            createVertex();

            createUnifrom();
        } while (false);

        if (!_renderPassDescriptor || !_renderPipelineState) {
            assert(0);
            return false;
        }

        renderPassDescriptor = _renderPassDescriptor;
        renderPipelineState = _renderPipelineState;

        return true;
    }
}

const float *MetalShaderContext::textureCoordinatesForRotation(const ImageFilter::ImageRotationMode &rotationMode)
{
    static const float noRotationTextureCoordinates[] = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        0.0f, 1.0f,
        1.0f, 1.0f,
    };

    static const float rotateLeftTextureCoordinates[] = {
        1.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 0.0f,
        0.0f, 1.0f,
    };

    static const float rotateRightTextureCoordinates[] = {
        0.0f, 1.0f,
        0.0f, 0.0f,
        1.0f, 1.0f,
        1.0f, 0.0f,
    };

    static const float verticalFlipTextureCoordinates[] = {
        0.0f, 1.0f,
        1.0f, 1.0f,
        0.0f,  0.0f,
        1.0f,  0.0f,
    };

    static const float horizontalFlipTextureCoordinates[] = {
        1.0f, 0.0f,
        0.0f, 0.0f,
        1.0f,  1.0f,
        0.0f,  1.0f,
    };

    static const float rotateRightVerticalFlipTextureCoordinates[] = {
        0.0f, 0.0f,
        0.0f, 1.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
    };

    static const float rotateRightHorizontalFlipTextureCoordinates[] = {
        1.0f, 1.0f,
        1.0f, 0.0f,
        0.0f, 1.0f,
        0.0f, 0.0f,
    };

    static const float rotate180TextureCoordinates[] = {
        1.0f, 1.0f,
        0.0f, 1.0f,
        1.0f, 0.0f,
        0.0f, 0.0f,
    };

    switch(rotationMode)
    {
        case ImageFilter::IMAGE_NoRotation: return noRotationTextureCoordinates;
        case ImageFilter::IMAGE_ROTATE_LEFT: return rotateLeftTextureCoordinates;
        case ImageFilter::IMAGE_ROTATE_RIGHT: return rotateRightTextureCoordinates;
        case ImageFilter::IMAGE_FLIP_VERTICAL: return verticalFlipTextureCoordinates;
        case ImageFilter::IMAGE_FLIP_HORIZONAL: return horizontalFlipTextureCoordinates;
        case ImageFilter::IMAGE_ROTATE_RIGHTFLIPVERTICAL: return rotateRightVerticalFlipTextureCoordinates;
        case ImageFilter::IMAGE_ROTATE_RIGHTFLIPHORIZONTAL: return rotateRightHorizontalFlipTextureCoordinates;
        case ImageFilter::IMAGE_ROTATE180: return rotate180TextureCoordinates;
    }
    return nullptr;
}

void MetalShaderContext::createUnifrom()
{
    @autoreleasepool {
        MetalRenderContext* metalRenderCtx = MetalRenderContext::getInstance();
        id<MTLDevice> device = metalRenderCtx->getDevice();
        id<MTLBuffer> _uniform = [device newBufferWithLength:sizeof(Uniform) options:MTLResourceStorageModeShared];
        uniform = _uniform;
        matrix_float4x4 mvp = matrix_identity_float4x4;
        Uniform* uniformContent = reinterpret_cast<Uniform*>(_uniform.contents);
        uniformContent->mvp = mvp;
    }
}

void MetalShaderContext::destroyUniform()
{
    @autoreleasepool {
        if (uniform) {
            id<MTLBuffer> _uniform = static_cast<id<MTLBuffer> >(uniform);
            [_uniform release];
            uniform = nullptr;
        }
    }
}

void MetalShaderContext::updateUniform(void *context)
{
    @autoreleasepool {
        id<MTLRenderCommandEncoder> renderEncoder = static_cast<id<MTLRenderCommandEncoder> >(context);
        id<MTLBuffer> _uniform = static_cast<id<MTLBuffer> >(uniform);
        [renderEncoder setVertexBuffer:_uniform offset:0 atIndex:VertexIndexUniform];
        [renderEncoder setFragmentBuffer:_uniform offset:0 atIndex:TextureIndexUniform];
    }
}

void MetalShaderContext::createVertex()
{
    @autoreleasepool {
        MetalRenderContext* metalRenderCtx = MetalRenderContext::getInstance();
        id<MTLDevice> device = metalRenderCtx->getDevice();

        const TextureVertex textureVertex[] = {
            { { -1.f,  1.f }, { 0.f, 0.f } },
            { {  1.f,  1.f }, { 1.f, 0.f } },
            { { -1.f, -1.f }, { 0.f, 1.f } },
            { {  1.f, -1.f }, { 1.f, 1.f } },
        };
        vertex = [device newBufferWithBytes:textureVertex length:sizeof(textureVertex) options:MTLResourceStorageModeShared];
    }
}

void MetalShaderContext::destroyVertex()
{
    @autoreleasepool {
        if (vertex) {
            id<MTLBuffer> _vertex = static_cast<id<MTLBuffer> >(vertex);
            [_vertex release];
            vertex = nullptr;
        }
    }
}

void MetalShaderContext::updateVertex(void *context)
{
    @autoreleasepool {
        id<MTLRenderCommandEncoder> renderEncoder = static_cast<id<MTLRenderCommandEncoder> >(context);
        id<MTLBuffer> _vertex = static_cast<id<MTLBuffer> >(vertex);
        [renderEncoder setVertexBuffer:_vertex offset:0 atIndex:VertexIndexVertex];
    }
}

void MetalShaderContext::updateTexture(void *context)
{
    (void)context;
}

void MetalShaderContext::drawPrimitive(void *context)
{
    @autoreleasepool {
        id<MTLRenderCommandEncoder> renderEncoder = static_cast<id<MTLRenderCommandEncoder> >(context);
        [renderEncoder drawPrimitives:MTLPrimitiveTypeTriangleStrip vertexStart:0 vertexCount:4];
    }
}

}
