#pragma once

#include "common/common.hpp"
#include "common/memory/object_pool.hpp"

#include "rg/resource.hpp"

#include "rhi/command_list.hpp"

#include <bitset>

namespace rn::rg
{
    enum class ResourceOwnership : uint32_t
    {
        Internal = 0,
        External
    };

    constexpr const uint32_t MAX_RENDER_PASS_COUNT = 128;
    constexpr const uint32_t MAX_VIEWPORT_STACK_SIZE = 16;
    constexpr const uint32_t INVALID_PASS_INDEX = 0xFFFFFFFF;

    struct Texture2DBuildData
    {
        Texture2DDesc desc;

        uint32_t firstUsedPass = INVALID_PASS_INDEX;
        uint32_t lastUsedPass = INVALID_PASS_INDEX;

        ResourceFlags resourceFlags = ResourceFlags::None;
        rhi::TextureCreationFlags creationFlags = rhi::TextureCreationFlags::None;
        uint32_t rwViewMipBitmask = 0;
        ResourceOwnership ownership = ResourceOwnership::Internal;

        rhi::GPUMemoryRegion gpuRegion = {};

        uint32_t barrierLastUpdated = INVALID_PASS_INDEX;
        rhi::PipelineSyncStage prevSyncStage = rhi::PipelineSyncStage::None;
        rhi::PipelineAccess prevAccess = rhi::PipelineAccess::None;
        rhi::TextureLayout prevLayout = rhi::TextureLayout::Undefined;

        rhi::PipelineSyncStage currSyncStage = rhi::PipelineSyncStage::None;
        rhi::PipelineAccess currAccess = rhi::PipelineAccess::None;
        rhi::TextureLayout currLayout = rhi::TextureLayout::Undefined;

        bool requiresReadWriteBarrier = false;
    };

    struct Texture3DBuildData
    {
        Texture3DDesc desc;

        uint32_t firstUsedPass = INVALID_PASS_INDEX;
        uint32_t lastUsedPass = INVALID_PASS_INDEX;

        ResourceFlags resourceFlags = ResourceFlags::None;
        rhi::TextureCreationFlags creationFlags = rhi::TextureCreationFlags::None;
        uint32_t rwViewMipBitmask = 0;
        ResourceOwnership ownership = ResourceOwnership::Internal;

        rhi::GPUMemoryRegion gpuRegion = {};

        uint32_t barrierLastUpdated = INVALID_PASS_INDEX;
        rhi::PipelineSyncStage prevSyncStage = rhi::PipelineSyncStage::None;
        rhi::PipelineAccess prevAccess = rhi::PipelineAccess::None;
        rhi::TextureLayout prevLayout = rhi::TextureLayout::Undefined;

        rhi::PipelineSyncStage currSyncStage = rhi::PipelineSyncStage::None;
        rhi::PipelineAccess currAccess = rhi::PipelineAccess::None;
        rhi::TextureLayout currLayout = rhi::TextureLayout::Undefined;

        bool requiresReadWriteBarrier = false;
    };

    struct BufferBuildData
    {
        BufferDesc desc;

        uint32_t firstUsedPass = INVALID_PASS_INDEX;
        uint32_t lastUsedPass = INVALID_PASS_INDEX;

        ResourceFlags resourceFlags = ResourceFlags::None;
        rhi::BufferCreationFlags creationFlags = rhi::BufferCreationFlags::None;
        ResourceOwnership ownership;

        rhi::GPUMemoryRegion gpuRegion = {};

        uint32_t barrierLastUpdated = INVALID_PASS_INDEX;
        rhi::PipelineSyncStage prevSyncStage = rhi::PipelineSyncStage::None;
        rhi::PipelineAccess prevAccess = rhi::PipelineAccess::None;

        rhi::PipelineSyncStage currSyncStage = rhi::PipelineSyncStage::None;
        rhi::PipelineAccess currAccess = rhi::PipelineAccess::None;

        bool requiresReadWriteBarrier = false;
    };

    constexpr const uint32_t MAX_RW_VIEWS = 16;
    struct Texture2DRunData
    {
        rhi::GPUAllocation pinnedAllocation = rhi::GPUAllocation::Invalid;

        rhi::Texture2D rhiTexture = rhi::Texture2D::Invalid;
        rhi::Texture2DView view = rhi::Texture2DView::Invalid;
        rhi::RWTexture2DView rwView[MAX_RW_VIEWS] = {};

        rhi::RenderTargetView rtv = rhi::RenderTargetView::Invalid;
        rhi::DepthStencilView dsv = rhi::DepthStencilView::Invalid;

        rhi::ClearValue clearValue = {};

        std::bitset<MAX_RENDER_PASS_COUNT> resourceAccessMask;
        std::bitset<MAX_RENDER_PASS_COUNT> viewAccessMask;
        std::bitset<MAX_RENDER_PASS_COUNT> rwViewAccessMask;
    };
    
    struct Texture3DRunData
    {
        rhi::GPUAllocation pinnedAllocation = rhi::GPUAllocation::Invalid;

        rhi::Texture3D rhiTexture = rhi::Texture3D::Invalid;
        rhi::Texture3DView view = rhi::Texture3DView::Invalid;
        rhi::RWTexture3DView rwView[MAX_RW_VIEWS] = {};

        std::bitset<MAX_RENDER_PASS_COUNT> resourceAccessMask;
        std::bitset<MAX_RENDER_PASS_COUNT> viewAccessMask;
        std::bitset<MAX_RENDER_PASS_COUNT> rwViewAccessMask;
    };

    struct BufferRunData
    {
        rhi::GPUAllocation pinnedAllocation = rhi::GPUAllocation::Invalid;

        rhi::Buffer buffer = rhi::Buffer::Invalid;
        rhi::BufferView view = rhi::BufferView::Invalid;
        rhi::TypedBufferView typedView = rhi::TypedBufferView::Invalid;
        rhi::UniformBufferView uniformView = rhi::UniformBufferView::Invalid;
        rhi::RWBufferView rwView = rhi::RWBufferView::Invalid;

        std::bitset<MAX_RENDER_PASS_COUNT> resourceAccessMask;
        std::bitset<MAX_RENDER_PASS_COUNT> viewAccessMask;
        std::bitset<MAX_RENDER_PASS_COUNT> typedViewAccessMask;
        std::bitset<MAX_RENDER_PASS_COUNT> uniformViewAccessMask;
        std::bitset<MAX_RENDER_PASS_COUNT> rwViewAccessMask;
    };

    using Texture2DPool = ObjectPool<rg::Texture2D, Texture2DRunData, Texture2DBuildData>;
    using Texture3DPool = ObjectPool<rg::Texture3D, Texture3DRunData, Texture3DBuildData>;
    using BufferPool = ObjectPool<rg::Buffer, BufferRunData, BufferBuildData>;

    struct RenderGraphResources
    {
        Texture2DPool* texture2Ds;
        Texture3DPool* texture3Ds;
        BufferPool* buffers;
    };
}