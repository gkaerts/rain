#pragma once

#include "common/common.hpp"
#include "rg/resource.hpp"

namespace rn::rhi
{
    class Device;
    class CommandList;
    struct Viewport;
};

namespace rn::rg
{
    class RenderPassContext;
    class RenderGraph;

    enum class RenderPassFlags
    {
        None = 0,

        IsSmall =           0x01, // Potentially merge this together with neighboring passes in one CL
        ComputeOnly =       0x02, // Render pass only contains compute work
        AllDrawUseEarlyZ =  0x04, // All draw in this render pass are guaranteed to not require late Z
        ReadOnlyDepth =     0x08, // Depth target attachment is read-only
    };
    RN_DEFINE_ENUM_CLASS_BITWISE_API(RenderPassFlags)

    template <typename UserData>
    using FnExecuteRenderPass = void(*)(rhi::Device*, RenderPassContext*, rhi::CommandList*, const UserData*, uint32_t);

    template <typename UserData>
    struct RenderPassDesc
    {
        const char* name;
        RenderPassFlags flags;

        Span<const TextureAttachment> colorAttachments;
        TextureAttachment depthAttachment;

        Span<const Texture2DUsage> texture2Ds;
        Span<const Texture3DUsage> texture3Ds;
        Span<const BufferUsage> buffers;
        Span<const rg::TLAS> tlases;

        FnExecuteRenderPass<UserData> onExecute;
    };

    enum class RenderGraphExecutionFlags
    {
        None = 0x0,
        ForceSingleThreaded = 0x01,
    };
    RN_DEFINE_ENUM_CLASS_BITWISE_API(RenderGraphExecutionFlags)

    struct RenderGraphImpl;
    class RenderGraph
    {
    public:

        RenderGraph(rhi::Device* device);
        ~RenderGraph();

        rg::Texture2D AllocateTexture2D(const rg::Texture2DDesc& desc);
        rg::Texture3D AllocateTexture3D(const rg::Texture3DDesc& desc);
        rg::Buffer AllocateBuffer(const rg::BufferDesc& desc);

        rg::Texture2D RegisterTexture2D(const Texture2DRegistrationDesc& desc);
        rg::Texture3D RegisterTexture3D(const Texture3DRegistrationDesc& desc);
        rg::Buffer RegisterBuffer(const BufferRegistrationDesc& desc);

        void PushViewport(const rhi::Viewport& viewport);
        void PopViewport();
        const rhi::Viewport& CurrentViewport() const;

        void Build();
        void Reset(const rhi::Viewport& viewport);
        void Execute(RenderGraphExecutionFlags flags = RenderGraphExecutionFlags::None);

        template <typename T>
        void AddRenderPass(const RenderPassDesc<T>& desc, const T* passData)
        {
            AddRenderPassInternal({
                .name = desc.name,
                .flags = desc.flags,
                .colorAttachments = desc.colorAttachments,
                .depthAttachment = desc.depthAttachment,
                .texture2Ds = desc.texture2Ds,
                .texture3Ds = desc.texture3Ds,
                .buffers = desc.buffers,
                .tlases = desc.tlases,
                .onExecute = reinterpret_cast<FnExecuteRenderPass<void>>(desc.onExecute)
            }, passData);
        }

    private:

        void AddRenderPassInternal(const RenderPassDesc<void>& desc, const void* passData);

        RenderGraphImpl* _impl = nullptr;
        rhi::Device* _device = nullptr;
        bool _isClosed = true;
    };
}