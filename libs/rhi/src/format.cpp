#include "rhi/format.hpp"

namespace rn::rhi
{
    size_t PixelOrBlockByteWidth(TextureFormat format)
    {
        size_t width = 1;
        switch (format)
        {
        case TextureFormat::RGBA8Unorm:
        case TextureFormat::RG16Float:
        case TextureFormat::RG11B10Float:
        case TextureFormat::R32Float:
        case TextureFormat::RGB10A2Unorm:
        case TextureFormat::BGRA8Unorm:
            width = 4;
            break;
        case TextureFormat::RGBA16Float:
            width = 8;
            break;
        case TextureFormat::R16Unorm:
            width = 2;
            break;
        case TextureFormat::BC1:
        case TextureFormat::BC4:
        case TextureFormat::EAC_R11:
            width = 8;
            break;
        case TextureFormat::BC2:
        case TextureFormat::BC3:
        case TextureFormat::BC5:
        case TextureFormat::BC6H:
        case TextureFormat::BC7:
        case TextureFormat::ASTC_4x4:
        case TextureFormat::EAC_RG11:
        case TextureFormat::ETC2:
            width = 16;
            break;
        case TextureFormat::None:
        default:
            break;
        }

        return width;
    }

    uint32_t BlockRowCount(TextureFormat format)
    {
        uint32_t rowCount = 1;

        switch (format)
        {
        case TextureFormat::BC1:
        case TextureFormat::BC4:  
        case TextureFormat::BC2:
        case TextureFormat::BC3:
        case TextureFormat::BC5:
        case TextureFormat::BC6H:
        case TextureFormat::BC7:
        case TextureFormat::EAC_R11:
        case TextureFormat::EAC_RG11:
        case TextureFormat::ETC2:
        case TextureFormat::ASTC_4x4:
            rowCount = 4;
            break;
        case TextureFormat::None:
        case TextureFormat::RGBA8Unorm:
        case TextureFormat::RG16Float:
        case TextureFormat::RG11B10Float:
        case TextureFormat::R32Float:
        case TextureFormat::RGBA16Float:
        case TextureFormat::R16Unorm:
        case TextureFormat::RGB10A2Unorm:
        case TextureFormat::BGRA8Unorm:
        default:
            break;
        }

        return rowCount;
    }

    DepthFormat ToDepthEquivalent(TextureFormat format)
    {
        constexpr DepthFormat LUT[] =
        {
            DepthFormat::None,     // None
            DepthFormat::None,     // RGBA8Unorm
            DepthFormat::None,     // RGBA16Float
            DepthFormat::None,     // RG16Float
            DepthFormat::None,     // RG11B10Float
            DepthFormat::D32Float, // R32Float
            DepthFormat::D16Unorm, // R16Unorm
            DepthFormat::None,     // RGB10A2Unorm
            DepthFormat::None,     // R32Uint
            DepthFormat::None,     // R8Unorm
            DepthFormat::None,     // BGRA8Unorm

            DepthFormat::None,      // BC1
            DepthFormat::None,      // BC2
            DepthFormat::None,      // BC3
            DepthFormat::None,      // BC4
            DepthFormat::None,      // BC5
            DepthFormat::None,      // BC6H
            DepthFormat::None,      // BC7

            DepthFormat::None,      // EAC_R11
            DepthFormat::None,      // EAC_RG11
            DepthFormat::None,      // ETC2
            DepthFormat::None,      // ASTC_4x4
        };
        RN_MATCH_ENUM_AND_ARRAY(LUT, TextureFormat)

        return LUT[int(format)];
    }

    RenderTargetFormat ToRenderTargetEquivalent(TextureFormat format)
    {
        constexpr RenderTargetFormat LUT[] =
        {
            RenderTargetFormat::None,         // None
            RenderTargetFormat::RGBA8Unorm,   // RGBA8Unorm
            RenderTargetFormat::RGBA16Float,  // RGBA16Float
            RenderTargetFormat::RG16Float,    // RG16Float
            RenderTargetFormat::RG11B10Float, // RG11B10Float
            RenderTargetFormat::None,         // R32Float
            RenderTargetFormat::None,         // R16Unorm
            RenderTargetFormat::RGB10A2Unorm, // RGB10A2Unorm
            RenderTargetFormat::R32Uint,      // R32Uint
            RenderTargetFormat::None,         // R8Unorm
            RenderTargetFormat::BGRA8Unorm,   // BGRA8Unorm

            RenderTargetFormat::None,         // BC1
            RenderTargetFormat::None,         // BC2
            RenderTargetFormat::None,         // BC3
            RenderTargetFormat::None,         // BC4
            RenderTargetFormat::None,         // BC5
            RenderTargetFormat::None,         // BC6H
            RenderTargetFormat::None,         // BC7

            RenderTargetFormat::None,         // EAC_R11
            RenderTargetFormat::None,         // EAC_RG11
            RenderTargetFormat::None,         // ETC2
            RenderTargetFormat::None,         // ASTC_4x4
        };
        RN_MATCH_ENUM_AND_ARRAY(LUT, TextureFormat)

        return LUT[int(format)];
    }

    TextureFormat ToTextureEquivalent(RenderTargetFormat format)
    {
        constexpr TextureFormat LUT[] =
        {
            TextureFormat::None,           // None,
            TextureFormat::RGBA8Unorm,     // RGBA8Unorm,
            TextureFormat::RGBA16Float,    // RGBA16Float,
            TextureFormat::RG16Float,      // RG16Float,
            TextureFormat::None,           // RG16Uint,
            TextureFormat::RG11B10Float,   // RG11B10Float,
            TextureFormat::RGB10A2Unorm,   // RGB10A2Unorm,
            TextureFormat::R32Uint,        // R32Uint,
            TextureFormat::BGRA8Unorm,     // BGRA8Unorm
        };
        RN_MATCH_ENUM_AND_ARRAY(LUT, RenderTargetFormat)

        return LUT[int(format)];
    }
}