#pragma once

#include "rhi/command_list.hpp"
#include "rhi/pipeline.hpp"
#include "rhi/temporary_resource.hpp"
#include "d3d12_fwd.hpp"

namespace rn::rhi
{
    class DeviceD3D12;
    class CommandListD3D12 : public CommandList
    {
    public:

        void BeginRenderPass(const RenderPassBeginDesc& desc) override;
        void EndRenderPass() override;

        void SetScissorRect(const Rect& rect) override;
        void SetShadingRate(ShadingRate rate) override;

        void PushGraphicsConstantData(uint32_t offsetInData, const void* data, uint32_t dataSize) override;
        void PushComputeConstantData(uint32_t offsetInData, const void* data, uint32_t dataSize) override;

        void BindDrawIDBuffer(Buffer drawIDBuffer, uint32_t offsetInBuffer, uint32_t drawIDCount) override;

        void Draw(std::span<DrawPacket> packets) override;
        void DrawIndexed(std::span<IndexedDrawPacket> packets) override;
        void DrawIndirect(std::span<IndirectDrawPacket> packets) override;
        void DrawIndirectIndexed(std::span<IndirectIndexedDrawPacket> packets) override;
        void Dispatch(std::span<DispatchPacket> packets) override;
        void DispatchIndirect(std::span<IndirectDispatchPacket> packets) override;
        void DispatchMesh(std::span<DispatchMeshPacket> packets) override;
        void DispatchMeshIndirect(std::span<IndirectDispatchMeshPacket> packets) override;
        void DispatchRays(std::span<DispatchRaysPacket> packets) override;
        void DispatchRaysIndirect(std::span<IndirectDispatchRaysPacket> packets) override;

        void Barrier(const BarrierDesc& desc) override;

        TemporaryResource AllocateTemporaryResource(uint32_t sizeInBytes) override;

        void UploadBufferData(Buffer destBuffer, uint32_t destBufferOffset, std::span<const unsigned char> data) override;
        void UploadTextureData(Texture2D destTexture, uint32_t startMipIndex, std::span<const MipUploadDesc> mipDescs, std::span<const unsigned char> sourceData) override;
        void UploadTextureData(Texture3D destTexture, uint32_t startMipIndex, std::span<const MipUploadDesc> mipDescs, std::span<const unsigned char> sourceData) override;

        void CopyBufferRegion(Buffer dest, uint32_t destOffsetInBytes, Buffer src, uint32_t srcOffsetInBytes, uint32_t sizeInBytes) override;
        void CopyTexture(Texture2D dest, Texture2D src) override;
        void CopyTexture(Texture3D dest, Texture3D src) override;

        void UploadTLASInstances(Buffer instanceBuffer, uint32_t offsetInInstanceBuffer, std::span<const TLASInstanceDesc> instances) override;
        void BuildBLAS(const BLASBuildDesc& desc) override;
        void BuildTLAS(const TLASBuildDesc& desc) override;
        void BuildShaderRecordTable(const SRTBuildDesc& desc) override;

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
            TopologyType currentTopology = TopologyType::TriangleList;

            Buffer currentIndexBuffer = Buffer::Invalid;
            uint32_t currentOffsetInIndexBuffer = 0;
            IndexFormat currentIndexFormat = IndexFormat::Uint16;
        };

        DeviceD3D12* _device = nullptr;
        ID3D12GraphicsCommandList7* _cl = nullptr;

        TemporaryResourceAllocator* _uploadAllocator = nullptr;

        State _state = {};
    };
}