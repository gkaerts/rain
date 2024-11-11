#include "format_d3d12.hpp"
#include "rhi/format.hpp"

namespace rn::rhi
{
    namespace
    {
        constexpr const DXGI_FORMAT DEPTH_FORMATS[] =
        {
            DXGI_FORMAT_UNKNOWN,            // None
            DXGI_FORMAT_D32_FLOAT,          // D32Float
            DXGI_FORMAT_D24_UNORM_S8_UINT,  // D24Unorm_S8
            DXGI_FORMAT_D16_UNORM           // D16Unorm
        };
        RN_MATCH_ENUM_AND_ARRAY(DEPTH_FORMATS, DepthFormat)

        constexpr const DXGI_FORMAT RENDER_TARGET_FORMATS[] =
        {
            DXGI_FORMAT_UNKNOWN,            // None
            DXGI_FORMAT_R8G8B8A8_UNORM,     // RGBA8Unorm
            DXGI_FORMAT_R16G16B16A16_FLOAT, // RGBA16Float
            DXGI_FORMAT_R16G16_FLOAT,       // RG16Float
            DXGI_FORMAT_R16G16_UINT,        // RG16Uint
            DXGI_FORMAT_R11G11B10_FLOAT,    // RG11B10Float
            DXGI_FORMAT_R10G10B10A2_UNORM,  // RGB10A2Unorm
            DXGI_FORMAT_R32_UINT,           // R32Uint
            DXGI_FORMAT_B8G8R8A8_UNORM,     // BGRA8Unorm
        };
        RN_MATCH_ENUM_AND_ARRAY(RENDER_TARGET_FORMATS, RenderTargetFormat)

        constexpr const DXGI_FORMAT TYPED_TEXTURE_FORMATS[] =
        {
            DXGI_FORMAT_UNKNOWN,            // None
            DXGI_FORMAT_R8G8B8A8_UNORM,     // RGBA8Unorm
            DXGI_FORMAT_R16G16B16A16_FLOAT, // RGBA16Float
            DXGI_FORMAT_R16G16_FLOAT,       // RG16Float
            DXGI_FORMAT_R11G11B10_FLOAT,    // RG11B10Float
            DXGI_FORMAT_R32_FLOAT,          // R32Float
            DXGI_FORMAT_R16_UNORM,          // R16Unorm
            DXGI_FORMAT_R10G10B10A2_UNORM,  // RGB10A2Unorm
            DXGI_FORMAT_R32_UINT,           // R32Uint
            DXGI_FORMAT_R8_UNORM,           // R8Unorm
            DXGI_FORMAT_B8G8R8A8_UNORM,     // BGRA8Unorm
            

            DXGI_FORMAT_BC1_UNORM,          // BC1
            DXGI_FORMAT_BC2_UNORM,          // BC2
            DXGI_FORMAT_BC3_UNORM,          // BC3
            DXGI_FORMAT_BC4_UNORM,          // BC4
            DXGI_FORMAT_BC5_UNORM,          // BC5
            DXGI_FORMAT_BC6H_SF16,          // BC6H
            DXGI_FORMAT_BC7_UNORM,          // BC7

            DXGI_FORMAT_UNKNOWN,            // EAC_R11
            DXGI_FORMAT_UNKNOWN,            // EAC_RG11
            DXGI_FORMAT_UNKNOWN,            // ETC2
            DXGI_FORMAT_UNKNOWN,            // ASTC_4x4
        };
        RN_MATCH_ENUM_AND_ARRAY(TYPED_TEXTURE_FORMATS, TextureFormat)

        constexpr DXGI_FORMAT TYPELESS_TEXTURE_FORMATS[] =
        {
            DXGI_FORMAT_UNKNOWN,               // None
            DXGI_FORMAT_R8G8B8A8_TYPELESS,     // RGBA8Unorm
            DXGI_FORMAT_R16G16B16A16_TYPELESS, // RGBA16Float
            DXGI_FORMAT_R16G16_TYPELESS,       // RG16Float
            DXGI_FORMAT_UNKNOWN,               // RG11B10Float
            DXGI_FORMAT_R32_TYPELESS,          // R32Float
            DXGI_FORMAT_R16_TYPELESS,          // R16Unorm
            DXGI_FORMAT_R10G10B10A2_TYPELESS,  // RGB10A2Unorm
            DXGI_FORMAT_R32_TYPELESS,          // R32Uint
            DXGI_FORMAT_R8_TYPELESS,           // R8Unorm
            DXGI_FORMAT_B8G8R8A8_TYPELESS,     // BGRA8Unorm

            DXGI_FORMAT_BC1_TYPELESS,          // BC1
            DXGI_FORMAT_BC2_TYPELESS,          // BC2
            DXGI_FORMAT_BC3_TYPELESS,          // BC3
            DXGI_FORMAT_BC4_TYPELESS,          // BC4
            DXGI_FORMAT_BC5_TYPELESS,          // BC5
            DXGI_FORMAT_BC6H_TYPELESS,         // BC6H
            DXGI_FORMAT_BC7_TYPELESS,          // BC7

            DXGI_FORMAT_UNKNOWN,               // EAC_R11
            DXGI_FORMAT_UNKNOWN,               // EAC_RG11
            DXGI_FORMAT_UNKNOWN,               // ETC2
            DXGI_FORMAT_UNKNOWN,               // ASTC_4x4
        };
        RN_MATCH_ENUM_AND_ARRAY(TYPELESS_TEXTURE_FORMATS, TextureFormat)
    }

    DXGI_FORMAT ToDXGIFormat(DepthFormat format)
    {
        return DEPTH_FORMATS[int(format)];
    }

    DXGI_FORMAT ToDXGIFormat(RenderTargetFormat format)
    {
        return RENDER_TARGET_FORMATS[int(format)];
    }

    DXGI_FORMAT ToDXGIFormat(TextureFormat format)
    {
        return TYPED_TEXTURE_FORMATS[int(format)];
    }

    DXGI_FORMAT ToTypelessDXGIFormat(TextureFormat format)
    {
        return TYPELESS_TEXTURE_FORMATS[int(format)];
    }

    TextureFormat DXGIToTextureFormat(DXGI_FORMAT dxgiFormat)
    {
        TextureFormat format = TextureFormat::None;
        switch (dxgiFormat)
        {
        case DXGI_FORMAT_R8G8B8A8_TYPELESS:
        case DXGI_FORMAT_R8G8B8A8_UNORM:
            format = TextureFormat::RGBA8Unorm;
            break;
        case DXGI_FORMAT_R16G16B16A16_TYPELESS:
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
            format = TextureFormat::RGBA16Float;
            break;
        case DXGI_FORMAT_R16G16_TYPELESS:
        case DXGI_FORMAT_R16G16_FLOAT:
            format = TextureFormat::RG16Float;
            break;
        case DXGI_FORMAT_R11G11B10_FLOAT:
            format = TextureFormat::RG11B10Float;
            break;
        case DXGI_FORMAT_R32_TYPELESS:         // This can technically also be R32Uint.
        case DXGI_FORMAT_R32_FLOAT:
            format = TextureFormat::R32Float;
            break;
        case DXGI_FORMAT_R16_TYPELESS:         // R16Unorm
        case DXGI_FORMAT_R16_UNORM:
            format = TextureFormat::R16Unorm;
            break;
        case DXGI_FORMAT_R10G10B10A2_TYPELESS:
        case DXGI_FORMAT_R10G10B10A2_UNORM:
            format = TextureFormat::RGB10A2Unorm;
            break;
        case DXGI_FORMAT_R32_UINT:
            format = TextureFormat::R32Uint;
            break;
        case DXGI_FORMAT_R8_UNORM:
        case DXGI_FORMAT_R8_TYPELESS:
            format = TextureFormat::R8Unorm;
            break;

        case DXGI_FORMAT_BC1_TYPELESS:
        case DXGI_FORMAT_BC1_UNORM:
            format = TextureFormat::BC1;
            break;
        case DXGI_FORMAT_BC2_TYPELESS:
        case DXGI_FORMAT_BC2_UNORM:
            format = TextureFormat::BC2;
            break;
        case DXGI_FORMAT_BC3_TYPELESS:
        case DXGI_FORMAT_BC3_UNORM:
            format = TextureFormat::BC3;
            break;
        case DXGI_FORMAT_BC4_TYPELESS:
        case DXGI_FORMAT_BC4_UNORM:
            format = TextureFormat::BC4;
            break;
        case DXGI_FORMAT_BC5_TYPELESS:
        case DXGI_FORMAT_BC5_UNORM:
            format = TextureFormat::BC5;
            break;
        case DXGI_FORMAT_BC6H_TYPELESS:
        case DXGI_FORMAT_BC6H_SF16:
            format = TextureFormat::BC6H;
            break;
        case DXGI_FORMAT_BC7_TYPELESS:
        case DXGI_FORMAT_BC7_UNORM:
            format = TextureFormat::BC7;
            break;
        case DXGI_FORMAT_UNKNOWN:
        default:
            break;
        }

        return format;
    }
}