#include "rg/resource.hpp"

namespace rn::rg
{
    Texture2DUsage ShaderReadOnly(Texture2D texture)
    {
        return {
            .texture = texture,
            .access = TextureAccess::ShaderReadOnly,
        };
    }

    Texture3DUsage ShaderReadOnly(Texture3D texture)
    {
        return {
            .texture = texture,
            .access = TextureAccess::ShaderReadOnly,
        };
    }

    BufferUsage ShaderReadOnly(Buffer buffer, uint32_t structureSizeInBytes)
    {
        return {
            .buffer = buffer,
            .access = BufferAccess::ShaderReadOnly,
            .structureSizeInBytes = structureSizeInBytes
        };
    }

    Texture2DUsage ShaderReadWrite(Texture2D texture, uint32_t mipIndex, ResourceReadWriteFlags rwFlags)
    {
        return {
            .texture = texture,
            .access = TextureAccess::ShaderReadWrite,
            .shaderReadWrite = {
                .mipIndex = mipIndex,
                .flags = rwFlags
            }
        };
    }

    Texture3DUsage ShaderReadWrite(Texture3D texture, uint32_t mipIndex, ResourceReadWriteFlags rwFlags)
    {
        return {
            .texture = texture,
            .access = TextureAccess::ShaderReadWrite,
            .shaderReadWrite = {
                .mipIndex = mipIndex,
                .flags = rwFlags
            }
        };
    }

    BufferUsage ShaderReadWrite(Buffer buffer, uint32_t structureSizeInBytes, ResourceReadWriteFlags rwFlags)
    {
        return {
            .buffer = buffer,
            .access = BufferAccess::ShaderReadWrite,
            .structureSizeInBytes = structureSizeInBytes,
            .shaderReadWrite = {
                .flags = rwFlags
            }
        };
    }

    BufferUsage UniformBuffer(Buffer buffer)
    {
        return {
            .buffer = buffer,
            .access = BufferAccess::UniformBuffer,
            .structureSizeInBytes = 0
        };
    }

    BufferUsage DrawIDBuffer(Buffer buffer)
    {
        return {
            .buffer = buffer,
            .access = BufferAccess::DrawIDBuffer,
            .structureSizeInBytes = 0
        };
    }

    BufferUsage IndexBuffer(Buffer buffer)
    {
        return {
            .buffer = buffer,
            .access = BufferAccess::IndexBuffer,
            .structureSizeInBytes = 0
        };
    }

    BufferUsage ArgumentBuffer(Buffer buffer)
    {
        return {
            .buffer = buffer,
            .access = BufferAccess::ArgumentBuffer,
            .structureSizeInBytes = 0
        };
    }

    Texture2DUsage Present(Texture2D texture)
    {
        return {
            .texture = texture,
            .access = TextureAccess::Presentation
        };
    }

}