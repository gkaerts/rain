#pragma once

#include "rhi/command_list.hpp"
#include "rhi/pipeline.hpp"
#include "rhi/temporary_resource.hpp"
#include "rhi/limits.hpp"

#include "common/memory/vector.hpp"
#include "common/memory/hash_map.hpp"

#include "d3d12_fwd.hpp"

#include <mutex>
#include <thread>

namespace rn::rhi
{
    class DeviceD3D12;
    class CommandListD3D12;

    RN_MEMORY_CATEGORY(RHI);

    class CommandListPool
    {
    public:

        ~CommandListPool();

        CommandListD3D12* GetCommandList(
            DeviceD3D12* device,
            uint64_t frameIndex, 
            TemporaryResourceAllocator* uploadAllocator,
            TemporaryResourceAllocator* readbackAllocator,
            ID3D12RootSignature* rootSignature, 
            ID3D12DescriptorHeap* resourceHeap, 
            ID3D12DescriptorHeap* samplerHeap);

        void ReturnCommandList(CommandListD3D12* cl);
        void ResetAllocators(uint64_t frameIndex);
        void Reset();

    private:

        struct ThreadCommandAllocators
        {
            ID3D12CommandAllocator* allocators[MAX_FRAME_LATENCY] = {};
        };

        std::mutex _mutex;
        Vector<CommandListD3D12*> _availableCommandLists = MakeVector<CommandListD3D12*>(MemoryCategory::RHI);
        HashMap<std::thread::id, ThreadCommandAllocators> _commandAllocators = MakeHashMap<std::thread::id, ThreadCommandAllocators>(MemoryCategory::RHI);
    };

    class CommandListD3D12 : public CommandList
    {
    public:

        CommandListD3D12(DeviceD3D12* device, ID3D12CommandAllocator* allocator);
        ~CommandListD3D12();

        void Reset(ID3D12CommandAllocator* allocator);
        void BindInitialState(
            TemporaryResourceAllocator* uploadAllocator,
            TemporaryResourceAllocator* readbackAllocator,
            ID3D12RootSignature* rootSignature, 
            ID3D12DescriptorHeap* resourceHeap, 
            ID3D12DescriptorHeap* samplerHeap);
        void Finalize();

        void BeginEvent(const char* fmt) override;
        void EndEvent() override;

        void BeginRenderPass(const RenderPassBeginDesc& desc) override;
        void EndRenderPass() override;

        void SetScissorRect(const Rect& rect) override;
        void SetShadingRate(ShadingRate rate) override;

        void PushGraphicsConstantData(uint32_t offsetInData, const void* data, uint32_t dataSize) override;
        void PushComputeConstantData(uint32_t offsetInData, const void* data, uint32_t dataSize) override;

        void BindDrawIDBuffer(Buffer drawIDBuffer, uint32_t offsetInBuffer, uint32_t drawIDCount) override;

        void Draw(Span<const DrawPacket> packets) override;
        void DrawIndexed(Span<const IndexedDrawPacket> packets) override;
        void DrawIndirect(Span<const IndirectDrawPacket> packets) override;
        void DrawIndirectIndexed(Span<const IndirectIndexedDrawPacket> packets) override;
        void Dispatch(Span<const DispatchPacket> packets) override;
        void DispatchIndirect(Span<const IndirectDispatchPacket> packets) override;
        void DispatchMesh(Span<const DispatchMeshPacket> packets) override;
        void DispatchMeshIndirect(Span<const IndirectDispatchMeshPacket> packets) override;
        void DispatchRays(Span<const DispatchRaysPacket> packets) override;
        void DispatchRaysIndirect(Span<const IndirectDispatchRaysPacket> packets) override;

        void Barrier(const BarrierDesc& desc) override;

        TemporaryResource AllocateTemporaryResource(uint32_t sizeInBytes) override;

        void UploadBufferData(Buffer destBuffer, uint32_t destBufferOffset, Span<const unsigned char> data) override;
        void UploadTextureData(Texture2D destTexture, uint32_t startMipIndex, Span<const MipUploadDesc> mipDescs, Span<const unsigned char> sourceData) override;
        void UploadTextureData(Texture3D destTexture, uint32_t startMipIndex, Span<const MipUploadDesc> mipDescs, Span<const unsigned char> sourceData) override;

        void CopyBufferRegion(Buffer dest, uint32_t destOffsetInBytes, Buffer src, uint32_t srcOffsetInBytes, uint32_t sizeInBytes) override;
        void CopyTexture(Texture2D dest, Texture2D src) override;
        void CopyTexture(Texture3D dest, Texture3D src) override;

        void UploadTLASInstances(Buffer instanceBuffer, uint32_t offsetInInstanceBuffer, Span<const TLASInstanceDesc> instances) override;
        void BuildBLAS(const BLASBuildDesc& desc) override;
        void BuildTLAS(const TLASBuildDesc& desc) override;
        void BuildShaderRecordTable(const SRTBuildDesc& desc) override;

        void QueueBufferReadback(Buffer buffer, uint32_t offsetInBuffer, uint32_t sizeInBytes, FnOnReadback onReadback, void* userData) override;
        void QueueTextureReadback(Texture2D texture, FnOnReadback onReadback, void* userData) override;
        void QueueTextureReadback(Texture3D texture, FnOnReadback onReadback, void* userData) override;

        ID3D12GraphicsCommandList7* D3DCommandList() const { return _cl; }

    private:

        void BindRasterPipeline(RasterPipeline pipeline);
        void BindComputePipeline(ComputePipeline pipeline);
        void BindRTPipeline(RTPipeline pipeline);
        void BindIndexBuffer(Buffer indexBuffer, uint32_t offsetInBuffer, IndexFormat format);

        struct State
        {
            ShadingRate currentShadingRate = ShadingRate::SR_1X1;
            Buffer currentDrawIDBuffer = Buffer::Invalid;
            uint32_t currentOffsetInDrawIDBuffer = 0;
            uint32_t currentDrawIDCount = 0;

            RasterPipeline currentRasterPipeline = RasterPipeline::Invalid;
            ComputePipeline currentComputePipeline = ComputePipeline::Invalid;
            RTPipeline currentRTPipeline = RTPipeline::Invalid;
            TopologyType currentTopology = TopologyType::Count;

            Buffer currentIndexBuffer = Buffer::Invalid;
            uint32_t currentOffsetInIndexBuffer = 0;
            IndexFormat currentIndexFormat = IndexFormat::Uint16;
        };

        DeviceD3D12* _device = nullptr;
        ID3D12GraphicsCommandList7* _cl = nullptr;

        TemporaryResourceAllocator* _uploadAllocator = nullptr;
        TemporaryResourceAllocator* _readbackAllocator = nullptr;

        State _state = {};
    };
}