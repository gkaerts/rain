#pragma once

#include "common/common.hpp"

#include "rhi/handles.hpp"
#include "rhi/pipeline.hpp"
#include "rhi/resource.hpp"

namespace rn
{
    enum class MemoryCategoryID : uint8_t;
}

namespace rn::rhi
{
    class CommandList;
    class TemporaryResourceAllocator;
    class SwapChain;

    struct DeviceMemorySettings
    {
        uint32_t rasterPipelineCapacity;
        uint32_t computePipelineCapacity;
        uint32_t raytracingPipelineCapacity;
        uint32_t gpuAllocationCapacity;
        uint32_t bufferCapacity;
        uint32_t texture2dCapacity;
        uint32_t texture3dCapacity;
        uint32_t renderPassCapacity;
        uint32_t blasCapacity;
        uint32_t tlasCapacity;
    };

    DeviceMemorySettings DefaultDeviceMemorySettings();

    struct DeviceCaps
    {
        bool hasHardwareRaytracing : 1;
        bool hasVariableRateShading : 1;
        bool hasMeshShaders : 1;
        bool isUMA : 1;
        bool hasMemorylessAllocations : 1;
    };
    enum class PresentMode : uint32_t
    {
        Immediate = 0,
        VSync
    };

    class Device
    {
    public:

        virtual ~Device() {}

        virtual DeviceCaps              Capabilities() const = 0;
        virtual void                    EndFrame() = 0;

        virtual SwapChain*              CreateSwapChain(
                                            void* windowHandle, 
                                            RenderTargetFormat format,
                                            uint32_t width, 
                                            uint32_t height,
                                            uint32_t bufferCount,
                                            PresentMode presentMode) { RN_NOT_IMPLEMENTED(); return nullptr; };
        virtual void                    Destroy(SwapChain* swapChain) { RN_NOT_IMPLEMENTED(); }

        // Pipeline API
        virtual RasterPipeline          CreateRasterPipeline(const VertexRasterPipelineDesc& desc) { RN_NOT_IMPLEMENTED(); return RasterPipeline::Invalid; }
        virtual RasterPipeline          CreateRasterPipeline(const MeshRasterPipelineDesc& desc) { RN_NOT_IMPLEMENTED(); return RasterPipeline::Invalid; }
        virtual ComputePipeline         CreateComputePipeline(const ComputePipelineDesc& desc) { RN_NOT_IMPLEMENTED(); return ComputePipeline::Invalid; }
        virtual RTPipeline              CreateRTPipeline(const RTPipelineDesc& desc) { RN_NOT_IMPLEMENTED(); return RTPipeline::Invalid; }

        virtual void                    Destroy(RasterPipeline pipeline) { RN_NOT_IMPLEMENTED(); }
        virtual void                    Destroy(ComputePipeline pipeline) { RN_NOT_IMPLEMENTED(); }
        virtual void                    Destroy(RTPipeline pipeline) { RN_NOT_IMPLEMENTED(); }


        // Resource API
        virtual GPUAllocation           GPUAlloc(MemoryCategoryID cat, uint64_t sizeInBytes, GPUAllocationFlags flags) { RN_NOT_IMPLEMENTED(); return GPUAllocation::Invalid; }
        virtual void                    GPUFree(GPUAllocation allocation) { RN_NOT_IMPLEMENTED(); }

        virtual Buffer                  CreateBuffer(const BufferDesc& desc, const GPUMemoryRegion& region) { RN_NOT_IMPLEMENTED(); return Buffer::Invalid; }
        virtual Texture2D               CreateTexture2D(const Texture2DDesc& desc, const GPUMemoryRegion& region) { RN_NOT_IMPLEMENTED(); return Texture2D::Invalid; }
        virtual Texture3D               CreateTexture3D(const Texture3DDesc& desc, const GPUMemoryRegion& region) { RN_NOT_IMPLEMENTED(); return Texture3D::Invalid; }

        virtual RenderTargetView        CreateRenderTargetView(const RenderTargetViewDesc& rtvDesc) { RN_NOT_IMPLEMENTED(); return RenderTargetView::Invalid; }
        virtual DepthStencilView        CreateDepthStencilView(const DepthStencilViewDesc& dsvDesc) { RN_NOT_IMPLEMENTED(); return DepthStencilView::Invalid; }
        virtual Texture2DView           CreateTexture2DView(const Texture2DViewDesc& desc) { RN_NOT_IMPLEMENTED(); return Texture2DView::Invalid; }
        virtual Texture3DView           CreateTexture3DView(const Texture3DViewDesc& desc) { RN_NOT_IMPLEMENTED(); return Texture3DView::Invalid; }
        virtual TextureCubeView         CreateTextureCubeView(const Texture2DViewDesc& desc) { RN_NOT_IMPLEMENTED(); return TextureCubeView::Invalid; }
        virtual Texture2DArrayView      CreateTexture2DArrayView(const Texture2DArrayViewDesc& desc) { RN_NOT_IMPLEMENTED(); return Texture2DArrayView::Invalid; };
        virtual BufferView              CreateBufferView(const BufferViewDesc& desc) { RN_NOT_IMPLEMENTED(); return BufferView::Invalid; }
        virtual TypedBufferView         CreateTypedBufferView(const TypedBufferViewDesc& desc) { RN_NOT_IMPLEMENTED(); return TypedBufferView::Invalid; }
        virtual UniformBufferView       CreateUniformBufferView(const UniformBufferViewDesc& desc) { RN_NOT_IMPLEMENTED(); return UniformBufferView::Invalid; }
        virtual RWTexture2DView         CreateRWTexture2DView(const RWTexture2DViewDesc& desc) { RN_NOT_IMPLEMENTED(); return RWTexture2DView::Invalid; }
        virtual RWTexture3DView         CreateRWTexture3DView(const RWTexture3DViewDesc& desc) { RN_NOT_IMPLEMENTED(); return RWTexture3DView::Invalid; }
        virtual RWTexture2DArrayView    CreateRWTexture2DArrayView(const RWTexture2DArrayViewDesc& desc) { RN_NOT_IMPLEMENTED(); return RWTexture2DArrayView::Invalid; }
        virtual RWBufferView            CreateRWBufferView(const RWBufferViewDesc& desc) { RN_NOT_IMPLEMENTED(); return RWBufferView::Invalid; }
        virtual TLASView                CreateTLASView(const ASViewDesc& desc) { RN_NOT_IMPLEMENTED(); return TLASView::Invalid; }
        virtual BLASView                CreateBLASView(const ASViewDesc& desc) { RN_NOT_IMPLEMENTED(); return BLASView::Invalid; }

        virtual void                    Destroy(Buffer buffer) { RN_NOT_IMPLEMENTED(); }
        virtual void                    Destroy(Texture2D texture) { RN_NOT_IMPLEMENTED(); }
        virtual void                    Destroy(Texture3D texture) { RN_NOT_IMPLEMENTED(); }

        virtual void                    Destroy(RenderTargetView view) { RN_NOT_IMPLEMENTED(); }
        virtual void                    Destroy(DepthStencilView view) { RN_NOT_IMPLEMENTED(); }
        virtual void                    Destroy(Texture2DView view) { RN_NOT_IMPLEMENTED(); }
        virtual void                    Destroy(Texture3DView view) { RN_NOT_IMPLEMENTED(); }
        virtual void                    Destroy(TextureCubeView view) { RN_NOT_IMPLEMENTED(); }
        virtual void                    Destroy(Texture2DArrayView view) { RN_NOT_IMPLEMENTED(); }
        virtual void                    Destroy(BufferView view) { RN_NOT_IMPLEMENTED(); }
        virtual void                    Destroy(TypedBufferView view) { RN_NOT_IMPLEMENTED(); }
        virtual void                    Destroy(UniformBufferView view) { RN_NOT_IMPLEMENTED(); }
        virtual void                    Destroy(RWTexture2DView view) { RN_NOT_IMPLEMENTED(); }
        virtual void                    Destroy(RWTexture3DView view) { RN_NOT_IMPLEMENTED(); }
        virtual void                    Destroy(RWTexture2DArrayView view) { RN_NOT_IMPLEMENTED(); }
        virtual void                    Destroy(RWBufferView view) { RN_NOT_IMPLEMENTED(); }
        virtual void                    Destroy(TLASView view) { RN_NOT_IMPLEMENTED(); }
        virtual void                    Destroy(BLASView view) { RN_NOT_IMPLEMENTED(); }

        virtual ResourceFootprint       CalculateTextureFootprint(const Texture2DDesc& desc) { RN_NOT_IMPLEMENTED(); return {}; }
        virtual ResourceFootprint       CalculateTextureFootprint(const Texture3DDesc& desc) { RN_NOT_IMPLEMENTED(); return {}; }
        virtual ASFootprint             CalculateBLASFootprint(std::initializer_list<BLASTriangleGeometryDesc> geometryDescs) { RN_NOT_IMPLEMENTED(); return {}; }
        virtual ASFootprint             CalculateTLASFootprint(Buffer instanceBuffer, uint32_t offsetInInstanceBuffer, uint32_t instanceCount) { RN_NOT_IMPLEMENTED(); return {}; }
        virtual SRTFootprint            CalculateShaderRecordTableFootprint(const SRTFootprintDesc& desc) { RN_NOT_IMPLEMENTED(); return {}; }

        virtual uint64_t                CalculateMipUploadDescs(const Texture2DDesc& desc, std::span<MipUploadDesc> outMipUploadDescs) { RN_NOT_IMPLEMENTED(); return 0; }
        virtual uint64_t                CalculateMipUploadDescs(const Texture3DDesc& desc, std::span<MipUploadDesc> outMipUploadDescs) { RN_NOT_IMPLEMENTED(); return 0; }

        virtual ResourceFootprint       CalculateTLASInstanceBufferFootprint(uint32_t instanceCount) { RN_NOT_IMPLEMENTED(); return {}; }
        virtual void                    PopulateTLASInstances(std::initializer_list<const TLASInstanceDesc> instances, std::span<unsigned char*> destData) { RN_NOT_IMPLEMENTED(); }

        // Command API
        virtual CommandList*            AllocateCommandList() { RN_NOT_IMPLEMENTED(); return nullptr; }
        virtual void                    SubmitCommandLists(std::span<CommandList*> cls) { RN_NOT_IMPLEMENTED(); }
    };

    void DestroyDevice(Device* device);
}