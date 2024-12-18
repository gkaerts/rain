#include "rg/render_pass_context.hpp"
#include "rhi/device.hpp"
#include "rhi/command_list.hpp"

#include "render_graph_internal.hpp"

namespace rn::rg
{
    RenderPassContext::RenderPassContext(rhi::Device* device, 
        rhi::CommandList* cl,
        const rhi::Viewport& viewport,
        const RenderGraphResources* resources, 
        uint32_t passIdx)
        : _device(device)
        , _cl(cl)
        , _resources(resources)
        , _viewport(viewport)
        , _passIdx(passIdx)
    {}

    RenderPassContext::~RenderPassContext()
    {
        for (uint32_t i = 0; i < _tempBufferCount; ++i)
        {
            _device->Destroy(_tempBuffers[i]);
        }

        for (uint32_t i = 0; i < _tempUniformBufferCount; ++i)
        {
            _device->Destroy(_tempUniformBuffers[i]);
        }
    }

    BufferRegion RenderPassContext::Resolve(rg::Buffer buffer) const
    {
        BufferRegion outRegion = {};

        const BufferRunData& runData = _resources->buffers->GetHot(buffer);

        bool passIsAllowedToAccessHandle = runData.resourceAccessMask.test(_passIdx);
        RN_ASSERT(passIsAllowedToAccessHandle);

        if (passIsAllowedToAccessHandle)
        {
            outRegion = runData.buffer;
        }

        return outRegion;
    }

    rhi::BufferView RenderPassContext::ResolveView(rg::Buffer buffer) const
    {
        rhi::BufferView outHandle = rhi::BufferView::Invalid;

        const BufferRunData& runData = _resources->buffers->GetHot(buffer);
        
        bool passIsAllowedToAccessHandle = runData.viewAccessMask.test(_passIdx);
        RN_ASSERT(passIsAllowedToAccessHandle);

        if (passIsAllowedToAccessHandle)
        {
            outHandle = runData.view;
        }

        return outHandle;
    }

    rhi::TypedBufferView RenderPassContext::ResolveTypedView(rg::Buffer buffer) const
    {
        rhi::TypedBufferView outHandle = rhi::TypedBufferView::Invalid;

        const BufferRunData& runData = _resources->buffers->GetHot(buffer);
        
        bool passIsAllowedToAccessHandle = runData.typedViewAccessMask.test(_passIdx);
        RN_ASSERT(passIsAllowedToAccessHandle);

        if (passIsAllowedToAccessHandle)
        {
            outHandle = runData.typedView;
        }

        return outHandle;
    }

    rhi::UniformBufferView RenderPassContext::ResolveUniformView(rg::Buffer buffer) const
    {
        rhi::UniformBufferView outHandle = rhi::UniformBufferView::Invalid;

        const BufferRunData& runData = _resources->buffers->GetHot(buffer);
        
        bool passIsAllowedToAccessHandle = runData.uniformViewAccessMask.test(_passIdx);
        RN_ASSERT(passIsAllowedToAccessHandle);

        if (passIsAllowedToAccessHandle)
        {
            outHandle = runData.uniformView;
        }

        return outHandle;
    }

    rhi::RWBufferView RenderPassContext::ResolveRWView(rg::Buffer buffer) const
    {
        rhi::RWBufferView outHandle = rhi::RWBufferView::Invalid;

        const BufferRunData& runData = _resources->buffers->GetHot(buffer);
        
        bool passIsAllowedToAccessHandle = runData.rwViewAccessMask.test(_passIdx);
        RN_ASSERT(passIsAllowedToAccessHandle);

        if (passIsAllowedToAccessHandle)
        {
            outHandle = runData.rwView;
        }

        return outHandle;
    }

    rhi::Texture2D RenderPassContext::Resolve(rg::Texture2D texture) const
    {
        rhi::Texture2D outHandle = rhi::Texture2D::Invalid;

        const Texture2DRunData& runData = _resources->texture2Ds->GetHot(texture);
        
        bool passIsAllowedToAccessHandle = runData.resourceAccessMask.test(_passIdx);
        RN_ASSERT(passIsAllowedToAccessHandle);

        if (passIsAllowedToAccessHandle)
        {
            outHandle = runData.rhiTexture;
        }

        return outHandle;
    }

    rhi::Texture2DView RenderPassContext::ResolveView(rg::Texture2D texture) const
    {
        rhi::Texture2DView outHandle = rhi::Texture2DView::Invalid;

        const Texture2DRunData& runData = _resources->texture2Ds->GetHot(texture);
        
        bool passIsAllowedToAccessHandle = runData.viewAccessMask.test(_passIdx);
        RN_ASSERT(passIsAllowedToAccessHandle);

        if (passIsAllowedToAccessHandle)
        {
            outHandle = runData.view;
        }

        return outHandle;
    }

    rhi::RWTexture2DView RenderPassContext::ResolveRWView(rg::Texture2D texture, uint32_t mipIdx) const
    {
        rhi::RWTexture2DView outHandle = rhi::RWTexture2DView::Invalid;

        const Texture2DRunData& runData = _resources->texture2Ds->GetHot(texture);
        
        bool passIsAllowedToAccessHandle = runData.rwViewAccessMask.test(_passIdx);
        RN_ASSERT(passIsAllowedToAccessHandle);

        if (passIsAllowedToAccessHandle)
        {
            outHandle = runData.rwView[mipIdx];
        }

        return outHandle;
    }

    rhi::Texture3D RenderPassContext::Resolve(rg::Texture3D texture) const
    {
        rhi::Texture3D outHandle = rhi::Texture3D::Invalid;

        const Texture3DRunData& runData = _resources->texture3Ds->GetHot(texture);
        
        bool passIsAllowedToAccessHandle = runData.resourceAccessMask.test(_passIdx);
        RN_ASSERT(passIsAllowedToAccessHandle);

        if (passIsAllowedToAccessHandle)
        {
            outHandle = runData.rhiTexture;
        }

        return outHandle;
    }

    rhi::Texture3DView RenderPassContext::ResolveView(rg::Texture3D texture) const
    {
        rhi::Texture3DView outHandle = rhi::Texture3DView::Invalid;

        const Texture3DRunData& runData = _resources->texture3Ds->GetHot(texture);
        
        bool passIsAllowedToAccessHandle = runData.viewAccessMask.test(_passIdx);
        RN_ASSERT(passIsAllowedToAccessHandle);

        if (passIsAllowedToAccessHandle)
        {
            outHandle = runData.view;
        }

        return outHandle;
    }

    rhi::RWTexture3DView RenderPassContext::ResolveRWView(rg::Texture3D texture, uint32_t mipIdx) const
    {
        rhi::RWTexture3DView outHandle = rhi::RWTexture3DView::Invalid;

        const Texture3DRunData& runData = _resources->texture3Ds->GetHot(texture);
        
        bool passIsAllowedToAccessHandle = runData.rwViewAccessMask.test(_passIdx);
        RN_ASSERT(passIsAllowedToAccessHandle);

        if (passIsAllowedToAccessHandle)
        {
            outHandle = runData.rwView[mipIdx];
        }

        return outHandle;
    }

    rhi::BufferView RenderPassContext::AllocateTemporaryBufferView(Span<const uint8_t> data)
    {
        RN_ASSERT(_tempBufferCount + 1 <= MAX_TEMP_BUFFER_COUNT);

        rhi::TemporaryResource tempResource = _cl->AllocateTemporaryResource(uint32_t(data.size()));
        std::memcpy(tempResource.cpuPtr, data.data(), data.size());

        rhi::BufferView view = _device->CreateBufferView({
            .buffer = tempResource.buffer,
            .offsetInBytes = tempResource.offsetInBytes,
            .sizeInBytes = tempResource.sizeInBytes
        });

        _tempBuffers[_tempBufferCount++] = view;
        return view;
    }

    rhi::UniformBufferView RenderPassContext::AllocateTemporaryUniformBufferView(Span<const uint8_t> data)
    {
        RN_ASSERT(_tempUniformBufferCount + 1 <= MAX_TEMP_BUFFER_COUNT);

        rhi::TemporaryResource tempResource = _cl->AllocateTemporaryResource(uint32_t(data.size()));
        std::memcpy(tempResource.cpuPtr, data.data(), data.size());

        rhi::UniformBufferView view = _device->CreateUniformBufferView({
            .buffer = tempResource.buffer,
            .offsetInBytes = tempResource.offsetInBytes,
            .sizeInBytes = tempResource.sizeInBytes
        });

        _tempUniformBuffers[_tempUniformBufferCount++] = view;
        return view;
    }

}