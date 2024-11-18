#include "command_list_d3d12.hpp"
#include "device_d3d12.hpp"
#include "format_d3d12.hpp"
#include "rhi/limits.hpp"
#include "common/memory/vector.hpp"

#include "WinPixEventRuntime/pix3.h"

#include "d3d12.h"

namespace rn::rhi
{
    CommandListPool::~CommandListPool()
    {
        Reset();
    }

    void CommandListPool::Reset()
    {
        for (CommandListD3D12* cl : _availableCommandLists)
        {
            TrackedDelete(cl);
        }
        _availableCommandLists.clear();

        for (auto it : _commandAllocators)
        {
            for (uint32_t i = 0; i < MAX_FRAME_LATENCY; ++i)
            {
                it.second.allocators[i]->Release();
                it.second.allocators[i] = nullptr;
            }
        }
        _commandAllocators.clear();
    }

    CommandListD3D12* CommandListPool::GetCommandList(
        DeviceD3D12* device,
        uint64_t frameIndex,
        TemporaryResourceAllocator* uploadAllocator,
        TemporaryResourceAllocator* readbackAllocator,
        ID3D12RootSignature* rootSignature, 
        ID3D12DescriptorHeap* resourceHeap, 
        ID3D12DescriptorHeap* samplerHeap)
    {
        CommandListD3D12* cl = nullptr;
        ID3D12CommandAllocator* allocator = nullptr;
        {
            std::unique_lock L(_mutex);

            auto allocatorsIt = _commandAllocators.find(std::this_thread::get_id());
            if (allocatorsIt == _commandAllocators.end())
            {
                ID3D12Device10* d3dDevice = device->D3DDevice();

                ThreadCommandAllocators newAllocators = {};
                for (uint32_t i = 0; i < MAX_FRAME_LATENCY; ++i)
                {
                    RN_ASSERT(SUCCEEDED(d3dDevice->CreateCommandAllocator(
                        D3D12_COMMAND_LIST_TYPE_DIRECT,
                        __uuidof(ID3D12CommandAllocator),
                        (void**)&newAllocators.allocators[i])));
                }

                allocatorsIt = _commandAllocators.insert_or_assign(std::this_thread::get_id(), newAllocators).first;
            }

            allocator = allocatorsIt->second.allocators[frameIndex % MAX_FRAME_LATENCY];

            if (!_availableCommandLists.empty())
            {
                cl = _availableCommandLists.back();
                _availableCommandLists.pop_back();
            }
        }

        if (!cl)
        {
            cl = TrackedNew<CommandListD3D12>(MemoryCategory::RHI, device, allocator);
        }
        else
        {
            cl->Reset(allocator);
        }

        cl->BindInitialState(uploadAllocator, readbackAllocator, rootSignature, resourceHeap, samplerHeap);
        return cl;
    }

    void CommandListPool::ReturnCommandList(CommandListD3D12* cl)
    {
        std::unique_lock L(_mutex);
        _availableCommandLists.push_back(cl);
    }

    void CommandListPool::ResetAllocators(uint64_t frameIndex)
    {
        for (const auto& p : _commandAllocators)
        {
            RN_ASSERT(SUCCEEDED(p.second.allocators[frameIndex % MAX_FRAME_LATENCY]->Reset()));
        }
    }

    CommandListD3D12::CommandListD3D12(DeviceD3D12* device, ID3D12CommandAllocator* allocator)
        : _device(device)
    {
        ID3D12Device10* d3dDevice = device->D3DDevice();
        RN_ASSERT(SUCCEEDED(d3dDevice->CreateCommandList(
            0,
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            allocator,
            nullptr,
            __uuidof(ID3D12GraphicsCommandList7),
            (void**)(&_cl))));
    }

    CommandListD3D12::~CommandListD3D12()
    {
        if (_cl)
        {
            _cl->Release();
            _cl = nullptr;
        }
    }

    void CommandListD3D12::Reset(ID3D12CommandAllocator* allocator)
    {
        _cl->Reset(allocator, nullptr);
        _state = {};
    }

    void CommandListD3D12::BindInitialState(
        TemporaryResourceAllocator* uploadAllocator, 
        TemporaryResourceAllocator* readbackAllocator,
        ID3D12RootSignature* rootSignature, 
        ID3D12DescriptorHeap* resourceHeap, 
        ID3D12DescriptorHeap* samplerHeap)
    {
        _uploadAllocator = uploadAllocator;
        _readbackAllocator = readbackAllocator;
        
        ID3D12DescriptorHeap* const heaps[] = {
            resourceHeap,
            samplerHeap
        };

        _cl->SetDescriptorHeaps(RN_ARRAY_SIZE(heaps), heaps);
        _cl->SetGraphicsRootSignature(rootSignature);
        _cl->SetComputeRootSignature(rootSignature);
    }

    void CommandListD3D12::Finalize()
    {
        RN_ASSERT(SUCCEEDED(_cl->Close()));
        _uploadAllocator = nullptr;
        _readbackAllocator = nullptr;
    }

    void CommandListD3D12::BeginEvent(const char* fmt)
    {
        PIXBeginEvent(_cl, 0, fmt);
    }

    void CommandListD3D12::EndEvent()
    {
        PIXEndEvent(_cl);
    }


    void CommandListD3D12::BeginRenderPass(const RenderPassBeginDesc& desc)
    {
        D3D12_CPU_DESCRIPTOR_HANDLE dsvDescriptor = {};
        D3D12_CPU_DESCRIPTOR_HANDLE* dsvDescriptorPtr = nullptr;
        if (IsValid(desc.depthTarget.view))
        {
            dsvDescriptor = _device->Resolve(desc.depthTarget.view);
            dsvDescriptorPtr = &dsvDescriptor;

            switch(desc.depthTarget.loadOp)
            {
                case LoadOp::Clear:
                    _cl->ClearDepthStencilView(
                        dsvDescriptor,
                        D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
                        desc.depthTarget.clearValue.depthStencil.depth,
                        desc.depthTarget.clearValue.depthStencil.stencil,
                        0,
                        nullptr);
                    break;
                case LoadOp::Load:
                case LoadOp::DoNotCare:
                default:
                    break;
            };
        }
        
        MemoryScope SCOPE;
        ScopedVector<D3D12_CPU_DESCRIPTOR_HANDLE> rtvDescriptors;
        rtvDescriptors.reserve(desc.renderTargets.size());

        for (const RenderPassRenderTarget& rt : desc.renderTargets)
        {
            D3D12_CPU_DESCRIPTOR_HANDLE rtvDescriptor = _device->Resolve(rt.view);
            
            switch(rt.loadOp)
            {
                case LoadOp::Clear:
                    _cl->ClearRenderTargetView(rtvDescriptor, rt.clearValue.color, 0, nullptr);
                    break;
                case LoadOp::Load:
                case LoadOp::DoNotCare:
                default:
                    break;
            };

            rtvDescriptors.push_back(rtvDescriptor);
        }
        
        _cl->OMSetRenderTargets(UINT(rtvDescriptors.size()), rtvDescriptors.data(), FALSE, dsvDescriptorPtr);

        D3D12_VIEWPORT viewport = 
        {
            .TopLeftX = float(desc.viewport.x),
            .TopLeftY = float(desc.viewport.y),
            .Width = float(desc.viewport.width),
            .Height = float(desc.viewport.height),
            .MinDepth = 0.0f,
            .MaxDepth = 1.0f
        };

        D3D12_RECT scissor = 
        {
            .left = LONG(desc.viewport.x),
            .top = LONG(desc.viewport.y),
            .right = LONG(desc.viewport.x + desc.viewport.width),
            .bottom = LONG(desc.viewport.y + desc.viewport.height)
        };

        _cl->RSSetViewports(1, &viewport);
        _cl->RSSetScissorRects(1, &scissor);
    }

    void CommandListD3D12::EndRenderPass()
    {}

    void CommandListD3D12::SetScissorRect(const Rect& rect)
    {
        D3D12_RECT d3dScissorRect
        {
            LONG(rect.x),
            LONG(rect.y),
            LONG(rect.x + rect.width),
            LONG(rect.y + rect.height)
        };

        _cl->RSSetScissorRects(1, &d3dScissorRect);
    }

    void CommandListD3D12::SetShadingRate(ShadingRate rate)
    {
        constexpr const D3D12_SHADING_RATE SHADING_RATES[] =
        {
            D3D12_SHADING_RATE_1X1,
            D3D12_SHADING_RATE_1X2,
            D3D12_SHADING_RATE_2X1,
            D3D12_SHADING_RATE_2X2,
            D3D12_SHADING_RATE_2X4,
            D3D12_SHADING_RATE_4X2,
            D3D12_SHADING_RATE_4X4
        };

        if (rate != _state.currentShadingRate)
        {
            _cl->RSSetShadingRate(SHADING_RATES[int(rate)], nullptr);
            _state.currentShadingRate = rate;
        }
    }

    void CommandListD3D12::PushGraphicsConstantData(uint32_t offsetInData, const void* data, uint32_t dataSize)
    {
        uint32_t numUints = (dataSize + (sizeof(uint32_t) - 1)) / sizeof(uint32_t);
        uint32_t uintOffset = (offsetInData + (sizeof(uint32_t) - 1)) / sizeof(uint32_t);

        _cl->SetGraphicsRoot32BitConstants(0, numUints, data, uintOffset);
    }

    void CommandListD3D12::PushComputeConstantData(uint32_t offsetInData, const void* data, uint32_t dataSize)
    {
        uint32_t numUints = (dataSize + (sizeof(uint32_t) - 1)) / sizeof(uint32_t);
        uint32_t uintOffset = (offsetInData + (sizeof(uint32_t) - 1)) / sizeof(uint32_t);

        _cl->SetComputeRoot32BitConstants(0, numUints, data, uintOffset);
    }

    void CommandListD3D12::BindDrawIDBuffer(Buffer drawIDBuffer, uint32_t offsetInBuffer, uint32_t drawIDCount)
    {
        if (drawIDBuffer != _state.currentDrawIDBuffer ||
            offsetInBuffer != _state.currentOffsetInDrawIDBuffer ||
            drawIDCount != _state.currentDrawIDCount)
        {
            ID3D12Resource* buffer = _device->Resolve(drawIDBuffer);

            D3D12_VERTEX_BUFFER_VIEW vbView = 
            {
                .BufferLocation = buffer->GetGPUVirtualAddress() + offsetInBuffer,
                .SizeInBytes = drawIDCount * sizeof(uint32_t),
                .StrideInBytes = sizeof(uint32_t)
            };

            _cl->IASetVertexBuffers(0, 1, &vbView);

            _state.currentDrawIDBuffer = drawIDBuffer;
            _state.currentOffsetInDrawIDBuffer = offsetInBuffer;
            _state.currentDrawIDCount = drawIDCount;
        }
    }

    namespace
    {
        constexpr const D3D_PRIMITIVE_TOPOLOGY D3D_TOPOLOGIES[] =
        {
            D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
            D3D_PRIMITIVE_TOPOLOGY_LINELIST,
        };
    }

    void CommandListD3D12::BindRasterPipeline(RasterPipeline pipeline)
    {
        if (_state.currentRasterPipeline != pipeline)
        {
            ID3D12PipelineState* pso = _device->Resolve(pipeline);
            RasterPipelineData pipelineData = _device->ResolveAdditionalData(pipeline);

            if (_state.currentTopology != pipelineData.topology)
            {
                _cl->IASetPrimitiveTopology(D3D_TOPOLOGIES[int(pipelineData.topology)]);
                _state.currentTopology = pipelineData.topology;
            }

            _cl->SetPipelineState(pso);
            _state.currentRasterPipeline = pipeline;
            _state.currentComputePipeline = ComputePipeline::Invalid;
            _state.currentRTPipeline = RTPipeline::Invalid;
        }
    }

    void CommandListD3D12::BindComputePipeline(ComputePipeline pipeline)
    {
        if (_state.currentComputePipeline != pipeline)
        {
            ID3D12PipelineState* pso = _device->Resolve(pipeline);
            _cl->SetPipelineState(pso);
            _state.currentComputePipeline = pipeline;
            _state.currentRasterPipeline = RasterPipeline::Invalid;
            _state.currentRTPipeline = RTPipeline::Invalid;
        }
    }

    void CommandListD3D12::BindRTPipeline(RTPipeline pipeline)
    {
        if (_state.currentRTPipeline != pipeline)
        {
            ID3D12StateObject* pso = _device->Resolve(pipeline);
            _cl->SetPipelineState1(pso);
            _state.currentRTPipeline = pipeline;
            _state.currentComputePipeline = ComputePipeline::Invalid;
            _state.currentRasterPipeline = RasterPipeline::Invalid;
        }
    }

    void CommandListD3D12::BindIndexBuffer(Buffer indexBuffer, uint32_t offsetInBuffer, IndexFormat format)
    {
        if (_state.currentIndexBuffer != indexBuffer ||
            _state.currentOffsetInIndexBuffer != offsetInBuffer ||
            _state.currentIndexFormat != format)
        {
            ID3D12Resource* buffer = _device->Resolve(indexBuffer);

            D3D12_RESOURCE_DESC desc = buffer->GetDesc();
            D3D12_INDEX_BUFFER_VIEW ibView =
            {
                .BufferLocation = buffer->GetGPUVirtualAddress() + offsetInBuffer,
                .SizeInBytes = UINT(desc.Width - offsetInBuffer),
                .Format = format == IndexFormat::Uint16 ? 
                    DXGI_FORMAT_R16_UINT : 
                    DXGI_FORMAT_R32_UINT,
            };

            _cl->IASetIndexBuffer(&ibView);

            _state.currentIndexBuffer = indexBuffer;
            _state.currentOffsetInIndexBuffer = offsetInBuffer;
            _state.currentIndexFormat = format;
        }
    }

    void CommandListD3D12::Draw(Span<const DrawPacket> packets)
    {
        for (const DrawPacket& draw : packets)
        {
            BindRasterPipeline(draw.pipeline);
            _cl->DrawInstanced(draw.vertexCount, draw.instanceCount, 0, draw.drawID);
        }
    }

    void CommandListD3D12::DrawIndexed(Span<const IndexedDrawPacket> packets)
    {
        for (const IndexedDrawPacket& draw : packets)
        {
            BindRasterPipeline(draw.pipeline);
            BindIndexBuffer(draw.indexBuffer, draw.offsetInIndexBuffer, draw.indexFormat);
            _cl->DrawIndexedInstanced(draw.indexCount, draw.instanceCount, 0, 0, draw.drawID);
        }
    }

    void CommandListD3D12::DrawIndirect(Span<const IndirectDrawPacket> packets)
    {
        ID3D12CommandSignature* signature = _device->CommandSignature(CommandSignatureType::Draw);
        for (const IndirectDrawPacket& draw : packets)
        {
            BindRasterPipeline(draw.pipeline);

            ID3D12Resource* argsBuffer = _device->Resolve(draw.argsBuffer);
            _cl->ExecuteIndirect(signature, 1, argsBuffer, draw.offsetInArgsBuffer, nullptr, 0);
        }
    }

    void CommandListD3D12::DrawIndirectIndexed(Span<const IndirectIndexedDrawPacket> packets)
    {
        ID3D12CommandSignature* signature = _device->CommandSignature(CommandSignatureType::DrawIndexed);
        for (const IndirectIndexedDrawPacket& draw : packets)
        {
            BindRasterPipeline(draw.pipeline);
            BindIndexBuffer(draw.indexBuffer, draw.offsetInIndexBuffer, draw.indexFormat);

            ID3D12Resource* argsBuffer = _device->Resolve(draw.argsBuffer);
            _cl->ExecuteIndirect(signature, 1, argsBuffer, draw.offsetInArgsBuffer, nullptr, 0);
        }
    }

    void CommandListD3D12::Dispatch(Span<const DispatchPacket> packets)
    {
        for (const DispatchPacket& dispatch : packets)
        {
            BindComputePipeline(dispatch.pipeline);
            _cl->Dispatch(dispatch.x, dispatch.y, dispatch.z);
        }
    }

    void CommandListD3D12::DispatchIndirect(Span<const IndirectDispatchPacket> packets)
    {
        ID3D12CommandSignature* signature = _device->CommandSignature(CommandSignatureType::Dispatch);
        for (const IndirectDispatchPacket& dispatch : packets)
        {
            BindComputePipeline(dispatch.pipeline);
            
            ID3D12Resource* argsBuffer = _device->Resolve(dispatch.argsBuffer);
            _cl->ExecuteIndirect(signature, 1, argsBuffer, dispatch.offsetInArgsBuffer, nullptr, 0);
        }
    }

    void CommandListD3D12::DispatchMesh(Span<const DispatchMeshPacket> packets)
    {
        for (const DispatchMeshPacket& dispatch : packets)
        {
            BindRasterPipeline(dispatch.pipeline);
            _cl->DispatchMesh(dispatch.x, dispatch.y, dispatch.z);
        }
    }

    void CommandListD3D12::DispatchMeshIndirect(Span<const IndirectDispatchMeshPacket> packets)
    {
        ID3D12CommandSignature* signature = _device->CommandSignature(CommandSignatureType::DispatchMesh);
        for (const IndirectDispatchMeshPacket& dispatch : packets)
        {
            BindRasterPipeline(dispatch.pipeline);

            ID3D12Resource* argsBuffer = _device->Resolve(dispatch.argsBuffer);
            _cl->ExecuteIndirect(signature, 1, argsBuffer, dispatch.offsetInArgsBuffer, nullptr, 0);
        }
    }

    void CommandListD3D12::DispatchRays(Span<const DispatchRaysPacket> packets)
    {
        for (const DispatchRaysPacket& packet : packets)
        {
            ID3D12Resource* rayGenBuffer = _device->Resolve(packet.rayGenBuffer.recordBuffer);
            ID3D12Resource* missBuffer = _device->Resolve(packet.missBuffer.recordBuffer);
            ID3D12Resource* hitGroupBuffer = _device->Resolve(packet.hitGroupBuffer.recordBuffer);

            RN_ASSERT(rayGenBuffer);
            RN_ASSERT(missBuffer);
            RN_ASSERT(hitGroupBuffer);

            BindRTPipeline(packet.pipeline);

            D3D12_DISPATCH_RAYS_DESC desc = {
                .RayGenerationShaderRecord = {
                    .StartAddress = rayGenBuffer->GetGPUVirtualAddress() + packet.rayGenBuffer.offsetInRecordBuffer,
                    .SizeInBytes = packet.rayGenBuffer.bufferSize
                },
                
                .MissShaderTable = {
                    .StartAddress = missBuffer->GetGPUVirtualAddress() + packet.missBuffer.offsetInRecordBuffer,
                    .SizeInBytes = packet.missBuffer.bufferSize,
                    .StrideInBytes = packet.missBuffer.recordStride
                },

                .HitGroupTable = {
                    .StartAddress = hitGroupBuffer->GetGPUVirtualAddress() + packet.hitGroupBuffer.offsetInRecordBuffer,
                    .SizeInBytes = packet.hitGroupBuffer.bufferSize,
                    .StrideInBytes = packet.hitGroupBuffer.recordStride
                },

                .CallableShaderTable = {},

                .Width = packet.x,
                .Height = packet.y,
                .Depth = packet.z,
            };

            _cl->DispatchRays(&desc);
        }
    }

    void CommandListD3D12::DispatchRaysIndirect(Span<const IndirectDispatchRaysPacket> packets)
    {
        ID3D12CommandSignature* signature = _device->CommandSignature(CommandSignatureType::DispatchRays);
        for (const IndirectDispatchRaysPacket& packet : packets)
        {
            BindRTPipeline(packet.pipeline);

            ID3D12Resource* argsBuffer = _device->Resolve(packet.argsBuffer);
            _cl->ExecuteIndirect(signature, 1, argsBuffer, packet.offsetInArgsBuffer, nullptr, 0);
        }
    }

    namespace
    {
        D3D12_BARRIER_SYNC ToBarrierSync(PipelineSyncStage sync)
        {
            D3D12_BARRIER_SYNC d3dSync = D3D12_BARRIER_SYNC_NONE;

            #define MATCH_FLAG(d3dFlag, rnFlag) if (TestFlag(sync, rnFlag)) { d3dSync |= d3dFlag; }

            MATCH_FLAG(D3D12_BARRIER_SYNC_EXECUTE_INDIRECT, PipelineSyncStage::IndirectCommand);
            MATCH_FLAG(D3D12_BARRIER_SYNC_INDEX_INPUT,      PipelineSyncStage::InputAssembly);
            MATCH_FLAG(D3D12_BARRIER_SYNC_VERTEX_SHADING,   PipelineSyncStage::VertexShader);
            MATCH_FLAG(D3D12_BARRIER_SYNC_PIXEL_SHADING,    PipelineSyncStage::PixelShader);
            MATCH_FLAG(D3D12_BARRIER_SYNC_DEPTH_STENCIL,    PipelineSyncStage::EarlyDepthTest);
            MATCH_FLAG(D3D12_BARRIER_SYNC_DEPTH_STENCIL,    PipelineSyncStage::LateDepthTest);
            MATCH_FLAG(D3D12_BARRIER_SYNC_RENDER_TARGET,    PipelineSyncStage::RenderTargetOutput);
            MATCH_FLAG(D3D12_BARRIER_SYNC_COMPUTE_SHADING,  PipelineSyncStage::ComputeShader);
            MATCH_FLAG(D3D12_BARRIER_SYNC_RAYTRACING,       PipelineSyncStage::RayTracing);
            MATCH_FLAG(D3D12_BARRIER_SYNC_COPY,             PipelineSyncStage::Copy);

            MATCH_FLAG(D3D12_BARRIER_SYNC_BUILD_RAYTRACING_ACCELERATION_STRUCTURE,  PipelineSyncStage::BuildAccelerationStructure);
            MATCH_FLAG(D3D12_BARRIER_SYNC_COPY_RAYTRACING_ACCELERATION_STRUCTURE,   PipelineSyncStage::CopyAccelerationStructure);

            #undef MATCH_FLAG

            return d3dSync;
        }

        D3D12_BARRIER_ACCESS ToBarrierAccess(PipelineAccess access)
        {
            if (access == PipelineAccess::None)
            {
                return D3D12_BARRIER_ACCESS_NO_ACCESS;
            }

            D3D12_BARRIER_ACCESS d3dAccess = D3D12_BARRIER_ACCESS_COMMON;

            #define MATCH_FLAG(d3dFlag, rnFlag) if (TestFlag(access, rnFlag)) { d3dAccess |= d3dFlag; }

            MATCH_FLAG(D3D12_BARRIER_ACCESS_INDIRECT_ARGUMENT,      PipelineAccess::CommandInput);
            MATCH_FLAG(D3D12_BARRIER_ACCESS_VERTEX_BUFFER,          PipelineAccess::VertexInput);
            MATCH_FLAG(D3D12_BARRIER_ACCESS_INDEX_BUFFER,           PipelineAccess::IndexInput);
            MATCH_FLAG(D3D12_BARRIER_ACCESS_SHADER_RESOURCE,        PipelineAccess::ShaderRead);
            MATCH_FLAG(D3D12_BARRIER_ACCESS_UNORDERED_ACCESS,       PipelineAccess::ShaderReadWrite);
            MATCH_FLAG(D3D12_BARRIER_ACCESS_RENDER_TARGET,          PipelineAccess::RenderTargetWrite);
            MATCH_FLAG(D3D12_BARRIER_ACCESS_DEPTH_STENCIL_READ,     PipelineAccess::DepthTargetRead);
            MATCH_FLAG(D3D12_BARRIER_ACCESS_DEPTH_STENCIL_WRITE,    PipelineAccess::DepthTargetReadWrite);
            MATCH_FLAG(D3D12_BARRIER_ACCESS_COPY_SOURCE,            PipelineAccess::CopyRead);
            MATCH_FLAG(D3D12_BARRIER_ACCESS_COPY_DEST,              PipelineAccess::CopyWrite);
            MATCH_FLAG(D3D12_BARRIER_ACCESS_CONSTANT_BUFFER,        PipelineAccess::UniformBuffer);

            MATCH_FLAG(D3D12_BARRIER_ACCESS_RAYTRACING_ACCELERATION_STRUCTURE_READ,     PipelineAccess::AccelerationStructureRead);
            MATCH_FLAG(D3D12_BARRIER_ACCESS_RAYTRACING_ACCELERATION_STRUCTURE_WRITE,    PipelineAccess::AccelerationStructureWrite);

            #undef MATCH_FLAG

            return d3dAccess;
        }

        constexpr const D3D12_BARRIER_LAYOUT BARRIER_LAYOUTS[] = 
        {
            D3D12_BARRIER_LAYOUT_UNDEFINED,             // Undefined
            D3D12_BARRIER_LAYOUT_COMMON,                // General
            D3D12_BARRIER_LAYOUT_RENDER_TARGET,         // RenderTarget
            D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_READ,    // DepthTargetRead
            D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_WRITE,   // DepthTargetReadWrite
            D3D12_BARRIER_LAYOUT_SHADER_RESOURCE,       // ShaderRead
            D3D12_BARRIER_LAYOUT_UNORDERED_ACCESS,      // ShaderReadWrite
            D3D12_BARRIER_LAYOUT_COPY_SOURCE,           // CopyRead
            D3D12_BARRIER_LAYOUT_COPY_DEST,             // CopyWrite
            D3D12_BARRIER_LAYOUT_PRESENT,               // Present
        };
        RN_MATCH_ENUM_AND_ARRAY(BARRIER_LAYOUTS, TextureLayout);
    }

    void CommandListD3D12::Barrier(const BarrierDesc& desc)
    {
        MemoryScope SCOPE;

        constexpr const uint32_t MAX_BARRIER_GROUPS = 4;

        ScopedVector<D3D12_GLOBAL_BARRIER> globalBarriers;
        ScopedVector<D3D12_TEXTURE_BARRIER> texture2DBarriers;
        ScopedVector<D3D12_TEXTURE_BARRIER> texture3DBarriers;
        ScopedVector<D3D12_BUFFER_BARRIER> bufferBarriers;
        globalBarriers.reserve(desc.pipelineBarriers.size());
        texture2DBarriers.reserve(desc.texture2DBarriers.size());
        texture3DBarriers.reserve(desc.texture3DBarriers.size());
        bufferBarriers.reserve(desc.bufferBarriers.size());

        ScopedVector<D3D12_BARRIER_GROUP> barrierGroups;
        barrierGroups.reserve(MAX_BARRIER_GROUPS);

        if (desc.pipelineBarriers.size() > 0)
        {
            for (const PipelineBarrier& barrier : desc.pipelineBarriers)
            {
                globalBarriers.push_back({
                    .SyncBefore = ToBarrierSync(barrier.fromStage),
                    .SyncAfter = ToBarrierSync(barrier.toStage),
                    .AccessBefore = ToBarrierAccess(barrier.fromAccess),
                    .AccessAfter = ToBarrierAccess(barrier.toAccess)
                });
            }

            barrierGroups.push_back({
                .Type = D3D12_BARRIER_TYPE_GLOBAL,
                .NumBarriers = UINT(globalBarriers.size()),
                .pGlobalBarriers = globalBarriers.data()
            });
        }
        
        if (desc.texture2DBarriers.size() > 0)
        {
            for (const Texture2DBarrier& barrier : desc.texture2DBarriers)
            {
                ID3D12Resource* resource = _device->Resolve(barrier.handle);
                texture2DBarriers.push_back({
                    .SyncBefore = ToBarrierSync(barrier.fromStage),
                    .SyncAfter = ToBarrierSync(barrier.toStage),
                    .AccessBefore = ToBarrierAccess(barrier.fromAccess),
                    .AccessAfter = ToBarrierAccess(barrier.toAccess),
                    .LayoutBefore = BARRIER_LAYOUTS[int(barrier.fromLayout)],
                    .LayoutAfter = BARRIER_LAYOUTS[int(barrier.toLayout)],
                    .pResource = resource,
                    .Subresources = {
                        .IndexOrFirstMipLevel = barrier.firstMipLevel,
                        .NumMipLevels = barrier.numMips,
                        .FirstArraySlice = barrier.firstArraySlice,
                        .NumArraySlices = barrier.numArraySlices,
                        .FirstPlane = 0,
                        .NumPlanes = 1
                    }
                });
            }

            barrierGroups.push_back({
                .Type = D3D12_BARRIER_TYPE_TEXTURE,
                .NumBarriers = UINT(texture2DBarriers.size()),
                .pTextureBarriers = texture2DBarriers.data()
            });
        }

        if (desc.texture3DBarriers.size() > 0)
        {
            for (const Texture3DBarrier& barrier : desc.texture3DBarriers)
            {
                ID3D12Resource* resource = _device->Resolve(barrier.handle);
                texture3DBarriers.push_back({
                    .SyncBefore = ToBarrierSync(barrier.fromStage),
                    .SyncAfter = ToBarrierSync(barrier.toStage),
                    .AccessBefore = ToBarrierAccess(barrier.fromAccess),
                    .AccessAfter = ToBarrierAccess(barrier.toAccess),
                    .LayoutBefore = BARRIER_LAYOUTS[int(barrier.fromLayout)],
                    .LayoutAfter = BARRIER_LAYOUTS[int(barrier.toLayout)],
                    .pResource = resource,
                    .Subresources = {
                        .IndexOrFirstMipLevel = barrier.firstMipLevel,
                        .NumMipLevels = barrier.numMips,
                        .FirstArraySlice = 0,
                        .NumArraySlices = 1,
                        .FirstPlane = 0,
                        .NumPlanes = 1
                    }
                });
            }

            barrierGroups.push_back({
                .Type = D3D12_BARRIER_TYPE_TEXTURE,
                .NumBarriers = UINT(texture3DBarriers.size()),
                .pTextureBarriers = texture3DBarriers.data()
            });
        }

        if (desc.bufferBarriers.size() > 0)
        {
            for (const BufferBarrier& barrier : desc.bufferBarriers)
            {
                ID3D12Resource* resource = _device->Resolve(barrier.handle);
                bufferBarriers.push_back({
                    .SyncBefore = ToBarrierSync(barrier.fromStage),
                    .SyncAfter = ToBarrierSync(barrier.toStage),
                    .AccessBefore = ToBarrierAccess(barrier.fromAccess),
                    .AccessAfter = ToBarrierAccess(barrier.toAccess),
                    .pResource = resource,
                    .Offset = barrier.offset,
                    .Size = barrier.size,
                });
            }

            barrierGroups.push_back({
                .Type = D3D12_BARRIER_TYPE_BUFFER,
                .NumBarriers = UINT(bufferBarriers.size()),
                .pBufferBarriers = bufferBarriers.data()
            });
        }

        _cl->Barrier(UINT(barrierGroups.size()), barrierGroups.data());
    }

    TemporaryResource CommandListD3D12::AllocateTemporaryResource(uint32_t sizeInBytes)
    {
        return _uploadAllocator->AllocateTemporaryResource(sizeInBytes, 256);
    }

    void CommandListD3D12::UploadBufferData(Buffer destBuffer, uint32_t destBufferOffset, Span<const unsigned char> data)
    {
        ID3D12Resource* resource = _device->Resolve(destBuffer);

        D3D12_HEAP_PROPERTIES heapProperties = {};
        D3D12_HEAP_FLAGS heapFlags = D3D12_HEAP_FLAGS(0);
        RN_ASSERT(SUCCEEDED(resource->GetHeapProperties(&heapProperties, &heapFlags)));
        RN_ASSERT(heapProperties.Type != D3D12_HEAP_TYPE_READBACK);

        if (heapProperties.Type == D3D12_HEAP_TYPE_UPLOAD || 
            (heapProperties.Type == D3D12_HEAP_TYPE_CUSTOM && heapProperties.CPUPageProperty == D3D12_CPU_PAGE_PROPERTY_WRITE_COMBINE))
        {
            ID3D12Resource* resource = _device->Resolve(destBuffer);

            D3D12_RANGE range = {
                .Begin = destBufferOffset,
                .End = destBufferOffset + UINT(data.size())
            };

            void* mappedPtr = nullptr;
            RN_ASSERT(SUCCEEDED(resource->Map(0, &range, &mappedPtr)));
            std::memcpy(mappedPtr, data.data(), data.size());

            resource->Unmap(0, &range);
        }
        else if (heapProperties.Type == D3D12_HEAP_TYPE_DEFAULT)
        {
            TemporaryResource tempResource = AllocateTemporaryResource(uint32_t(data.size()));
            std::memcpy(tempResource.cpuPtr, data.data(), data.size());
            CopyBufferRegion(destBuffer, destBufferOffset, tempResource.buffer, tempResource.offsetInBytes, uint32_t(data.size()));
        }
    }

    void CommandListD3D12::UploadTextureData(Texture2D destTexture, uint32_t startMipIndex, Span<const MipUploadDesc> mipDescs, Span<const unsigned char> sourceData)
    {
        ID3D12Resource* texResource = _device->Resolve(destTexture);
        TemporaryResource tempResource = AllocateTemporaryResource(uint32_t(sourceData.size()));

        std::memcpy(tempResource.cpuPtr, sourceData.data(), sourceData.size());
        ID3D12Resource* bufferResource = _device->Resolve(tempResource.buffer);

        uint32_t subresourceIdx = 0;
        for (const MipUploadDesc& mipDesc : mipDescs)
        {
            D3D12_TEXTURE_COPY_LOCATION destLocation = {
                .pResource = texResource,
                .Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
                .SubresourceIndex = subresourceIdx,
            };

            D3D12_TEXTURE_COPY_LOCATION sourceLocation = {
                .pResource = bufferResource,
                .Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT,
                .PlacedFootprint = {
                    .Offset = tempResource.offsetInBytes + mipDesc.offsetInUploadBuffer,
                    .Footprint = {
                        .Format = ToDXGIFormat(mipDesc.format),
                        .Width = mipDesc.width,
                        .Height = mipDesc.height,
                        .Depth = mipDesc.depth,
                        .RowPitch = mipDesc.rowPitch,
                    }
                }
            };

            _cl->CopyTextureRegion(
                &destLocation,
                0,
                0,
                0,
                &sourceLocation,
                nullptr);

            ++subresourceIdx;
        }
    }

    void CommandListD3D12::UploadTextureData(Texture3D destTexture, uint32_t startMipIndex, Span<const MipUploadDesc> mipDescs, Span<const unsigned char> sourceData)
    {
        ID3D12Resource* texResource = _device->Resolve(destTexture);
        TemporaryResource tempResource = AllocateTemporaryResource(uint32_t(sourceData.size()));

        std::memcpy(tempResource.cpuPtr, sourceData.data(), sourceData.size());
        ID3D12Resource* bufferResource = _device->Resolve(tempResource.buffer);

        uint32_t subresourceIdx = 0;
        for (const MipUploadDesc& mipDesc : mipDescs)
        {
            D3D12_TEXTURE_COPY_LOCATION destLocation = {
                .pResource = texResource,
                .Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
                .SubresourceIndex = subresourceIdx,
            };

            D3D12_TEXTURE_COPY_LOCATION sourceLocation = {
                .pResource = bufferResource,
                .Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT,
                .PlacedFootprint = {
                    .Offset = tempResource.offsetInBytes + mipDesc.offsetInUploadBuffer,
                    .Footprint = {
                        .Format = ToDXGIFormat(mipDesc.format),
                        .Width = mipDesc.width,
                        .Height = mipDesc.height,
                        .Depth = mipDesc.depth,
                        .RowPitch = mipDesc.rowPitch,
                    }
                }
            };

            _cl->CopyTextureRegion(
                &destLocation,
                0,
                0,
                0,
                &sourceLocation,
                nullptr);

            ++subresourceIdx;
        }
    }

    void CommandListD3D12::CopyBufferRegion(Buffer dest, uint32_t destOffsetInBytes, Buffer src, uint32_t srcOffsetInBytes, uint32_t sizeInBytes)
    {
        ID3D12Resource* destBuffer = _device->Resolve(dest);
        ID3D12Resource* srcBuffer = _device->Resolve(src);
        _cl->CopyBufferRegion(destBuffer, destOffsetInBytes, srcBuffer, srcOffsetInBytes, sizeInBytes);
    }

    void CommandListD3D12::CopyTexture(Texture2D dest, Texture2D src)
    {
        ID3D12Resource* destTexture = _device->Resolve(dest);
        ID3D12Resource* srcTexture = _device->Resolve(src);
        _cl->CopyResource(destTexture, srcTexture);
    }

    void CommandListD3D12::CopyTexture(Texture3D dest, Texture3D src)
    {
        ID3D12Resource* destTexture = _device->Resolve(dest);
        ID3D12Resource* srcTexture = _device->Resolve(src);
        _cl->CopyResource(destTexture, srcTexture);
    }

    void CommandListD3D12::UploadTLASInstances(Buffer instanceBuffer, uint32_t offsetInInstanceBuffer, Span<const TLASInstanceDesc> instances)
    {
        MemoryScope SCOPE;

        ScopedVector<D3D12_RAYTRACING_INSTANCE_DESC> d3dInstances;
        d3dInstances.reserve(instances.size());

        for (const TLASInstanceDesc& instance : instances)
        {
            ID3D12Resource* blas = _device->Resolve(instance.blasBuffer);
            D3D12_RAYTRACING_INSTANCE_DESC instanceDesc = {
                .InstanceID = instance.instanceID,
                .InstanceMask = instance.instanceMask,
                .InstanceContributionToHitGroupIndex = instance.hitGroupModifier,
                .Flags = D3D12_RAYTRACING_INSTANCE_FLAG_TRIANGLE_FRONT_COUNTERCLOCKWISE,
                .AccelerationStructure = blas->GetGPUVirtualAddress() + instance.offsetInBlasBuffer,
            };

            for (uint32_t row = 0; row < 3; ++row)
            {
                for (uint32_t col = 0; col < 4; ++col)
                {
                    instanceDesc.Transform[row][col] = instance.transform[col][row];
                }
            }

            d3dInstances.push_back(instanceDesc);
        }

        UploadTypedBufferData<D3D12_RAYTRACING_INSTANCE_DESC>(instanceBuffer, offsetInInstanceBuffer, d3dInstances);
    }

    void CommandListD3D12::BuildBLAS(const BLASBuildDesc& desc)
    {
        MemoryScope SCOPE;

        ScopedVector<D3D12_RAYTRACING_GEOMETRY_DESC> srcD3DGeoDesc;
        srcD3DGeoDesc.reserve(desc.geometry.size());

        for (const BLASTriangleGeometryDesc& geoDesc : desc.geometry)
        {
            ID3D12Resource* indexBuffer = _device->Resolve(geoDesc.indexBuffer);
            ID3D12Resource* vertexBuffer = _device->Resolve(geoDesc.vertexBuffer);

            srcD3DGeoDesc.push_back({
                .Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES,
                .Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE,
                .Triangles = {
                    .IndexFormat = (geoDesc.indexFormat == IndexFormat::Uint16) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT,
                    .VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT,
                    .IndexCount = geoDesc.indexCount,
                    .VertexCount = geoDesc.vertexCount,
                    .IndexBuffer = indexBuffer->GetGPUVirtualAddress() + geoDesc.offsetInIndexBuffer,
                    .VertexBuffer = {
                        .StartAddress = vertexBuffer->GetGPUVirtualAddress() + geoDesc.offsetInVertexBuffer,
                        .StrideInBytes = geoDesc.vertexStride
                    }
                }
            });
        }

        ID3D12Resource* blasBuffer = _device->Resolve(desc.blasBuffer);
        ID3D12Resource* scratchBuffer = _device->Resolve(desc.scratchBuffer);
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = 
        {
            .DestAccelerationStructureData = blasBuffer->GetGPUVirtualAddress() + desc.offsetInBlasBuffer,
            .Inputs = {
                .Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL,
                .Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE,
                .NumDescs = UINT(srcD3DGeoDesc.size()),
                .DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY,
                .pGeometryDescs = srcD3DGeoDesc.data(),
            },
            .ScratchAccelerationStructureData = scratchBuffer->GetGPUVirtualAddress() + desc.offsetInScratchBuffer
        };

        _cl->BuildRaytracingAccelerationStructure(
            &buildDesc,
            0,
            nullptr);
    }

    void CommandListD3D12::BuildTLAS(const TLASBuildDesc& desc)
    {
        ID3D12Resource* tlasBuffer = _device->Resolve(desc.tlasBuffer);
        ID3D12Resource* scratchBuffer = _device->Resolve(desc.scratchBuffer);
        ID3D12Resource* instanceBuffer = _device->Resolve(desc.instanceBuffer);

        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = 
        {
            .DestAccelerationStructureData = tlasBuffer->GetGPUVirtualAddress() + desc.offetInTlasBuffer,
            .Inputs = {
                .Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL,
                .Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE,
                .NumDescs = desc.instanceCount,
                .DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY,
                .InstanceDescs = instanceBuffer->GetGPUVirtualAddress() + desc.offsetInInstanceBuffer,
            },
            .ScratchAccelerationStructureData = scratchBuffer->GetGPUVirtualAddress() + desc.offsetInScratchBuffer
        };

        _cl->BuildRaytracingAccelerationStructure(
            &buildDesc,
            0,
            nullptr);
    }

    void CommandListD3D12::BuildShaderRecordTable(const SRTBuildDesc& desc)
    {
        ID3D12StateObject* rtpso = _device->Resolve(desc.rtPipeline);
        ID3D12StateObjectProperties* rtpsoProperties = nullptr;
        RN_ASSERT(SUCCEEDED(rtpso->QueryInterface<ID3D12StateObjectProperties>(&rtpsoProperties)));

        // NOTE: This shader record struct needs to be updated if we make any changes to the RT local root signature
        struct ShaderRecord
        {
            char shaderIdentifier[D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES];
        };

        auto UploadRecord = [&](
            const ShaderRecordBuildDesc& buildDesc,
            uint32_t tableOffset,
            uint32_t recordStride,
            uint32_t recordIndex)
        {
            if (buildDesc.shaderName)
            {
                wchar_t wBuffer[256]{};
                size_t numConverted = 0;
                mbstowcs_s(&numConverted, wBuffer, buildDesc.shaderName, RN_ARRAY_SIZE(wBuffer) - 1);

                ShaderRecord record = {};
                void* identifier = rtpsoProperties->GetShaderIdentifier(wBuffer);
                std::memcpy(record.shaderIdentifier, identifier, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

                uint32_t recordOffset = desc.offsetInSRTBuffer + tableOffset + recordIndex * recordStride;
                UploadTypedBufferData<ShaderRecord>(desc.srtBuffer, recordOffset, { &record, 1 });
            }
        };

        UploadRecord(desc.rayGenDesc, desc.srtFootprint.rayGenRecordOffset, 0, 0);

        uint32_t missRecordIndex = 0;
        for (const ShaderRecordBuildDesc& missDesc : desc.missDescs)
        {
            UploadRecord(missDesc, desc.srtFootprint.missRecordOffset, desc.srtFootprint.missRecordStride, missRecordIndex++);
        }

        uint32_t hitGroupRecordIndex = 0;
        for (const ShaderRecordBuildDesc& hitGroupDesc : desc.hitGroupDescs)
        {
            UploadRecord(hitGroupDesc, desc.srtFootprint.hitGroupRecordOffset, desc.srtFootprint.hitGroupRecordStride, hitGroupRecordIndex++);
        }

        rtpsoProperties->Release();
    }

    namespace
    {
        struct ReadbackData
        {
            Span<const unsigned char> data;
            FnOnReadback onReadback;
            void* userData;
        };
    }

    void CommandListD3D12::QueueBufferReadback(Buffer buffer, uint32_t offsetInBuffer, uint32_t sizeInBytes, FnOnReadback onReadback, void* userData)
    {
        TemporaryResource readbackResource = _readbackAllocator->AllocateTemporaryResource(sizeInBytes, 256);
        CopyBufferRegion(
            readbackResource.buffer,
            readbackResource.offsetInBytes,
            buffer,
            offsetInBuffer,
            sizeInBytes);

        // Readbacks *should* be infrequent, so this won't hurt too bad, but it'd be nice to replace this with a bump allocator later
        ReadbackData* readbackData = TrackedNew<ReadbackData>(MemoryCategory::RHI);
        *readbackData = {
            .data = Span<const unsigned char>(static_cast<const unsigned char*>(readbackResource.cpuPtr), readbackResource.sizeInBytes),
            .onReadback = onReadback,
            .userData = userData
        };

        _device->QueueFrameFinalizerAction([](DeviceD3D12* device, void* userData)
            {
                ReadbackData* readbackData = reinterpret_cast<ReadbackData*>(userData);
                readbackData->onReadback(readbackData->data, readbackData->userData);

                TrackedDelete(readbackData);
            },
            readbackData);
    }

    namespace
    {
        void QueueTextureReadbackInternal(
            DeviceD3D12* device,
            ID3D12GraphicsCommandList7* cl,
            ID3D12Resource* srcResource,
            TemporaryResourceAllocator* readbackAllocator,
            FnOnReadback onReadback,
            void* userData)
        {
            D3D12_PLACED_SUBRESOURCE_FOOTPRINT textureFootprint = {};
        
            UINT numRows = 0;
            UINT64 rowSizeInBytes = 0;
            UINT64 readbackSize = 0;
            D3D12_RESOURCE_DESC srcTextureDesc = srcResource->GetDesc();

            device->D3DDevice()->GetCopyableFootprints(
                &srcTextureDesc,
                0,
                1,
                0,
                &textureFootprint,
                &numRows,
                &rowSizeInBytes,
                &readbackSize);

            TemporaryResource readbackResource = readbackAllocator->AllocateTemporaryResource(uint32_t(readbackSize), 4 * KILO);
            ID3D12Resource* destResource = device->Resolve(readbackResource.buffer);
            
            const D3D12_TEXTURE_COPY_LOCATION bufferDestLocation = 
            {
                .pResource = destResource,
                .Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT,
                .PlacedFootprint = 
                {
                    .Offset = readbackResource.offsetInBytes,
                    .Footprint = textureFootprint.Footprint
                }
            };

            const D3D12_TEXTURE_COPY_LOCATION textureSrcLocation =
            {
                .pResource = srcResource,
                .Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
                .SubresourceIndex = 0
            };

            cl->CopyTextureRegion(
                &bufferDestLocation,
                0, 0, 0,
                &textureSrcLocation,
                nullptr);

            ReadbackData* readbackData = TrackedNew<ReadbackData>(MemoryCategory::RHI);
            *readbackData = {
                .data = Span<const unsigned char>(static_cast<const unsigned char*>(readbackResource.cpuPtr), readbackResource.sizeInBytes),
                .onReadback = onReadback,
                .userData = userData
            };

            device->QueueFrameFinalizerAction([](DeviceD3D12* device, void* userData)
                {
                    ReadbackData* readbackData = reinterpret_cast<ReadbackData*>(userData);
                    readbackData->onReadback(readbackData->data, readbackData->userData);

                    TrackedDelete(readbackData);
                },
                readbackData);
        }
    }

    void CommandListD3D12::QueueTextureReadback(Texture2D texture, FnOnReadback onReadback, void* userData)
    {
        ID3D12Resource* srcResource = _device->Resolve(texture);
        QueueTextureReadbackInternal(_device, _cl, srcResource, _readbackAllocator, onReadback, userData);
    }

    void CommandListD3D12::QueueTextureReadback(Texture3D texture, FnOnReadback onReadback, void* userData)
    {
        ID3D12Resource* srcResource = _device->Resolve(texture);
        QueueTextureReadbackInternal(_device, _cl, srcResource, _readbackAllocator, onReadback, userData);
    }
}