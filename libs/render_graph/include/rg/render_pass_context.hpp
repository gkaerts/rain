#pragma once

#include "common/common.hpp"
#include "common/memory/span.hpp"
#include "rhi/handles.hpp"
#include "rhi/command_list.hpp"
#include "rg/resource.hpp"


namespace rn::rg
{
    constexpr const uint32_t MAX_TEMP_BUFFER_COUNT = 256;
    class RenderGraph;
    struct RenderGraphResources;
    class RenderPassContext
    {
    public:

        RenderPassContext(rhi::Device* device, 
            rhi::CommandList* cl,
            const rhi::Viewport& viewport,
            const RenderGraphResources* resources, 
            uint32_t passIdx);

        ~RenderPassContext();

        rhi::CommandList*       CommandList() const { return _cl; }
        const rhi::Viewport&    Viewport() const { return _viewport; }

        rhi::Buffer             Resolve(rg::Buffer buffer) const;
        rhi::BufferView         ResolveView(rg::Buffer buffer) const;
        rhi::TypedBufferView    ResolveTypedView(rg::Buffer buffer) const;
        rhi::UniformBufferView  ResolveUniformView(rg::Buffer buffer) const;
        rhi::RWBufferView       ResolveRWView(rg::Buffer buffer) const;

        rhi::Texture2D          Resolve(rg::Texture2D texture) const;
        rhi::Texture2DView      ResolveView(rg::Texture2D texture) const;
        rhi::RWTexture2DView    ResolveRWView(rg::Texture2D texture, uint32_t mipIdx) const;

        rhi::Texture3D          Resolve(rg::Texture3D texture) const;
        rhi::Texture3DView      ResolveView(rg::Texture3D texture) const;
        rhi::RWTexture3DView    ResolveRWView(rg::Texture3D texture, uint32_t mipIdx) const;

        rhi::BufferView         AllocateTemporaryBufferView(Span<const uint8_t> data);
        rhi::UniformBufferView  AllocateTemporaryUniformBufferView(Span<const uint8_t> data);

        template <typename T>
        rhi::BufferView         AllocateTemporaryBufferView(const T& data) 
        {
            return AllocateTemporaryBufferView(Span<const uint8_t>(&data, sizeof(data)));
        }

        template <typename T>
        rhi::UniformBufferView  AllocateTemporaryUniformBufferView(const T& data) 
        {
            return AllocateTemporaryUniformBufferView(Span<const uint8_t>(&data, sizeof(data)));
        }

    private:

        rhi::Device* _device = nullptr;
        rhi::CommandList* _cl = nullptr;
        const RenderGraphResources* _resources = nullptr;

        rhi::Viewport _viewport = {};
        uint32_t _passIdx = 0;

        rhi::BufferView _tempBuffers[MAX_TEMP_BUFFER_COUNT] = {};
        uint32_t _tempBufferCount = 0;

        rhi::UniformBufferView _tempUniformBuffers[MAX_TEMP_BUFFER_COUNT] = {};
        uint32_t _tempUniformBufferCount = 0;

    };
}