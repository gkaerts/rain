#include "build.hpp"
#include "data/schema/texture_generated.h"
#include "encoder/basisu_enc.h"
#include "encoder/basisu_comp.h"

#include "flatbuffers/flatbuffers.h"
#include <filesystem>

namespace rn
{
    struct Texture
    {
        std::filesystem::path sourceImage;
        schema::render::Usage usage;
    };

    constexpr const std::string_view SUPPORTED_IMAGE_TYPES[] = 
    {
        "png",
        "tga",
    };

    bool IsValidImageFileFormat(std::string_view file, toml::node& node)
    {
        std::string_view path = node.value_or(""sv);
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

    bool IsValidUsageValue(std::string_view file, toml::node& node)
    {
        std::string_view usage = node.value_or(""sv);

        bool validUsage = false;
        for (int i = 0; i < int(schema::render::Usage::Count); ++i)
        {
            if (usage == schema::render::EnumNamesUsage()[i])
            {
                validUsage = true;
                break;
            }
        }

        return validUsage;
    }

    schema::render::Usage UsageFromString(std::string_view usage)
    {
        schema::render::Usage outUsage = schema::render::Usage::Count;
        for (int i = 0; i < int(schema::render::Usage::Count); ++i)
        {
            if (usage == schema::render::EnumNamesUsage()[i])
            {
                outUsage = schema::render::Usage(i);
                break;
            }
        }

        return outUsage;
    }

    const TableSchema TEXTURE_TABLE_SCHEMA = {
        .name = "texture"sv,
        .requiredFields = {
            { .name = "source_image"sv, .type = toml::node_type::string, .fnIsValidValue = IsValidImageFileFormat },
            { .name = "usage"sv, .type = toml::node_type::string, .fnIsValidValue = IsValidUsageValue }
        },
    };

    int BuildTextureAsset(std::string_view file, toml::parse_result& root, const DataBuildOptions& options, Vector<std::string>& outFiles)
    {
        if (!root["texture"])
        {
            BuildError(file) << "No [texture] table found" << std::endl;
            return 1;
        }

        auto texture = root["texture"];
        if (!ValidateTable(file, *texture.node(), TEXTURE_TABLE_SCHEMA))
        {
            return 1;
        }

        Texture inputs = {
            .sourceImage = texture["source_image"].value_or(""sv),
            .usage = UsageFromString(texture["usage"].value_or(""sv))
        };

        std::filesystem::path buildFilePath = file;
        std::filesystem::path relBuildFileDirectory = buildFilePath.relative_path().parent_path();
        std::filesystem::path sourceImagePath = relBuildFileDirectory / inputs.sourceImage;

        basisu::image img;
        if (!basisu::load_image(sourceImagePath.string(), img))
        {
            BuildError(file) << "Failed to load source image: '" << inputs.sourceImage << "'" << std::endl;
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
        case schema::render::Usage::Diffuse:
            // -mipmap
            params.m_mip_gen = true;
        break;
        case schema::render::Usage::Normal:
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
        case schema::render::Usage::Control:
            // -mipmap
            params.m_mip_gen = true;

            // -linear
            params.m_perceptual = false;
            
            // -mip-linear
            params.m_mip_srgb = false;
        break;
        case schema::render::Usage::UI:
        break;
        case schema::render::Usage::LUT:
             // -linear
            params.m_perceptual = false;
        break;
        case schema::render::Usage::Heightmap:
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
        
        const basisu::uint8_vec& basisData = compressor.get_output_basis_file();

        flatbuffers::FlatBufferBuilder fbb;
        auto dataVec = fbb.CreateVector(reinterpret_cast<const uint8_t*>(basisData.data()), basisData.size());
        auto fbRoot = schema::render::CreateTexture(
            fbb,
            schema::render::TextureDataFormat::Basis,
            inputs.usage,
            dataVec);
        fbb.Finish(fbRoot, schema::render::TextureIdentifier());
        
        outFiles.push_back(sourceImagePath.string());
        return WriteAssetToDisk(file, schema::render::TextureExtension(), options, fbb.GetBufferSpan(), {}, outFiles);
    }
}