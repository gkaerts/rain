#pragma once

#include "rhi/device.hpp"
#include "rhi/handles.hpp"

#include "common/memory/object_pool.hpp"
#include "common/memory/vector.hpp"

#include "resource_d3d12.hpp"
#include "d3d12_fwd.hpp"

#include <atomic>
#include <mutex>

namespace rn::rhi
{
    RN_MEMORY_CATEGORY(RHI)

    constexpr const uint32_t MAX_FRAME_LATENCY = 3;

    class DeviceD3D12;
    struct DeviceD3D12Options;

    struct RasterPipelineData
    {
        TopologyType topology;
    };

    struct BLASData
    {

    };

    using RasterPipelinePool = ObjectPool<RasterPipeline, ID3D12PipelineState*, RasterPipelineData>;
    using ComputePipelinePool = ObjectPool<ComputePipeline, ID3D12PipelineState*>;
    using RTPipelinePool = ObjectPool<RTPipeline, ID3D12StateObject*>;
    using GPUAllocationPool = ObjectPool<GPUAllocation, ID3D12Heap*>;
    using BufferPool = ObjectPool<Buffer, ID3D12Resource*>;
    using Texture2DPool = ObjectPool<Texture2D, ID3D12Resource*>;
    using Texture3DPool = ObjectPool<Texture3D, ID3D12Resource*>;
    using BLASPool = ObjectPool<BLASView, BLASData>;

    using FnOnFinalize = void(*)(DeviceD3D12*, void*);


    class FinalizerQueue
    {
    public:

        void Queue(FnOnFinalize fn, void* data);
        void Flush(DeviceD3D12* device);

    private:

        static const uint32_t MAX_FAST_FINALIZER_ACTIONS = 128;

        struct FinalizerAction
        {
            FnOnFinalize onFinalize;
            void* data;
        };

        std::atomic_int32_t _highWatermark = 0;
        FinalizerAction _actions[MAX_FAST_FINALIZER_ACTIONS] = {};

        mutable std::mutex _overflowMutex;
        Vector<FinalizerAction> _overflowActions = MakeVector<FinalizerAction>(MemoryCategory::RHI);
    };

    class DeviceD3D12 : public Device
    {
    public:

        DeviceD3D12(const DeviceD3D12Options& options, const DeviceMemorySettings& memorySettings);
        ~DeviceD3D12();

        DeviceCaps Capabilities() const override { return _caps; }

        // Pipeline API
        RasterPipeline          CreateRasterPipeline(const VertexRasterPipelineDesc& desc) override;
        RasterPipeline          CreateRasterPipeline(const MeshRasterPipelineDesc& desc) override;
        ComputePipeline         CreateComputePipeline(const ComputePipelineDesc& desc) override;
        RTPipeline              CreateRTPipeline(const RTPipelineDesc& desc) override;
        void                    Destroy(RasterPipeline pipeline) override;
        void                    Destroy(ComputePipeline pipeline) override;
        void                    Destroy(RTPipeline pipeline) override;

        // Resource API
        GPUAllocation           GPUAlloc(MemoryCategoryID cat, uint64_t sizeInBytes, GPUAllocationFlags flags) override;
        void                    GPUFree(GPUAllocation allocation) override;

        Buffer                  CreateBuffer(const BufferDesc& desc, const GPUMemoryRegion& region) override;
        Texture2D               CreateTexture2D(const Texture2DDesc& desc, const GPUMemoryRegion& region) override;
        Texture3D               CreateTexture3D(const Texture3DDesc& desc, const GPUMemoryRegion& region) override;

        RenderTargetView        CreateRenderTargetView(const RenderTargetViewDesc& rtvDesc) override;
        DepthStencilView        CreateDepthStencilView(const DepthStencilViewDesc& dsvDesc) override;
        Texture2DView           CreateTexture2DView(const Texture2DViewDesc& desc) override;
        Texture3DView           CreateTexture3DView(const Texture3DViewDesc& desc) override;
        TextureCubeView         CreateTextureCubeView(const Texture2DViewDesc& desc) override;
        Texture2DArrayView      CreateTexture2DArrayView(const Texture2DArrayViewDesc& desc) override;
        BufferView              CreateBufferView(const BufferViewDesc& desc) override;
        TypedBufferView         CreateTypedBufferView(const TypedBufferViewDesc& desc) override;
        UniformBufferView       CreateUniformBufferView(const UniformBufferViewDesc& desc) override;
        RWTexture2DView         CreateRWTexture2DView(const RWTexture2DViewDesc& desc) override;
        RWTexture3DView         CreateRWTexture3DView(const RWTexture3DViewDesc& desc) override;
        RWTexture2DArrayView    CreateRWTexture2DArrayView(const RWTexture2DArrayViewDesc& desc) override;
        RWBufferView            CreateRWBufferView(const RWBufferViewDesc& desc) override;
        TLASView                CreateTLASView(const ASViewDesc& desc) override;
        BLASView                CreateBLASView(const ASViewDesc& desc) override;

        void                    Destroy(Buffer buffer) override;
        void                    Destroy(Texture2D texture) override;
        void                    Destroy(Texture3D texture) override;

        void                    Destroy(RenderTargetView view) override;
        void                    Destroy(DepthStencilView view) override;
        void                    Destroy(Texture2DView view) override;
        void                    Destroy(Texture3DView view) override;
        void                    Destroy(TextureCubeView view) override;
        void                    Destroy(Texture2DArrayView view) override;
        void                    Destroy(BufferView view) override;
        void                    Destroy(TypedBufferView view) override;
        void                    Destroy(UniformBufferView view) override;
        void                    Destroy(RWTexture2DView view) override;
        void                    Destroy(RWTexture3DView view) override;
        void                    Destroy(RWTexture2DArrayView view) override;
        void                    Destroy(RWBufferView view) override;
        void                    Destroy(TLASView view) override;
        void                    Destroy(BLASView view) override;

        ResourceFootprint       CalculateTextureFootprint(const Texture2DDesc& desc) override;
        ResourceFootprint       CalculateTextureFootprint(const Texture3DDesc& desc) override;

        ASFootprint             CalculateBLASFootprint(std::initializer_list<BLASTriangleGeometryDesc> geometryDescs) override;
        ASFootprint             CalculateTLASFootprint(Buffer instanceBuffer, uint32_t offsetInInstanceBuffer, uint32_t instanceCount) override;
        SRTFootprint            CalculateShaderRecordTableFootprint(const SRTFootprintDesc& desc) override;
        uint64_t                CalculateMipUploadDescs(const Texture2DDesc& desc, std::span<MipUploadDesc> outMipUploadDescs) override;
        uint64_t                CalculateMipUploadDescs(const Texture3DDesc& desc, std::span<MipUploadDesc> outMipUploadDescs) override;
        ResourceFootprint       CalculateTLASInstanceBufferFootprint(uint32_t instanceCount) override;
        void                    PopulateTLASInstances(std::initializer_list<const TLASInstanceDesc> instances, std::span<unsigned char*> destData) override;

    private:

        void QueueFrameFinalizerAction(FnOnFinalize fn, void* data);

        DeviceCaps _caps = {};

        IDXGIFactory7* _dxgiFactory = nullptr;
        IDXGIAdapter4* _dxgiAdapter = nullptr;

        ID3D12Device10* _d3dDevice = nullptr;
        ID3D12Debug3* _d3dDebug = nullptr;

        ID3D12RootSignature* _bindlessRootSignature = nullptr;
        ID3D12RootSignature* _rtLocalRootSignature = nullptr;

        RasterPipelinePool _rasterPipelines;
        ComputePipelinePool _computePipelines;
        RTPipelinePool _rtPipelines;
        GPUAllocationPool _gpuAllocations;
        BufferPool _buffers;
        Texture2DPool _texture2Ds;
        Texture3DPool _texture3Ds;
        BLASPool _blases;

        uint32_t _frameIndex = 0;
        FinalizerQueue _gpuFrameFinalizerQueues[MAX_FRAME_LATENCY];

        DescriptorHeap _resourceDescriptorHeap;
        DescriptorHeap _samplerDescriptorHeap;
        DescriptorHeap _rtvDescriptorHeap;
        DescriptorHeap _dsvDescriptorHeap;    

        ID3D12CommandQueue* _graphicsQueue = nullptr;
    };
}