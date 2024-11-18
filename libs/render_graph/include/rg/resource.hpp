#pragma once

#include "common/common.hpp"
#include "common/handle.hpp"
#include "common/memory/span.hpp"

#include "rhi/resource.hpp"
#include "rhi/format.hpp"
#include "rhi/command_list.hpp"

namespace rn::rhi
{
    enum class LoadOp : uint32_t;
}

namespace rn::rg
{
    RN_DEFINE_HANDLE(Texture2D, 0x20);
    RN_DEFINE_HANDLE(Texture3D, 0x21);
    RN_DEFINE_HANDLE(Buffer, 0x22);
    RN_DEFINE_HANDLE(TLAS, 0x23)
    
    enum class TextureSizeMode : uint32_t
    {
        Adaptive = 0,   // Width and Height of the texture desc is a divisor of the render output size of the first pass this texture is seen in
        Fixed           // Width and Height get interpreted as-is
    };

    enum class ResourceFlags : uint32_t
    {
        None = 0x0,
        Pinned = 0x01,  // Pinned resources live on a dedicated GPU allocation and keep their contents and memory across render graph executions
    };
    RN_DEFINE_ENUM_CLASS_BITWISE_API(ResourceFlags)

    enum class TextureAccess : uint32_t
    {
        ShaderReadOnly = 0,
        ShaderReadWrite,

        CopySource,
        CopyDest,

        Presentation,

        Undefined
    };

    enum class BufferAccess : uint32_t
    {
        ShaderReadOnly = 0,
        ShaderReadWrite,

        CopySource,
        CopyDest,

        UniformBuffer,
        DrawIDBuffer,
        IndexBuffer,
        ArgumentBuffer,

        Undefined
    };

    enum class ResourceReadWriteFlags
    {
        None = 0x0,
        SyncBeforeReadWriteAccess = 0x01,
    };
    RN_DEFINE_ENUM_CLASS_BITWISE_API(ResourceReadWriteFlags)

    struct Texture2DDesc
    {
        uint32_t width;
        uint32_t height;
        uint32_t mipLevels;

        rhi::TextureFormat format;
        rhi::RenderTargetFormat renderFormat;
        rhi::DepthFormat depthFormat;

        rhi::ClearValue clearValue;
        TextureSizeMode sizeMode;
        ResourceFlags flags;
        const char* name;
    };

    struct Texture3DDesc
    {
        uint32_t width;
        uint32_t height;
        uint32_t depth;
        uint32_t mipLevels;

        rhi::TextureFormat format;
        ResourceFlags flags;
        const char* name;
    };

    struct BufferDesc
    {
        uint32_t sizeInBytes;
        ResourceFlags flags;
        const char* name;
    };

    struct Texture2DRegistrationDesc
    {
        rhi::Texture2D texture;
        rhi::Texture2DView view;
        Span<rhi::RWTexture2DView> rwViews;

        rhi::RenderTargetView rtv;
        rhi::DepthStencilView dsv;

        rhi::ClearValue clearValue;

        rhi::PipelineSyncStage lastSyncStage;
        rhi::PipelineAccess lastAccess;
        rhi::TextureLayout lastLayout;
        const char* name;
    };

    struct Texture3DRegistrationDesc
    {
        rhi::Texture3D texture;
        rhi::Texture3DView view;
        Span<rhi::RWTexture3DView> rwViews;

        rhi::PipelineSyncStage lastSyncStage;
        rhi::PipelineAccess lastAccess;
        rhi::TextureLayout lastLayout;
        const char* name;
    };

    struct BufferRegistrationDesc
    {
        rhi::Buffer buffer;
        rhi::BufferView view;
        rhi::UniformBufferView uniformView;
        rhi::TypedBufferView typedView;
        rhi::RWBufferView rwView;

        rhi::PipelineSyncStage lastSyncStage;
        rhi::PipelineAccess lastAccess;
        const char* name;
    };

    struct Texture2DUsage
    {
        rg::Texture2D texture;
        TextureAccess access;
        
        struct
        {
            uint32_t mipIndex;
            ResourceReadWriteFlags flags;
        } shaderReadWrite;
    };

    struct Texture3DUsage
    {
        rg::Texture3D texture;
        TextureAccess access;

        struct
        {
            uint32_t mipIndex;
            ResourceReadWriteFlags flags;
        } shaderReadWrite;
    };

    struct BufferUsage
    {
        rg::Buffer buffer;
        BufferAccess access;
        uint32_t structureSizeInBytes;

        struct
        {
            ResourceReadWriteFlags flags;
        } shaderReadWrite;
    };

    enum class ResourceType
    {
        Texture2D = 0,
        Texture3D,
        Buffer
    };

    struct TextureAttachment
    {
        Texture2D texture;
        rhi::LoadOp loadOp;
    };

    Texture2DUsage  ShaderReadOnly(Texture2D texture);
    Texture3DUsage  ShaderReadOnly(Texture3D texture);
    BufferUsage     ShaderReadOnly(Buffer buffer, uint32_t structureSizeInBytes = 0);

    Texture2DUsage  ShaderReadWrite(Texture2D texture, uint32_t mipIndex, ResourceReadWriteFlags rwFlags = ResourceReadWriteFlags::None);
    Texture3DUsage  ShaderReadWrite(Texture3D texture, uint32_t mipIndex, ResourceReadWriteFlags rwFlags = ResourceReadWriteFlags::None);
    BufferUsage     ShaderReadWrite(Buffer buffer, uint32_t structureSizeInBytes = 0, ResourceReadWriteFlags rwFlags = ResourceReadWriteFlags::None);

    BufferUsage     UniformBuffer(Buffer buffer);
    BufferUsage     DrawIDBuffer(Buffer buffer);
    BufferUsage     IndexBuffer(Buffer buffer);
    BufferUsage     ArgumentBuffer(Buffer buffer);

    Texture2DUsage  Present(Texture2D texture);
}