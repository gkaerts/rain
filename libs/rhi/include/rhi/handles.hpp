
#pragma once    

#include "common/handle.hpp"
    
namespace rn
{
    // Pipeline API handles
    RN_DEFINE_HANDLE(RasterPipeline, 0x01)
    RN_DEFINE_HANDLE(ComputePipeline, 0x02)
    RN_DEFINE_HANDLE(RTPipeline, 0x03)

    // Resource type handles
    RN_DEFINE_HANDLE(GPUAllocation, 0x04)
    RN_DEFINE_HANDLE(Texture2D, 0x05)
    RN_DEFINE_HANDLE(Texture3D, 0x06)
    RN_DEFINE_HANDLE(Buffer, 0x07)

    // Resource view type handles
    // These are slim handles since they can be passed directly to shaders as bindless indices (except for render target and depth target handles)
    RN_DEFINE_SLIM_HANDLE(Texture2DView, 0x08)
    RN_DEFINE_SLIM_HANDLE(Texture3DView, 0x09)
    RN_DEFINE_SLIM_HANDLE(TextureCubeView, 0x0A)
    RN_DEFINE_SLIM_HANDLE(BufferView, 0x0B)
    RN_DEFINE_SLIM_HANDLE(TypedBufferView, 0x0C)
    RN_DEFINE_SLIM_HANDLE(UniformBufferView, 0x0D)
    RN_DEFINE_SLIM_HANDLE(Texture2DArrayView, 0x0E)
    RN_DEFINE_SLIM_HANDLE(TLASView, 0x0F)
    RN_DEFINE_HANDLE(BLASView, 0x10)

    RN_DEFINE_SLIM_HANDLE(RWTexture2DView, 0x11)
    RN_DEFINE_SLIM_HANDLE(RWTexture3DView, 0x12)
    RN_DEFINE_SLIM_HANDLE(RWBufferView, 0x13)
    RN_DEFINE_SLIM_HANDLE(RWTexture2DArrayView, 0x14)

    RN_DEFINE_HANDLE(RenderTargetView, 0x15)
    RN_DEFINE_HANDLE(DepthStencilView, 0x16)
}