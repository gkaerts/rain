#include "data/material.hpp"
#include "data/material_shader.hpp"
#include "data/texture.hpp"

#include "asset/registry.hpp"

#include "material_gen.hpp"
#include "luagen/schema.hpp"

#include "common/log/log.hpp"

namespace rn::data
{
    RN_DEFINE_LOG_CATEGORY(Data);

    MaterialBuilder::MaterialBuilder()
    {}

    MaterialData MaterialBuilder::Build(const asset::AssetBuildDesc& desc)
    {
        schema::Material asset = rn::Deserialize<schema::Material>(desc.data,
            [](size_t size) { 
                return ScopedAlloc(size, CACHE_LINE_TARGET_SIZE); 
            });

        const asset::Registry* registry = desc.registry;

        MaterialShader shader = MaterialShader(desc.dependencies[asset.materialShader.identifier]);
        const MaterialShaderData* shaderData = registry->Resolve<MaterialShader, MaterialShaderData>(shader);

        uint8_t* uniformData = TrackedNewArray<uint8_t>(MemoryCategory::Data, shaderData->uniformBufferSize);
        std::memset(uniformData, 0, shaderData->uniformBufferSize);

        for (const MaterialShaderParameter& shaderParam : shaderData->parameters)
        {
            uint8_t* paramPtr = uniformData + shaderParam.offsetInMaterialData;
            switch (shaderParam.type)
            {
            case MaterialShaderParameterType::Texture:
            {
                Texture textureAsset = shaderParam.pTexture.defaultValue;
                for (const schema::TextureMaterialParameter& param : asset.textureParams)
                {
                    if (shaderParam.name == param.name)
                    {
                        textureAsset = Texture(desc.dependencies[param.value.identifier]);
                        break;
                    }
                }

                const TextureData* textureData = desc.registry->Resolve<Texture, TextureData>(textureAsset);
                if (textureData->type == shaderParam.pTexture.type)
                {
                    if (textureData->type == TextureType::Texture2D)
                    {
                        std::memcpy(paramPtr,
                            &textureData->texture2D.rhiView,
                            sizeof(textureData->texture2D.rhiView));
                    }
                    else
                    {
                        std::memcpy(paramPtr,
                            &textureData->texture3D.rhiView,
                            sizeof(textureData->texture3D.rhiView));
                    }
                }
                else
                {
                    LogWarning(LogCategory::Data, "Texture type mismatch for parameter \"{}\" in asset \"{}\". Shading results might be incorrect.",
                        shaderParam.name,
                        desc.identifier);
                }
            }
            break;
            case MaterialShaderParameterType::FloatVec:
            {
                for (const schema::FloatVecMaterialParameter& param : asset.floatVecParams)
                {
                    if (shaderParam.name == param.name)
                    {
                        std::memcpy(paramPtr,
                            param.value.data(),
                            std::min(param.value.size(), size_t(shaderParam.pFloatVec.dimension)) * sizeof(float));
                        break;
                    }
                }
            }
            break;
            case MaterialShaderParameterType::UintVec:
            {
                for (const schema::UintVecMaterialParameter& param : asset.uintVecParams)
                {
                    if (shaderParam.name == param.name)
                    {
                        std::memcpy(paramPtr,
                            param.value.data(),
                            std::min(param.value.size(), size_t(shaderParam.pUintVec.dimension)) * sizeof(uint32_t));
                        break;
                    }
                }
            }
            break;
            case MaterialShaderParameterType::IntVec:
                for (const schema::IntVecMaterialParameter& param : asset.intVecParams)
                {
                    if (shaderParam.name == param.name)
                    {
                        std::memcpy(paramPtr,
                            param.value.data(),
                            std::min(param.value.size(), size_t(shaderParam.pIntVec.dimension)) * sizeof(int32_t));
                        break;
                    }
                }
            break;
            
            default:
                RN_ASSERT(false);
                break;
            }
        }

        return {
            .shader = shader,
            .uniformData = {
                uniformData,
                shaderData->uniformBufferSize
            }
        };
    }

    void MaterialBuilder::Destroy(MaterialData& data)
    {
        if (data.uniformData.data())
        {
            TrackedDeleteArray(data.uniformData.data());
        }

        data = {};
    }

    void MaterialBuilder::Finalize()
    {
        
    }

}