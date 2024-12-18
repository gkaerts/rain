#include "rg/render_graph.hpp"
#include "render_graph_internal.hpp"

#include "rg/render_pass_context.hpp"

#include "common/memory/memory.hpp"
#include "common/memory/span.hpp"
#include "common/memory/bump_allocator.hpp"
#include "common/memory/hash_set.hpp"
#include "common/task/scheduler.hpp"

#include "common/log/log.hpp"

#include "rhi/device.hpp"
#include "rhi/command_list.hpp"
#include "rhi/transient_resource.hpp"
#include "rhi/temporary_resource.hpp"

#include "TaskScheduler.h"

namespace rn::rg
{
    RN_DEFINE_MEMORY_CATEGORY(RenderGraph);
    RN_DEFINE_LOG_CATEGORY(RenderGraph);

    namespace
    {
        struct PassBuildData
        {
            const char* name;
            RenderPassFlags flags;

            Span<TextureAttachment> colorAttachments;
            TextureAttachment depthAttachment;

            Span<Texture2DUsage> texture2Ds;
            Span<Texture3DUsage> texture3Ds;
            Span<BufferUsage> buffers;
            Span<rg::TLAS> tlases;
        };

        struct PassExecutionData
        {
            const char* name;
            RenderPassFlags flags;

            rhi::Viewport viewport;

            Span<rhi::BufferBarrier> bufferBarriers;
            Span<rhi::Texture2DBarrier> texture2DBarriers;
            Span<rhi::Texture3DBarrier> texture3DBarriers;

            Span<rhi::RenderPassRenderTarget> renderTargets;
            rhi::RenderPassDepthTarget depthTarget;

            FnExecuteRenderPass<void> onExecute;
            const void* passData;
        };
    }
    
    
    struct RenderGraphImpl
    {
        RenderGraphImpl(rhi::Device* device)
         : resourceAllocator(MemoryCategory::RenderGraph, device, 4096)
         , bufferAllocator(device, 1, rhi::GPUAllocationFlags::DeviceOnly, 
            rhi::BufferCreationFlags::AllowShaderReadOnly |
            rhi::BufferCreationFlags::AllowShaderReadWrite |
            rhi::BufferCreationFlags::AllowUniformBuffer,
            nullptr,
            nullptr)
        {}

        Texture2DPool texture2Ds = Texture2DPool(MemoryCategory::RenderGraph, 256);
        Texture3DPool texture3Ds = Texture3DPool(MemoryCategory::RenderGraph, 256);
        BufferPool buffers = BufferPool(MemoryCategory::RenderGraph, 256);

        Vector<rg::Texture2D> texture2DHandles = MakeVector<rg::Texture2D>(MemoryCategory::RenderGraph);
        Vector<rg::Texture3D> texture3DHandles = MakeVector<rg::Texture3D>(MemoryCategory::RenderGraph);
        Vector<rg::Buffer> bufferHandles = MakeVector<rg::Buffer>(MemoryCategory::RenderGraph);

        rhi::Viewport viewportStack[MAX_VIEWPORT_STACK_SIZE] = {};
        uint32_t viewportIdx = MAX_VIEWPORT_STACK_SIZE;

        PassBuildData passBuild[MAX_RENDER_PASS_COUNT] = {};
        PassExecutionData passExecution[MAX_RENDER_PASS_COUNT] = {};
        uint32_t renderPassCount = 0;

        BumpAllocator scratchAllocator = BumpAllocator(MemoryCategory::RenderGraph, 16 * MEGA);
        rhi::TransientResourceAllocator resourceAllocator;
        rhi::TemporaryResourceAllocator bufferAllocator;
    };


    RenderGraph::RenderGraph(rhi::Device* device)
        : _device(device)
        , _bumpAlloc(MemoryCategory::RenderGraph, 16 * MEGA)
    {
        _impl = TrackedNew<RenderGraphImpl>(MemoryCategory::RenderGraph, device);
    }

    RenderGraph::~RenderGraph()
    {
        if (_impl)
        {
            TrackedDelete(_impl);
        }
    }

    rg::Texture2D RenderGraph::AllocateTexture2D(const rg::Texture2DDesc& desc)
    {
        RN_ASSERT(!_isClosed);
        Texture2DBuildData buildData = {
            .desc = desc,
            .resourceFlags = desc.flags
        };

        rg::Texture2D handle = _impl->texture2Ds.Store(Texture2DRunData(), std::move(buildData));
        _impl->texture2DHandles.push_back(handle);

        return handle;
    }

    rg::Texture3D RenderGraph::AllocateTexture3D(const rg::Texture3DDesc& desc)
    {
        RN_ASSERT(!_isClosed);
        Texture3DBuildData buildData = {
            .desc = desc,
            .resourceFlags = desc.flags,
            .rwViewMipBitmask = 0
        };

        rg::Texture3D handle = _impl->texture3Ds.Store(Texture3DRunData(), std::move(buildData));
        _impl->texture3DHandles.push_back(handle);

        return handle;
    }

    rg::Buffer RenderGraph::AllocateBuffer(const rg::BufferDesc& desc)
    {
        RN_ASSERT(!_isClosed);
        BufferBuildData buildData = {
            .desc = desc,
            .resourceFlags = desc.flags
        };

        rg::Buffer handle =  _impl->buffers.Store(BufferRunData(), std::move(buildData));
        _impl->bufferHandles.push_back(handle);

        return handle;
    }

    Span<uint8_t> RenderGraph::AllocateScratchSpace(size_t size)
    {
        return { static_cast<uint8_t*>(_bumpAlloc.Allocate(size, 16)), size };
    }

    rg::Texture2D RenderGraph::RegisterTexture2D(const Texture2DRegistrationDesc& desc)
    {
        RN_ASSERT(!_isClosed);
        Texture2DBuildData buildData = {
            .desc = {
                .name = desc.name
            },
            .ownership = ResourceOwnership::External,
            .currSyncStage = desc.lastSyncStage,
            .currAccess = desc.lastAccess,
            .currLayout = desc.lastLayout
        };

        Texture2DRunData runData = {
            .rhiTexture = desc.texture,
            .view = desc.view,
            .rtv = desc.rtv,
            .dsv = desc.dsv,
            .clearValue = desc.clearValue
        };

        uint32_t rwViewCount = std::min(MAX_RW_VIEWS, uint32_t(desc.rwViews.size()));
        for (uint32_t i = 0; i < rwViewCount; ++i)
        {
            runData.rwView[i] = desc.rwViews[i];
        }

        rg::Texture2D handle = _impl->texture2Ds.Store(std::move(runData), std::move(buildData));
        _impl->texture2DHandles.push_back(handle);

        return handle;
    }

    rg::Texture3D RenderGraph::RegisterTexture3D(const Texture3DRegistrationDesc& desc)
    {
        RN_ASSERT(!_isClosed);
         Texture3DBuildData buildData = {
            .desc = {
                .name = desc.name
            },
            .ownership = ResourceOwnership::External,
            .currSyncStage = desc.lastSyncStage,
            .currAccess = desc.lastAccess,
            .currLayout = desc.lastLayout
        };

        Texture3DRunData runData = {
            .rhiTexture = desc.texture,
            .view = desc.view
        };

        uint32_t rwViewCount = std::min(MAX_RW_VIEWS, uint32_t(desc.rwViews.size()));
        for (uint32_t i = 0; i < rwViewCount; ++i)
        {
            runData.rwView[i] = desc.rwViews[i];
        }

        rg::Texture3D handle = _impl->texture3Ds.Store(std::move(runData), std::move(buildData));
        _impl->texture3DHandles.push_back(handle);

        return handle;
    }

    rg::Buffer RenderGraph::RegisterBuffer(const BufferRegistrationDesc& desc)
    {
        RN_ASSERT(!_isClosed);
        BufferBuildData buildData = {
            .desc = {
                .name = desc.name
            },
            .ownership = ResourceOwnership::External,
            .currSyncStage = desc.lastSyncStage,
            .currAccess = desc.lastAccess
        };

        BufferRunData runData = {
            .buffer = desc.buffer,
            .view = desc.view,
            .typedView = desc.typedView,
            .uniformView = desc.uniformView,
            .rwView = desc.rwView
        };

        rg::Buffer handle = _impl->buffers.Store(std::move(runData), std::move(buildData));
        _impl->bufferHandles.push_back(handle);

        return handle;
    }


    void RenderGraph::AddRenderPassInternal(const RenderPassDesc<void>& desc, const void* passData)
    {
        RN_ASSERT(!_isClosed);
        RN_ASSERT(_impl->renderPassCount < MAX_RENDER_PASS_COUNT);

        uint32_t passIdx = _impl->renderPassCount++;

        _impl->passExecution[passIdx] = {
            .name = desc.name,
            .flags = desc.flags,
            .viewport = CurrentViewport(),
            .onExecute = desc.onExecute,
            .passData = passData
        };
        
        _impl->passBuild[passIdx] = {
            .name = desc.name,
            .flags = desc.flags,
            .colorAttachments = { 
                _impl->scratchAllocator.AllocatePODArray<TextureAttachment>(desc.colorAttachments.size()), 
                desc.colorAttachments.size()
            },
            .depthAttachment = desc.depthAttachment,
            .texture2Ds = {
                _impl->scratchAllocator.AllocatePODArray<Texture2DUsage>(desc.texture2Ds.size()),
                desc.texture2Ds.size()
            },
            .texture3Ds = {
                _impl->scratchAllocator.AllocatePODArray<Texture3DUsage>(desc.texture3Ds.size()),
                desc.texture3Ds.size()
            },
            .buffers = {
                _impl->scratchAllocator.AllocatePODArray<BufferUsage>(desc.buffers.size()),
                desc.buffers.size()
            },
            .tlases = {
                _impl->scratchAllocator.AllocatePODArray<rg::TLAS>(desc.tlases.size()),
                desc.tlases.size()
            }
        };

        if (desc.colorAttachments.size() > 0)
        {
            std::copy(desc.colorAttachments.begin(), desc.colorAttachments.end(), _impl->passBuild[passIdx].colorAttachments.begin());
        }

        if (desc.texture2Ds.size() > 0)
        {
            std::copy(desc.texture2Ds.begin(), desc.texture2Ds.end(), _impl->passBuild[passIdx].texture2Ds.begin());
        }

        if (desc.texture3Ds.size() > 0)
        {
            std::copy(desc.texture3Ds.begin(), desc.texture3Ds.end(), _impl->passBuild[passIdx].texture3Ds.begin());
        }

        if (desc.buffers.size() > 0)
        {
            std::copy(desc.buffers.begin(), desc.buffers.end(), _impl->passBuild[passIdx].buffers.begin());
        }

        if (desc.tlases.size() > 0)
        {
            std::copy(desc.tlases.begin(), desc.tlases.end(), _impl->passBuild[passIdx].tlases.begin());
        }
    }

    namespace
    {
        // Build Step 1: Iterate over all resource usages in all passes and update the properties for pass creation
        //               We need to determine their initial dimensions and combined usage flags
        void BuildPassResourceProperties(RenderGraphImpl* impl)
        {
            for (uint32_t passIdx = 0; passIdx < impl->renderPassCount; ++passIdx)
            {
                PassBuildData& passBuild = impl->passBuild[passIdx];
                PassExecutionData& passExecution = impl->passExecution[passIdx];

                // Color attachments
                for (const TextureAttachment& attachment : passBuild.colorAttachments)
                {
                    Texture2DBuildData& texture = impl->texture2Ds.GetColdMutable(attachment.texture);
                    texture.creationFlags |= rhi::TextureCreationFlags::AllowRenderTarget;
                    texture.lastUsedPass = passIdx;
                    if (texture.firstUsedPass == INVALID_PASS_INDEX)
                    {
                        texture.firstUsedPass = passIdx;

                        if (texture.ownership == ResourceOwnership::Internal && 
                            texture.desc.sizeMode == TextureSizeMode::Adaptive)
                        {
                            texture.desc.width = std::max(1u, passExecution.viewport.width / texture.desc.width);
                            texture.desc.height = std::max(1u, passExecution.viewport.height / texture.desc.height);
                        }
                    }
                }

                // Depth attachment
                if (passBuild.depthAttachment.texture != rg::Texture2D::Invalid)
                {
                    Texture2DBuildData& texture = impl->texture2Ds.GetColdMutable(passBuild.depthAttachment.texture);
                    texture.creationFlags |= rhi::TextureCreationFlags::AllowDepthStencilTarget;
                    texture.lastUsedPass = passIdx;
                    if (texture.firstUsedPass == INVALID_PASS_INDEX)
                    {
                        texture.firstUsedPass = passIdx;
                    }
                }
                
                // 2D texture resources
                for (const Texture2DUsage& usage : passBuild.texture2Ds)
                {
                    Texture2DBuildData& texture = impl->texture2Ds.GetColdMutable(usage.texture);
                    texture.lastUsedPass = passIdx;
                    if (texture.firstUsedPass == INVALID_PASS_INDEX)
                    {
                        texture.firstUsedPass = passIdx;

                        if (texture.ownership == ResourceOwnership::Internal && 
                            texture.desc.sizeMode == TextureSizeMode::Adaptive)
                        {
                            texture.desc.width = std::max(1u, passExecution.viewport.width / texture.desc.width);
                            texture.desc.height = std::max(1u, passExecution.viewport.height / texture.desc.height);
                        }
                    }

                    switch (usage.access)
                    {
                    case TextureAccess::ShaderReadOnly:
                        // It doesn't make sense to read from a texture that can't have any content yet
                        RN_ASSERT(texture.ownership == ResourceOwnership::External || 
                                  texture.firstUsedPass != passIdx);
                        texture.creationFlags |= rhi::TextureCreationFlags::AllowShaderReadOnly;
                        break;
                    case TextureAccess::ShaderReadWrite:
                        texture.creationFlags |= rhi::TextureCreationFlags::AllowShaderReadWrite;
                        texture.rwViewMipBitmask |= 1 << usage.shaderReadWrite.mipIndex;
                        break;
                    case TextureAccess::CopySource:
                        // It doesn't make sense to read from a texture that cant't have any content yet
                        RN_ASSERT(texture.ownership == ResourceOwnership::External || 
                                  texture.firstUsedPass != passIdx);    
                        break;
                    case TextureAccess::CopyDest:
                    case TextureAccess::Presentation:
                        break;
                    
                    default:
                        RN_ASSERT(false);
                        break;
                    }
                }

                // 3D texture resources
                for (const Texture3DUsage& usage : passBuild.texture3Ds)
                {
                    Texture3DBuildData& texture = impl->texture3Ds.GetColdMutable(usage.texture);
                    texture.lastUsedPass = passIdx;
                    if (texture.firstUsedPass == INVALID_PASS_INDEX)
                    {
                        texture.firstUsedPass = passIdx;
                    }

                    switch (usage.access)
                    {
                    case TextureAccess::ShaderReadOnly:
                        // It doesn't make sense to read from a texture that can't have any content yet
                        RN_ASSERT(texture.ownership == ResourceOwnership::External || 
                                  texture.firstUsedPass != passIdx);
                        texture.creationFlags |= rhi::TextureCreationFlags::AllowShaderReadOnly;
                        break;
                    case TextureAccess::ShaderReadWrite:
                        texture.creationFlags |= rhi::TextureCreationFlags::AllowShaderReadWrite;
                        texture.rwViewMipBitmask |= 1 << usage.shaderReadWrite.mipIndex;
                        break;
                    case TextureAccess::CopySource:
                        // It doesn't make sense to read from a texture that cant't have any content yet
                        RN_ASSERT(texture.ownership == ResourceOwnership::External || 
                                  texture.firstUsedPass != passIdx);    
                        break;
                    case TextureAccess::CopyDest:
                        break;
                    
                    default:
                        RN_ASSERT(false);
                        break;
                    }
                }

                for (const BufferUsage& usage : passBuild.buffers)
                {
                    BufferBuildData& buffer = impl->buffers.GetColdMutable(usage.buffer);
                    buffer.lastUsedPass = passIdx;
                    if (buffer.firstUsedPass == INVALID_PASS_INDEX)
                    {
                        buffer.firstUsedPass = passIdx;
                    }

                    switch (usage.access)
                    {
                    case BufferAccess::ShaderReadOnly:
                        // It doesn't make sense to read from a buffer that can't have any content yet
                        RN_ASSERT(buffer.ownership == ResourceOwnership::External || 
                                  buffer.firstUsedPass != passIdx);
                        buffer.creationFlags |= rhi::BufferCreationFlags::AllowShaderReadOnly;
                        break;
                    case BufferAccess::ShaderReadWrite:
                        buffer.creationFlags |= rhi::BufferCreationFlags::AllowShaderReadWrite;
                        break;
                    case BufferAccess::UniformBuffer:
                        // It doesn't make sense to read from a buffer that can't have any content yet
                        RN_ASSERT(buffer.ownership == ResourceOwnership::External || 
                                  buffer.firstUsedPass != passIdx);
                        buffer.creationFlags |= rhi::BufferCreationFlags::AllowUniformBuffer;
                        break;
                    case BufferAccess::CopySource:
                    case BufferAccess::DrawIDBuffer:
                    case BufferAccess::IndexBuffer:
                    case BufferAccess::ArgumentBuffer:
                        // It doesn't make sense to read from a buffer that can't have any content yet
                        RN_ASSERT(buffer.ownership == ResourceOwnership::External || 
                                  buffer.firstUsedPass != passIdx);
                        break;
                    case BufferAccess::CopyDest:
                        break;

                    default:
                        RN_ASSERT(false);
                        break;
                    }
                }
            }
        }


        // Build Step 2: Iterate over all resource usages in all passes and allocate the actual resources on a transient resource allocator
        //               We insert inter-pass barriers at this point as well
        void BuildTexture2D(rhi::Device* device, rhi::TransientResourceAllocator& resourceAllocator, Texture2DBuildData& buildData, Texture2DRunData& runData)
        {
            if (buildData.ownership == ResourceOwnership::External)
            {
                return;
            }

            bool resourceIsPinned = TestFlag(buildData.resourceFlags, ResourceFlags::Pinned);
            RN_ASSERT(runData.rhiTexture == rhi::Texture2D::Invalid ||
                resourceIsPinned); 

            if (resourceIsPinned && runData.rhiTexture != rhi::Texture2D::Invalid)
            {
                return;
            }

            rhi::Texture2DDesc desc = {
                .flags = buildData.creationFlags,
                .width = buildData.desc.width,
                .height = buildData.desc.height,
                .arraySize = 1,
                .mipLevels = buildData.desc.mipLevels,
                .format = buildData.desc.format,
                .optClearValue = &buildData.desc.clearValue,
                .name = buildData.desc.name
            };

            rhi::ResourceFootprint fp = device->CalculateTextureFootprint(desc);
            rhi::GPUMemoryRegion region = {}; 
            
            if (!resourceIsPinned)
            {
                region = resourceAllocator.AllocateMemoryRegion(uint32_t(fp.sizeInBytes));
            }
            else
            {
                runData.pinnedAllocation = device->GPUAlloc(MemoryCategory::RenderGraph, fp.sizeInBytes, rhi::GPUAllocationFlags::DeviceOnly);
                region = { 
                    .allocation = runData.pinnedAllocation,
                    .offsetInAllocation = 0,
                    .regionSize = fp.sizeInBytes
                };
            }

            runData.rhiTexture = device->CreateTexture2D(desc, region);
            buildData.gpuRegion = region;
        }

        void BuildTexture3D(rhi::Device* device, rhi::TransientResourceAllocator& resourceAllocator, Texture3DBuildData& buildData, Texture3DRunData& runData)
        {
            if (buildData.ownership == ResourceOwnership::External)
            {
                return;
            }

            bool resourceIsPinned = TestFlag(buildData.resourceFlags, ResourceFlags::Pinned);
            RN_ASSERT(runData.rhiTexture == rhi::Texture3D::Invalid || resourceIsPinned); 

            if (resourceIsPinned && runData.rhiTexture != rhi::Texture3D::Invalid)
            {
                return;
            }

            rhi::Texture3DDesc desc = {
                .flags = buildData.creationFlags,
                .width = buildData.desc.width,
                .height = buildData.desc.height,
                .depth = buildData.desc.depth,
                .mipLevels = buildData.desc.mipLevels,
                .format = buildData.desc.format,
                .optClearValue = nullptr,
                .name = buildData.desc.name
            };

            rhi::ResourceFootprint fp = device->CalculateTextureFootprint(desc);
            rhi::GPUMemoryRegion region = {}; 
            
            if (!resourceIsPinned)
            {
                region = resourceAllocator.AllocateMemoryRegion(uint32_t(fp.sizeInBytes));
            }
            else
            {
                runData.pinnedAllocation = device->GPUAlloc(MemoryCategory::RenderGraph, fp.sizeInBytes, rhi::GPUAllocationFlags::DeviceOnly);
                region = { 
                    .allocation = runData.pinnedAllocation,
                    .offsetInAllocation = 0,
                    .regionSize = fp.sizeInBytes
                };
            }

            runData.rhiTexture = device->CreateTexture3D(desc, region);
            buildData.gpuRegion = region;
        }

        void BuildBuffer(rhi::Device* device, rhi::TemporaryResourceAllocator& resourceAllocator, BufferBuildData& buildData, BufferRunData& runData)
        {
            if (buildData.ownership == ResourceOwnership::External)
            {
                return;
            }

            bool resourceIsPinned = TestFlag(buildData.resourceFlags, ResourceFlags::Pinned);
            RN_ASSERT(runData.buffer.buffer == rhi::Buffer::Invalid || resourceIsPinned); 

            if (resourceIsPinned && runData.buffer.buffer != rhi::Buffer::Invalid)
            {
                return;
            }

            rhi::TemporaryResource tmpBuffer = resourceAllocator.AllocateTemporaryResource(buildData.desc.sizeInBytes, 256);
            runData.buffer = {
                .buffer = tmpBuffer.buffer,
                .offset = tmpBuffer.offsetInBytes,
                .size = buildData.desc.sizeInBytes
            };
        }

        void RenderTargetSyncProperties(
            rhi::PipelineSyncStage& stage,
            rhi::PipelineAccess& access,
            rhi::TextureLayout& layout)
        {
            stage |= rhi::PipelineSyncStage::RenderTargetOutput;
            access |= rhi::PipelineAccess::RenderTargetWrite;
            layout = rhi::TextureLayout::RenderTarget;
        }

        void DepthTargetSyncProperties(
            RenderPassFlags passFlags,
            rhi::PipelineSyncStage& stage,
            rhi::PipelineAccess& access,
            rhi::TextureLayout& layout)
        {
            if (TestFlag(passFlags, RenderPassFlags::AllDrawUseEarlyZ))
            {
                stage |= rhi::PipelineSyncStage::EarlyDepthTest;
            }
            else
            {
                stage |= rhi::PipelineSyncStage::LateDepthTest;
            }

            if (TestFlag(passFlags, RenderPassFlags::AllDrawUseEarlyZ))
            {
                RN_ASSERT(layout == rhi::TextureLayout::Undefined ||
                    layout == rhi::TextureLayout::DepthTargetRead ||
                    layout == rhi::TextureLayout::ShaderRead);

                access |= rhi::PipelineAccess::DepthTargetRead;
                layout = rhi::TextureLayout::DepthTargetRead;
            }
            else
            {
                access |= rhi::PipelineAccess::DepthTargetReadWrite;
                layout = rhi::TextureLayout::DepthTargetReadWrite;
            }
        }

        void TextureSyncProperties(
            TextureAccess accessOp,
            RenderPassFlags passFlags,
            rhi::PipelineSyncStage& stage,
            rhi::PipelineAccess& access,
            rhi::TextureLayout& layout)
        {
            switch (accessOp)
            {
            case TextureAccess::ShaderReadOnly:
            {
                RN_ASSERT(layout == rhi::TextureLayout::Undefined || 
                    layout == rhi::TextureLayout::DepthTargetRead ||
                    layout == rhi::TextureLayout::ShaderRead);

                // TODO: This is terrible. We need a way to provide better granularity here.
                stage |= rhi::PipelineSyncStage::VertexShader | rhi::PipelineSyncStage::PixelShader | rhi::PipelineSyncStage::ComputeShader | rhi::PipelineSyncStage::RayTracing;
                if (TestFlag(passFlags, RenderPassFlags::ComputeOnly))
                {
                    stage = rhi::PipelineSyncStage::ComputeShader;
                }

                access |= rhi::PipelineAccess::ShaderRead;

                if (layout != rhi::TextureLayout::DepthTargetRead)
                {
                    layout = rhi::TextureLayout::ShaderRead;
                }
            }
                break;
            case TextureAccess::ShaderReadWrite:
            {
                RN_ASSERT(layout == rhi::TextureLayout::Undefined ||
                    layout == rhi::TextureLayout::ShaderReadWrite);

                // TODO: Same as above. This will provide horrible overlap on the GPU. Need to specify some hints.
                stage |= rhi::PipelineSyncStage::VertexShader | rhi::PipelineSyncStage::PixelShader | rhi::PipelineSyncStage::ComputeShader | rhi::PipelineSyncStage::RayTracing;
                if (TestFlag(passFlags, RenderPassFlags::ComputeOnly))
                {
                    stage = rhi::PipelineSyncStage::ComputeShader;
                }

                access |= rhi::PipelineAccess::ShaderReadWrite;
                layout = rhi::TextureLayout::ShaderReadWrite;
            }
                break;

            case TextureAccess::CopySource:
            {
                RN_ASSERT(layout == rhi::TextureLayout::Undefined ||
                    layout == rhi::TextureLayout::CopyRead);

                stage |= rhi::PipelineSyncStage::Copy;
                access |= rhi::PipelineAccess::CopyRead;
                layout = rhi::TextureLayout::CopyRead;
            }
                break;
            case TextureAccess::CopyDest:
            {
                RN_ASSERT(layout == rhi::TextureLayout::Undefined ||
                    layout == rhi::TextureLayout::CopyWrite);

                stage |= rhi::PipelineSyncStage::Copy;
                access |= rhi::PipelineAccess::CopyWrite;
                layout = rhi::TextureLayout::CopyWrite;
            }
                break;
            case TextureAccess::Presentation:
            {
                // Can't combine presentation with any other operations
                RN_ASSERT(stage == rhi::PipelineSyncStage::None);
                RN_ASSERT(access == rhi::PipelineAccess::None);

                RN_ASSERT(layout == rhi::TextureLayout::Undefined ||
                    layout == rhi::TextureLayout::Present);

                layout = rhi::TextureLayout::Present;
            }
                break;
            default:
                break;
            }
        }
    
        void BufferSyncProperties(
            BufferAccess accessOp,
            RenderPassFlags passFlags,
            rhi::PipelineSyncStage& stage,
            rhi::PipelineAccess& access)
        {
            switch (accessOp)
            {
            case BufferAccess::ShaderReadOnly:
            {
                // TODO: Same as the texture case. Terrible overlap.
                stage |= rhi::PipelineSyncStage::VertexShader | rhi::PipelineSyncStage::PixelShader | rhi::PipelineSyncStage::ComputeShader;
                if (TestFlag(passFlags, RenderPassFlags::ComputeOnly))
                {
                    stage = rhi::PipelineSyncStage::ComputeShader;
                }

                access |= rhi::PipelineAccess::ShaderRead;
            }
                break;
            case BufferAccess::ShaderReadWrite:
            {
                // TODO: Same as the texture case. Terrible overlap.
                stage |= rhi::PipelineSyncStage::VertexShader | rhi::PipelineSyncStage::PixelShader | rhi::PipelineSyncStage::ComputeShader;
                if (TestFlag(passFlags, RenderPassFlags::ComputeOnly))
                {
                    stage |= rhi::PipelineSyncStage::ComputeShader;
                }

                access |= rhi::PipelineAccess::ShaderReadWrite;
            }
                break;

            case BufferAccess::CopySource:
            {
                stage |= rhi::PipelineSyncStage::Copy;
                access |= rhi::PipelineAccess::CopyRead;
            }
                break;
            case BufferAccess::CopyDest:
            {
                stage |= rhi::PipelineSyncStage::Copy;
                access |= rhi::PipelineAccess::CopyWrite;
            }
                break;
            case BufferAccess::DrawIDBuffer:
            {
                stage |= rhi::PipelineSyncStage::InputAssembly;
                access |= rhi::PipelineAccess::VertexInput;
            }
                break;
            case BufferAccess::IndexBuffer:
            {
                stage |= rhi::PipelineSyncStage::InputAssembly;
                access |= rhi::PipelineAccess::IndexInput;
            }
                break;
            case BufferAccess::ArgumentBuffer:
            {
                stage |= rhi::PipelineSyncStage::IndirectCommand;
                access |= rhi::PipelineAccess::CommandInput;
            }
                break;
            case BufferAccess::UniformBuffer:
            {
                stage |= rhi::PipelineSyncStage::VertexShader | rhi::PipelineSyncStage::PixelShader | rhi::PipelineSyncStage::ComputeShader;
                if (TestFlag(passFlags, RenderPassFlags::ComputeOnly))
                {
                    stage |= rhi::PipelineSyncStage::ComputeShader;
                }

                access |= rhi::PipelineAccess::UniformBuffer;
            }
            default:
                break;
            }
        }
        
        void AllocatePassResources(rhi::Device* device, RenderGraphImpl* impl)
        {
            auto UpdateTextureBarrierData = [](uint32_t passIdx, auto& buildData, bool requiresReadWriteBarrier = false)
            {
                if (buildData.barrierLastUpdated != passIdx)
                {
                    buildData.prevSyncStage = buildData.currSyncStage;
                    buildData.prevAccess = buildData.currAccess;
                    buildData.prevLayout = buildData.currLayout;

                    buildData.currSyncStage = rhi::PipelineSyncStage::None;
                    buildData.currAccess = rhi::PipelineAccess::None;
                    buildData.currLayout = rhi::TextureLayout::Undefined;

                    buildData.barrierLastUpdated = passIdx;
                }
                buildData.requiresReadWriteBarrier = requiresReadWriteBarrier;
            };

            auto UpdateBufferBarrierData = [](uint32_t passIdx, auto& buildData, bool requiresReadWriteBarrier = false)
            {
                if (buildData.barrierLastUpdated != passIdx)
                {
                    buildData.prevSyncStage = buildData.currSyncStage;
                    buildData.prevAccess = buildData.currAccess;

                    buildData.currSyncStage = rhi::PipelineSyncStage::None;
                    buildData.currAccess = rhi::PipelineAccess::None;

                    buildData.barrierLastUpdated = passIdx;
                }
                buildData.requiresReadWriteBarrier = requiresReadWriteBarrier;
            };

            for (uint32_t passIdx = 0; passIdx < impl->renderPassCount; ++passIdx)
            {
                MemoryScope SCOPE;

                PassBuildData& passBuild = impl->passBuild[passIdx];
                PassExecutionData& passExecution = impl->passExecution[passIdx];

                ScopedHashSet<rg::Texture2D> used2DTextures;
                used2DTextures.reserve(passBuild.colorAttachments.size() + 
                    passBuild.texture2Ds.size() + 1);

                ScopedHashSet<rg::Texture3D> used3DTextures;
                used3DTextures.reserve(passBuild.texture3Ds.size());

                ScopedHashSet<rg::Buffer> usedBuffers;
                usedBuffers.reserve(passBuild.buffers.size());

                // Resource and view creation
                // Color attachments

                passExecution.renderTargets = {
                    impl->scratchAllocator.AllocatePODArray<rhi::RenderPassRenderTarget>(
                    passBuild.colorAttachments.size()),
                    passBuild.colorAttachments.size() 
                };

                uint32_t renderTargetSlot = 0;
                for (const TextureAttachment& attachment : passBuild.colorAttachments)
                {
                    Texture2DBuildData& buildData = impl->texture2Ds.GetColdMutable(attachment.texture);
                    Texture2DRunData& runData = impl->texture2Ds.GetHotMutable(attachment.texture);
                    if (buildData.firstUsedPass == passIdx)
                    {
                        BuildTexture2D(device, impl->resourceAllocator, buildData, runData);
                    }

                    if (runData.rtv == rhi::RenderTargetView::Invalid)
                    {
                         runData.rtv = device->CreateRenderTargetView({
                            .texture = runData.rhiTexture,
                            .format = buildData.desc.renderFormat,
                        });
                    }

                    UpdateTextureBarrierData(passIdx, buildData);
                    RenderTargetSyncProperties(buildData.currSyncStage, buildData.currAccess, buildData.currLayout);
                    used2DTextures.insert(attachment.texture);

                    passExecution.renderTargets[renderTargetSlot++] = {
                        .view = runData.rtv,
                        .loadOp = attachment.loadOp,
                        .clearValue = runData.clearValue
                    };
                }

                // Depth attachment
                if (passBuild.depthAttachment.texture != rg::Texture2D::Invalid)
                {
                    Texture2DBuildData& buildData = impl->texture2Ds.GetColdMutable(passBuild.depthAttachment.texture);
                    Texture2DRunData& runData = impl->texture2Ds.GetHotMutable(passBuild.depthAttachment.texture);
                    if (buildData.firstUsedPass == passIdx)
                    {
                        BuildTexture2D(device, impl->resourceAllocator, buildData, runData);
                    }

                    if (runData.dsv == rhi::DepthStencilView::Invalid)
                    {
                        runData.dsv = device->CreateDepthStencilView({
                            .texture = runData.rhiTexture,
                            .format = buildData.desc.depthFormat
                        });
                    }

                    UpdateTextureBarrierData(passIdx, buildData);
                    DepthTargetSyncProperties(passBuild.flags, buildData.currSyncStage, buildData.currAccess, buildData.currLayout);
                    used2DTextures.insert(passBuild.depthAttachment.texture);

                    passExecution.depthTarget = {
                        .view = runData.dsv,
                        .loadOp = passBuild.depthAttachment.loadOp,
                        .clearValue = runData.clearValue
                    };
                }

                // 2D texture resources
                for (const Texture2DUsage& usage : passBuild.texture2Ds)
                {
                    Texture2DBuildData& buildData = impl->texture2Ds.GetColdMutable(usage.texture);
                    Texture2DRunData& runData = impl->texture2Ds.GetHotMutable(usage.texture);
                    if (buildData.firstUsedPass == passIdx)
                    {
                        BuildTexture2D(device, impl->resourceAllocator, buildData, runData);
                    }

                    bool requiresReadWriteBarrier = false;
                    switch(usage.access)
                    {
                        case TextureAccess::ShaderReadOnly:
                        {
                            if (runData.view == rhi::Texture2DView::Invalid)
                            {
                                runData.view = device->CreateTexture2DView({
                                    .texture = runData.rhiTexture,
                                    .format = buildData.desc.format,
                                    .mipCount = buildData.desc.mipLevels,
                                });
                            }
                            runData.viewAccessMask.set(passIdx, true);
                        }
                        break;
                        case TextureAccess::ShaderReadWrite:
                        {
                            for (uint32_t mip = 0; mip < MAX_RW_VIEWS; ++mip)
                            {
                                if ((buildData.rwViewMipBitmask & (1 << mip)))
                                {
                                    if (runData.rwView[mip] == rhi::RWTexture2DView::Invalid)
                                    {
                                        runData.rwView[mip] = device->CreateRWTexture2DView({
                                            .texture = runData.rhiTexture,
                                            .format = buildData.desc.format,
                                            .mipIndex = mip
                                        });
                                    }
                                }
                            }
                            runData.rwViewAccessMask.set(passIdx, true);
                            requiresReadWriteBarrier = TestFlag(usage.shaderReadWrite.flags, ResourceReadWriteFlags::SyncBeforeReadWriteAccess);
                        }
                        break;
                        case TextureAccess::CopySource:
                        case TextureAccess::CopyDest:
                            runData.resourceAccessMask.set(passIdx, true);
                        break;
                        case TextureAccess::Presentation:
                        break;
                        case TextureAccess::Undefined:
                        default:
                            RN_ASSERT(false);
                        break;
                    }

                    UpdateTextureBarrierData(passIdx, buildData, requiresReadWriteBarrier);
                    TextureSyncProperties(usage.access, 
                        passBuild.flags,
                        buildData.currSyncStage,
                        buildData.currAccess,
                        buildData.currLayout);

                    used2DTextures.insert(usage.texture);
                }

                // 3D texture resources
                for (const Texture3DUsage& usage : passBuild.texture3Ds)
                {
                    Texture3DBuildData& buildData = impl->texture3Ds.GetColdMutable(usage.texture);
                    Texture3DRunData& runData = impl->texture3Ds.GetHotMutable(usage.texture);
                    if (buildData.firstUsedPass == passIdx)
                    {
                        BuildTexture3D(device, impl->resourceAllocator, buildData, runData);
                    }

                    bool requiresReadWriteBarrier = false;
                    switch(usage.access)
                    {
                        case TextureAccess::ShaderReadOnly:
                        {
                            if (runData.view == rhi::Texture3DView::Invalid)
                            {
                                runData.view = device->CreateTexture3DView({
                                    .texture = runData.rhiTexture,
                                    .format = buildData.desc.format,
                                    .mipCount = buildData.desc.mipLevels,
                                });
                            }
                            runData.viewAccessMask.set(passIdx, true);
                        }
                        break;
                        case TextureAccess::ShaderReadWrite:
                        {
                            for (uint32_t mip = 0; mip < MAX_RW_VIEWS; ++mip)
                            {
                                if ((buildData.rwViewMipBitmask & (1 << mip)))
                                {
                                    if (runData.rwView[mip] == rhi::RWTexture3DView::Invalid)
                                    {
                                        runData.rwView[mip] = device->CreateRWTexture3DView({
                                            .texture = runData.rhiTexture,
                                            .format = buildData.desc.format,
                                            .mipIndex = mip
                                        });
                                    }
                                }
                            }
                            runData.rwViewAccessMask.set(passIdx, true);
                            requiresReadWriteBarrier = TestFlag(usage.shaderReadWrite.flags, ResourceReadWriteFlags::SyncBeforeReadWriteAccess);
                        }
                        break;
                        case TextureAccess::CopySource:
                        case TextureAccess::CopyDest:
                            runData.resourceAccessMask.set(passIdx, true);
                        break;
                        case TextureAccess::Undefined:
                        default:
                            RN_ASSERT(false);
                        break;
                    }

                    UpdateTextureBarrierData(passIdx, buildData, requiresReadWriteBarrier);
                    TextureSyncProperties(usage.access, 
                        passBuild.flags,
                        buildData.currSyncStage,
                        buildData.currAccess,
                        buildData.currLayout);
                    used3DTextures.insert(usage.texture);
                }

                // Buffer resources
                for (const BufferUsage& usage : passBuild.buffers)
                {
                    BufferBuildData& buildData = impl->buffers.GetColdMutable(usage.buffer);
                    BufferRunData& runData = impl->buffers.GetHotMutable(usage.buffer);
                    if (buildData.firstUsedPass == passIdx)
                    {
                        BuildBuffer(device, impl->bufferAllocator, buildData, runData);
                    }

                    bool requiresReadWriteBarrier = false;
                    switch(usage.access)
                    {
                        case BufferAccess::ShaderReadOnly:
                        {
                            if (usage.structureSizeInBytes > 0)
                            {
                                if (runData.typedView == rhi::TypedBufferView::Invalid)
                                {
                                    runData.typedView = device->CreateTypedBufferView({
                                        .buffer = runData.buffer.buffer,
                                        .offsetInBytes = runData.buffer.offset,
                                        .elementSizeInBytes = usage.structureSizeInBytes,
                                        .elementCount = buildData.desc.sizeInBytes / usage.structureSizeInBytes,
                                    });
                                }
                                runData.typedViewAccessMask.set(passIdx, true);
                            }
                            else
                            {
                                if (runData.view == rhi::BufferView::Invalid)
                                {
                                    runData.view = device->CreateBufferView({
                                        .buffer = runData.buffer.buffer,
                                        .offsetInBytes = runData.buffer.offset,
                                        .sizeInBytes = buildData.desc.sizeInBytes
                                    });
                                }
                                runData.viewAccessMask.set(passIdx, true);
                            }
                        }
                        break;
                        case BufferAccess::ShaderReadWrite:
                        {
                            if (runData.rwView == rhi::RWBufferView::Invalid)
                            {
                                runData.rwView = device->CreateRWBufferView({
                                    .buffer = runData.buffer.buffer,
                                    .offsetInBytes = runData.buffer.offset,
                                    .sizeInBytes = buildData.desc.sizeInBytes
                                });
                            }
                            runData.rwViewAccessMask.set(passIdx, true);
                            requiresReadWriteBarrier = TestFlag(usage.shaderReadWrite.flags, ResourceReadWriteFlags::SyncBeforeReadWriteAccess);
                        }
                        break;
                        case BufferAccess::CopySource:
                        case BufferAccess::CopyDest:
                        case BufferAccess::DrawIDBuffer:
                        case BufferAccess::IndexBuffer:
                            runData.resourceAccessMask.set(passIdx, true);
                        break;
                        case BufferAccess::UniformBuffer:
                            if (runData.uniformView == rhi::UniformBufferView::Invalid)
                            {
                                runData.uniformView = device->CreateUniformBufferView({
                                    .buffer = runData.buffer.buffer,
                                    .offsetInBytes = runData.buffer.offset,
                                    .sizeInBytes = buildData.desc.sizeInBytes
                                });
                            }
                            runData.uniformViewAccessMask.set(passIdx, true);
                        break;
                    
                        case BufferAccess::Undefined:
                        default:
                            RN_ASSERT(false);
                        break;
                    }

                    UpdateBufferBarrierData(passIdx, buildData, requiresReadWriteBarrier);
                    BufferSyncProperties(usage.access, 
                        passBuild.flags,
                        buildData.currSyncStage,
                        buildData.currAccess);
                    usedBuffers.insert(usage.buffer);
                }
            
                // Resource de-allocation on final pass
                auto FreeMemoryRegionIfNeeded = [&impl, passIdx](auto& buildData)
                {
                    if (buildData.lastUsedPass == passIdx &&
                        buildData.gpuRegion.allocation != rhi::GPUAllocation::Invalid)
                    {
                        impl->resourceAllocator.FreeMemoryRegion(buildData.gpuRegion);
                        buildData.gpuRegion = {};
                    }
                };

                Span<rhi::Texture2DBarrier> texture2DBarriers = { 
                    impl->scratchAllocator.AllocatePODArray<rhi::Texture2DBarrier>(
                        used2DTextures.size()),
                    used2DTextures.size()
                };

                uint32_t tex2DBarrierCount = 0;
                for (rg::Texture2D texture : used2DTextures)
                {
                    Texture2DBuildData& buildData = impl->texture2Ds.GetColdMutable(texture);
                    const Texture2DRunData& runData = impl->texture2Ds.GetHotMutable(texture);
                    FreeMemoryRegionIfNeeded(buildData);

                    if (buildData.prevSyncStage != buildData.currSyncStage ||
                        buildData.prevAccess != buildData.currAccess ||
                        buildData.prevLayout != buildData.currLayout ||
                        buildData.requiresReadWriteBarrier)
                    {
                        texture2DBarriers[tex2DBarrierCount++] = {
                            .fromStage = buildData.prevSyncStage,
                            .toStage = buildData.currSyncStage,
                            .fromAccess = buildData.prevAccess,
                            .toAccess = buildData.currAccess,
                            .fromLayout = buildData.prevLayout,
                            .toLayout = buildData.currLayout,
                            .handle = runData.rhiTexture,
                            .firstMipLevel = 0,
                            .numMips = buildData.desc.mipLevels,
                            .firstArraySlice = 0,
                            .numArraySlices = 1
                        };
                    }
                }
                passExecution.texture2DBarriers = { texture2DBarriers.data(), tex2DBarrierCount };

                Span<rhi::Texture3DBarrier> texture3DBarriers = { 
                    impl->scratchAllocator.AllocatePODArray<rhi::Texture3DBarrier>(
                        used3DTextures.size()),
                    used3DTextures.size()
                };

                uint32_t tex3DBarrierCount = 0;
                for (rg::Texture3D texture : used3DTextures)
                {
                    Texture3DBuildData& buildData = impl->texture3Ds.GetColdMutable(texture);
                    const Texture3DRunData& runData = impl->texture3Ds.GetHotMutable(texture);
                    FreeMemoryRegionIfNeeded(buildData);

                    if (buildData.prevSyncStage != buildData.currSyncStage ||
                        buildData.prevAccess != buildData.currAccess ||
                        buildData.prevLayout != buildData.currLayout ||
                        buildData.requiresReadWriteBarrier)
                    {
                        texture3DBarriers[tex3DBarrierCount++] = {
                            .fromStage = buildData.prevSyncStage,
                            .toStage = buildData.currSyncStage,
                            .fromAccess = buildData.prevAccess,
                            .toAccess = buildData.currAccess,
                            .fromLayout = buildData.prevLayout,
                            .toLayout = buildData.currLayout,
                            .handle = runData.rhiTexture,
                            .firstMipLevel = 0,
                            .numMips = buildData.desc.mipLevels
                        };
                    }
                }
                passExecution.texture3DBarriers = { texture3DBarriers.data(), tex3DBarrierCount };


                Span<rhi::BufferBarrier> bufferBarriers = { 
                    impl->scratchAllocator.AllocatePODArray<rhi::BufferBarrier>(
                        usedBuffers.size()),
                    usedBuffers.size()
                };

                uint32_t bufferBarrierCount = 0;
                for (rg::Buffer buffer : usedBuffers)
                {
                    BufferBuildData& buildData = impl->buffers.GetColdMutable(buffer);
                    const BufferRunData& runData = impl->buffers.GetHotMutable(buffer);
                    FreeMemoryRegionIfNeeded(buildData);

                    // Buffers are simultaneous access. Only barrier on write hazard
                    if (buildData.requiresReadWriteBarrier)
                    {
                        bufferBarriers[bufferBarrierCount++] = {
                            .fromStage = buildData.prevSyncStage,
                            .toStage = buildData.currSyncStage,
                            .fromAccess = buildData.prevAccess,
                            .toAccess = buildData.currAccess,
                            .handle = runData.buffer.buffer,
                            .offset = runData.buffer.offset,
                            .size = buildData.desc.sizeInBytes
                        };
                    }
                }

                passExecution.bufferBarriers = { bufferBarriers.data(), bufferBarrierCount };
            }
        }
    }


    void RenderGraph::Build()
    {
        RN_ASSERT(!_isClosed);
        BuildPassResourceProperties(_impl);
        AllocatePassResources(_device, _impl);

        _isClosed = true;
    }

    namespace
    {
        void DestroyTexture2DIfNeeded(rhi::Device* device, Texture2DRunData& runData, const Texture2DBuildData& buildData)
        {
            if (buildData.ownership == ResourceOwnership::External)
            {
                return;
            }

            if (runData.rhiTexture != rhi::Texture2D::Invalid &&
                runData.pinnedAllocation == rhi::GPUAllocation::Invalid)
            {
                device->Destroy(runData.rhiTexture);
                runData.rhiTexture = rhi::Texture2D::Invalid;
            }

            if (runData.view != rhi::Texture2DView::Invalid)
            {
                device->Destroy(runData.view);
                runData.view = rhi::Texture2DView::Invalid;
            }

            for (uint32_t i = 0; i < MAX_RW_VIEWS; ++i)
            {
                if (runData.rwView[i] != rhi::RWTexture2DView::Invalid)
                {
                    device->Destroy(runData.rwView[i]);
                    runData.rwView[i] = rhi::RWTexture2DView::Invalid;
                }
            }

            if (runData.rtv != rhi::RenderTargetView::Invalid)
            {
                device->Destroy(runData.rtv);
                runData.rtv = rhi::RenderTargetView::Invalid;
            }

            if (runData.dsv != rhi::DepthStencilView::Invalid)
            {
                device->Destroy(runData.dsv);
                runData.dsv = rhi::DepthStencilView::Invalid;
            }

            runData.resourceAccessMask = 0;
            runData.viewAccessMask = 0;
            runData.rwViewAccessMask = 0;
        }

        void DestroyTexture3DIfNeeded(rhi::Device* device, Texture3DRunData& runData, const Texture3DBuildData& buildData)
        {
            if (buildData.ownership == ResourceOwnership::External)
            {
                return;
            }

            if (runData.rhiTexture != rhi::Texture3D::Invalid &&
                runData.pinnedAllocation == rhi::GPUAllocation::Invalid)
            {
                device->Destroy(runData.rhiTexture);
                runData.rhiTexture = rhi::Texture3D::Invalid;
            }

            if (runData.view != rhi::Texture3DView::Invalid)
            {
                device->Destroy(runData.view);
                runData.view = rhi::Texture3DView::Invalid;
            }

            for (uint32_t i = 0; i < MAX_RW_VIEWS; ++i)
            {
                if (runData.rwView[i] != rhi::RWTexture3DView::Invalid)
                {
                    device->Destroy(runData.rwView[i]);
                    runData.rwView[i] = rhi::RWTexture3DView::Invalid;
                }
            }

            runData.resourceAccessMask = 0;
            runData.viewAccessMask = 0;
            runData.rwViewAccessMask = 0;
        }

        void DestroyBufferIfNeeded(rhi::Device* device, BufferRunData& runData, const BufferBuildData& buildData)
        {
            if (buildData.ownership == ResourceOwnership::External)
            {
                return;
            }

            if (runData.buffer.buffer != rhi::Buffer::Invalid &&
                runData.pinnedAllocation == rhi::GPUAllocation::Invalid)
            {
                runData.buffer = {};
            }

            if (runData.view != rhi::BufferView::Invalid)
            {
                device->Destroy(runData.view);
                runData.view = rhi::BufferView::Invalid;
            }

            if (runData.typedView != rhi::TypedBufferView::Invalid)
            {
                device->Destroy(runData.typedView);
                runData.typedView = rhi::TypedBufferView::Invalid;
            }

            if (runData.uniformView != rhi::UniformBufferView::Invalid)
            {
                device->Destroy(runData.uniformView);
                runData.uniformView = rhi::UniformBufferView::Invalid;
            }

            if (runData.rwView != rhi::RWBufferView::Invalid)
            {
                device->Destroy(runData.rwView);
                runData.rwView = rhi::RWBufferView::Invalid;
            }

            runData.resourceAccessMask = 0;
            runData.typedViewAccessMask = 0;
            runData.uniformViewAccessMask = 0;
            runData.rwViewAccessMask = 0;
        }

        void DestroyResourcesAndViewsOnReset(rhi::Device* device, RenderGraphImpl* impl)
        {
            for (auto it = impl->texture2DHandles.begin(); it != impl->texture2DHandles.end();)
            {
                rg::Texture2D texture = *it;
                Texture2DBuildData& buildData = impl->texture2Ds.GetColdMutable(texture);
                Texture2DRunData& runData = impl->texture2Ds.GetHotMutable(texture);
            
                DestroyTexture2DIfNeeded(device, runData, buildData);
                if (!TestFlag(buildData.resourceFlags, ResourceFlags::Pinned))
                {
                    impl->texture2Ds.Remove(texture);
                    it = impl->texture2DHandles.erase(it);
                }
                else
                {
                    buildData.firstUsedPass = INVALID_PASS_INDEX;
                    buildData.lastUsedPass = INVALID_PASS_INDEX;
                    buildData.barrierLastUpdated = INVALID_PASS_INDEX;
                    ++it;
                }
            }

            for (auto it = impl->texture3DHandles.begin(); it != impl->texture3DHandles.end();)
            {
                rg::Texture3D texture = *it;
                Texture3DBuildData& buildData = impl->texture3Ds.GetColdMutable(texture);
                Texture3DRunData& runData = impl->texture3Ds.GetHotMutable(texture);
                
                DestroyTexture3DIfNeeded(device, runData, buildData);
                if (!TestFlag(buildData.resourceFlags, ResourceFlags::Pinned))
                {
                    impl->texture3Ds.Remove(texture);
                    it = impl->texture3DHandles.erase(it);
                }
                else
                {
                    buildData.firstUsedPass = INVALID_PASS_INDEX;
                    buildData.lastUsedPass = INVALID_PASS_INDEX;
                    buildData.barrierLastUpdated = INVALID_PASS_INDEX;
                    ++it;
                }
            }

            for (auto it = impl->bufferHandles.begin(); it != impl->bufferHandles.end();)
            {
                rg::Buffer buffer = *it;
                BufferBuildData& buildData = impl->buffers.GetColdMutable(buffer);
                BufferRunData& runData = impl->buffers.GetHotMutable(buffer);
                
                DestroyBufferIfNeeded(device, runData, buildData);
                if (!TestFlag(buildData.resourceFlags, ResourceFlags::Pinned))
                {
                    impl->buffers.Remove(buffer);
                    it = impl->bufferHandles.erase(it);
                }
                else
                {
                    buildData.firstUsedPass = INVALID_PASS_INDEX;
                    buildData.lastUsedPass = INVALID_PASS_INDEX;
                    buildData.barrierLastUpdated = INVALID_PASS_INDEX;
                    ++it;
                }       
            }
        }
    }

    void RenderGraph::Reset(const rhi::Viewport& viewport)
    {
        RN_ASSERT(_isClosed);
        
        DestroyResourcesAndViewsOnReset(_device, _impl);

        _impl->viewportStack[0] = viewport;
        _impl->viewportIdx = 0;
        _impl->renderPassCount = 0;
        _impl->scratchAllocator.Reset();
        _impl->bufferAllocator.Flush(0);
        _isClosed = false;
    }


    namespace
    {
        void ExecuteRenderPass(
            const PassExecutionData& data,
            const RenderGraphResources& resources,
            rhi::Device* device,
            rhi::CommandList* cl,
            uint32_t passIdx)
        {
            RN_GPU_EVENT_SCOPE(cl, data.name);

            if (!data.bufferBarriers.empty() || 
                !data.texture2DBarriers.empty() || 
                !data.texture3DBarriers.empty())
            {
                cl->Barrier({
                    .pipelineBarriers = {},
                    .bufferBarriers = data.bufferBarriers,
                    .texture2DBarriers = data.texture2DBarriers,
                    .texture3DBarriers = data.texture3DBarriers
                });
            }

            // Compute or copy only jobs don't need a low-level render pass
            bool needsBeginEndRenderPass = !data.renderTargets.empty() || data.depthTarget.view != rhi::DepthStencilView::Invalid;
            if (needsBeginEndRenderPass)
            {
                cl->BeginRenderPass({
                    .viewport = data.viewport,
                    .renderTargets = data.renderTargets,
                    .depthTarget = data.depthTarget
                });
            }

            RenderPassContext passContext(
                device,
                cl,
                data.viewport,
                &resources,
                passIdx);
            
            if (data.onExecute)
            {
                data.onExecute(device, &passContext, cl, data.passData, passIdx);
            }

            if (needsBeginEndRenderPass)
            {
                cl->EndRenderPass();
            }
        }
    }
    void RenderGraph::Execute(RenderGraphExecutionFlags flags)
    {
        RN_ASSERT(_isClosed);
        if (!TestFlag(flags, RenderGraphExecutionFlags::ForceSingleThreaded))
        {
            struct ExecutePassTask : enki::ITaskSet
            {
                const RenderGraph* graph = nullptr;
                uint32_t passStartIdx = 0;
                uint32_t passCount = 0;

                rhi::CommandList** destCl = nullptr;

                void ExecuteRange(enki::TaskSetPartition range_, uint32_t threadnum_) override
                {
                    RenderGraphResources graphResources = {
                        .texture2Ds = &graph->_impl->texture2Ds,
                        .texture3Ds = &graph->_impl->texture3Ds,
                        .buffers = &graph->_impl->buffers,
                    };

                    rhi::CommandList* cl = graph->_device->AllocateCommandList();
                    for (uint32_t passIdx = passStartIdx; passIdx < passStartIdx + passCount; ++passIdx)
                    {
                        ExecuteRenderPass(
                            graph->_impl->passExecution[passIdx],
                            graphResources,
                            graph->_device,
                            cl,
                            passIdx);
                    }
                    *destCl = cl;
                }
            };

            struct SubmitCommandListsTask : enki::ITaskSet
            {
                Span<rhi::CommandList*> commandLists = {};
                rhi::Device* device = nullptr;

                void ExecuteRange(enki::TaskSetPartition range_, uint32_t threadnum_) override
                {
                    device->SubmitCommandLists(commandLists);
                }
            };

            uint32_t passCount = _impl->renderPassCount;
            ScopedVector<ExecutePassTask> passExecutionTasks(passCount);
            ScopedVector<enki::Dependency> taskDependencies(passCount);
            ScopedVector<rhi::CommandList*> orderedCommandLists(passCount);

            SubmitCommandListsTask submitCLsTask;

            uint32_t taskCount = 0;
            uint32_t currentPassStartIdx = 0;
            uint32_t currentPassCount = 0;
            for (uint32_t passIdx = 0; passIdx < passCount; ++passIdx)
            {
                ++currentPassCount;

                const PassExecutionData& passData = _impl->passExecution[passIdx];
                
                if (!TestFlag(passData.flags, RenderPassFlags::IsSmall) || (passIdx + 1 == passCount))
                {
                    ExecutePassTask& task = passExecutionTasks[taskCount];
                    task.graph = this;
                    task.passStartIdx = currentPassStartIdx;
                    task.passCount = currentPassCount;
                    task.destCl = &orderedCommandLists[taskCount];

                    submitCLsTask.SetDependency(taskDependencies[taskCount], &task);

                    currentPassCount = 0;
                    currentPassStartIdx = passIdx + 1;
                    ++taskCount;
                }
            }

            submitCLsTask.commandLists = orderedCommandLists;
            submitCLsTask.device = _device;

            for (uint32_t taskIdx = 0; taskIdx < taskCount; ++taskIdx)
            {
                TaskScheduler()->AddTaskSetToPipe(&passExecutionTasks[taskIdx]);
            }

            TaskScheduler()->WaitforTask(&submitCLsTask);
        }
        else
        {
            RenderGraphResources graphResources = {
                .texture2Ds = &_impl->texture2Ds,
                .texture3Ds = &_impl->texture3Ds,
                .buffers = &_impl->buffers,
            };

            uint64_t sortKey = 0;
            ScopedVector<rhi::CommandList*> commandLists;
            commandLists.reserve(_impl->renderPassCount);

            rhi::CommandList* currentCl = nullptr;

            uint32_t passCount = _impl->renderPassCount;
            for (uint32_t passIdx = 0; passIdx < passCount; ++passIdx)
            {
                if (!currentCl)
                {
                    currentCl = _device->AllocateCommandList();
                }

                ExecuteRenderPass(_impl->passExecution[passIdx],
                    graphResources,
                    _device,
                    currentCl,
                    passIdx);

                if (!TestFlag(_impl->passExecution[passIdx].flags, RenderPassFlags::IsSmall))
                {
                    commandLists.push_back(currentCl);
                    currentCl = nullptr;

                    // Combine small passes together with other passes
                    continue;
                }
            }

            if (currentCl)
            {
                commandLists.push_back(currentCl);
                currentCl = nullptr;
            }

            if (!commandLists.empty())
            {
                _device->SubmitCommandLists(commandLists);
            }
        }
    }

    void RenderGraph::PushViewport(const rhi::Viewport& viewport)
    {
        RN_ASSERT(_impl->viewportIdx + 1 <= MAX_VIEWPORT_STACK_SIZE);
        _impl->viewportStack[_impl->viewportIdx++] = viewport;
    }

    void RenderGraph::PopViewport()
    {
        RN_ASSERT(_impl->viewportIdx > 0);
        _impl->viewportIdx--;
    }

    const rhi::Viewport& RenderGraph::CurrentViewport() const
    {
        return _impl->viewportStack[_impl->viewportIdx];
    }
}