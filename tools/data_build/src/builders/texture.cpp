#pragma warning(push, 1)
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/sdf/assetPath.h>
#include "rn/texture.h"
#pragma warning(pop)

#include "build.hpp"
#include "usd.hpp"
#include "texture.hpp"
#include "texture_gen.hpp"
#include "luagen/schema.hpp"

#include "encoder/basisu_enc.h"
#include "encoder/basisu_comp.h"

#include <filesystem>

namespace rn
{
    struct Texture
    {
        std::string sourceImage;
        data::schema::TextureUsage usage;
    };

    constexpr const std::string_view SUPPORTED_IMAGE_TYPES[] = 
    {
        "png",
        "tga",
    };

    bool IsValidImageFileFormat(std::string_view path)
    {
        std::string_view ext = path.substr(path.find_last_of('.') + 1);

        bool validFormat = false;
        for (std::string_view supportedExt : SUPPORTED_IMAGE_TYPES)
        {
            if (supportedExt == ext)
            {
                validFormat = true;
                break;
            }
        }

        return validFormat;
    }

    bool IsValidImageFileFormat(std::string_view file, const pxr::UsdAttribute& attr)
    {
        auto str = Value<pxr::SdfAssetPath>(attr);
        return IsValidImageFileFormat(str.GetAssetPath());
    }

    constexpr std::string_view TEXTURE_USAGE_NAMES[] = {
        "Diffuse",
        "Normal",
        "Control",
        "UI",
        "LUT",
        "Heightmap",
    };
    RN_MATCH_ENUM_AND_ARRAY(TEXTURE_USAGE_NAMES, data::schema::TextureUsage);

    bool IsValidUsageValue(std::string_view usage)
    {
        bool validUsage = false;
        for (int i = 0; i < CountOf<data::schema::TextureUsage>(); ++i)
        {
            if (usage == TEXTURE_USAGE_NAMES[i])
            {
                validUsage = true;
                break;
            }
        }

        return validUsage;
    }

    bool IsValidUsageValue(std::string_view file, const pxr::UsdAttribute& attr)
    {
        auto str = Value<pxr::TfToken>(attr);
        return IsValidUsageValue(str.GetString());
    }

    

    data::schema::TextureUsage UsageFromString(std::string_view usage)
    {
        data::schema::TextureUsage outUsage = data::schema::TextureUsage::Count;
        for (int i = 0; i < CountOf<data::schema::TextureUsage>(); ++i)
        {
            if (usage == TEXTURE_USAGE_NAMES[i])
            {
                outUsage = data::schema::TextureUsage(i);
                break;
            }
        }

        return outUsage;
    }

    const PrimSchema TEXTURE_PRIM_SCHEMA = {
        .requiredProperties = {
            { .name = "file", .fnIsValidPropertyValue = IsValidImageFileFormat },
            { .name = "usage", .fnIsValidPropertyValue = IsValidUsageValue }
        },
    };

    int CompressAndBuildAsset(std::string_view file, const DataBuildOptions& options, const Texture& inputs, Vector<std::string>& outFiles)
    {
        std::filesystem::path buildFilePath = file;
        std::filesystem::path relBuildFileDirectory = buildFilePath.relative_path().parent_path();
        std::filesystem::path sourceImagePath = relBuildFileDirectory / inputs.sourceImage;
        outFiles.push_back(sourceImagePath.string());

        basisu::image img;
        if (!basisu::load_image(sourceImagePath.string(), img))
        {
            BuildError(file) << "Failed to load source image: '" << sourceImagePath << "'" << std::endl;
            return 1;
        }

        struct BasisScope
        {
            BasisScope()
            {
                basisu::basisu_encoder_init();
            }

            ~BasisScope()
            {
                basisu::basisu_encoder_deinit();
            }
        };

        BasisScope basisScope;
        basisu::basis_compressor_params params;
        params.m_source_images.push_back(img);
        params.m_uastc = true;

        switch (inputs.usage)
        {
        case data::schema::TextureUsage::Diffuse:
            // -mipmap
            params.m_mip_gen = true;
        break;
        case data::schema::TextureUsage::Normal:
            // -mipmap
            params.m_mip_gen = true;

            // -normal_map
            params.m_perceptual = false;
            params.m_mip_srgb = false;
            params.m_no_selector_rdo = true;
            params.m_no_endpoint_rdo = true;

            // -separate_rg_to_color_alpha
            params.m_swizzle[0] = 0;
            params.m_swizzle[1] = 0;
            params.m_swizzle[2] = 0;
            params.m_swizzle[3] = 1;
        break;
        case data::schema::TextureUsage::Control:
            // -mipmap
            params.m_mip_gen = true;

            // -linear
            params.m_perceptual = false;
            
            // -mip-linear
            params.m_mip_srgb = false;
        break;
        case data::schema::TextureUsage::UI:
        break;
        case data::schema::TextureUsage::LUT:
             // -linear
            params.m_perceptual = false;
        break;
        case data::schema::TextureUsage::Heightmap:
             // -linear
            params.m_perceptual = false;
        break;
        default:
            break;
        }

        basisu::job_pool jp(1);
        params.m_pJob_pool = &jp;
        params.m_multithreading = false;

        basisu::basis_compressor compressor;
        if (!compressor.init(params))
        {
            BuildError(file) << "Failed to initialize basis compressor" << std::endl;
            return 1;
        }

        if(compressor.process() != basisu::basis_compressor::cECSuccess)
        {
            BuildError(file) << "Failed to compress basis data" << std::endl;
            return 1;
        }
        
        // I know we're copying here, but we'd need to copy this data over anyway for serialization :(
        basisu::uint8_vec basisData = compressor.get_output_basis_file();

        using namespace data;
        schema::Texture outTexture = {
            .dataFormat = schema::TextureDataFormat::Basis,
            .usage = inputs.usage,
            .data = basisData
        };

        uint64_t serializedSize = schema::Texture::SerializedSize(outTexture);
        Span<uint8_t> outData = { static_cast<uint8_t*>(ScopedAlloc(serializedSize, CACHE_LINE_TARGET_SIZE)), serializedSize };
        rn::Serialize<schema::Texture>(outData, outTexture);

        return WriteAssetToDisk(file, ".texture", options, outData, {}, outFiles);
    }

    int ProcessUsdTexture(std::string_view file, const DataBuildOptions& options, const pxr::UsdPrim& prim, Vector<std::string>& outFiles)
    {
        if (!ValidatePrim(file, prim, TEXTURE_PRIM_SCHEMA))
        {
            return false;
        }

        pxr::RnTexture texturePrim(prim);
        Texture inputs = {
            .sourceImage = Value<pxr::SdfAssetPath>(texturePrim.GetFileAttr()).GetResolvedPath(),
            .usage = UsageFromString(Value<pxr::TfToken>(texturePrim.GetUsageAttr()).GetString())
        };

        return CompressAndBuildAsset(file, options, inputs, outFiles);
    }
}