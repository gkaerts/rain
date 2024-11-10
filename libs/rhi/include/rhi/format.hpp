#pragma once

#include "common/common.hpp"

namespace rn::rhi
{
    enum class DepthFormat : uint32_t
    {
        None,
        D32Float,
        D24Unorm_S8,
        D16Unorm,

        Count
    };

    enum class RenderTargetFormat : uint32_t
    {
        None,
        RGBA8Unorm,
        RGBA16Float,
        RG16Float,
        RG16Uint,
        RG11B10Float,
        RGB10A2Unorm,
        R32Uint,
        BGRA8Unorm,

        Count
    };

    enum class TextureFormat : uint32_t
    {
        None,
        RGBA8Unorm,
        RGBA16Float,
        RG16Float,
        RG11B10Float,
        R32Float,
        R16Unorm,
        RGB10A2Unorm,
        R32Uint,
        R8Unorm,
        BGRA8Unorm,

        BC1,
        BC2,
        BC3,
        BC4,
        BC5,
        BC6H,
        BC7,

        ETC2,
        EAC_R11,
        EAC_RG11,
        ASTC_4x4,

        Count
    };

    size_t PixelOrBlockByteWidth(TextureFormat format);
    uint32_t BlockRowCount(TextureFormat format);

    DepthFormat ToDepthEquivalent(TextureFormat format);
    RenderTargetFormat ToRenderTargetEquivalent(TextureFormat format);
    TextureFormat ToTextureEquivalent(RenderTargetFormat format);
}