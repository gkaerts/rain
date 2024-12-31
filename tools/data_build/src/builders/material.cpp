#pragma warning(push, 1)
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/sdf/assetPath.h>
#include "rn/rainMaterialAPI.h"
#include "rn/materialParamFloatVec.h"
#include "rn/materialParamUintVec.h"
#include "rn/materialParamIntVec.h"
#include "rn/materialParamTexture.h"
#include "rn/texture.h"
#pragma warning(pop)

#include "build.hpp"
#include "usd.hpp"
#include "material.hpp"
#include "material_gen.hpp"

#include "luagen/schema.hpp"

namespace rn
{
    const PrimSchema MATERIAL_PRIM_SCHEMA = {
        .requiredProperties = {
            { .name = "rn:shader", .fnIsValidPropertyValue = IsNonEmptyAssetPath },
            { 
                .name = "rn:parameters", 
                .fnIsValidRelationship = RelatedPrimIsA<
                    pxr::RnMaterialParamFloatVec,
                    pxr::RnMaterialParamUintVec,
                    pxr::RnMaterialParamIntVec,
                    pxr::RnMaterialParamTexture> 
            }
        }
    };

    const PrimSchema TEXTURE_PARAM_PRIM_SCHEMA = {
        .requiredProperties = {
            { .name = "dimension" },
            { .name = "value", .fnIsValidRelationship = RelatedPrimIsA<pxr::RnTexture> }
        }
    }; 

    const PrimSchema FLOAT_VEC_PARAM_PRIM_SCHEMA = {
        .requiredProperties = {
            { .name = "dimension" },
            { .name = "value" }
        }
    }; 

    const PrimSchema UINT_VEC_PARAM_PRIM_SCHEMA = {
        .requiredProperties = {
            { .name = "dimension" },
            { .name = "value" }
        }
    }; 

    const PrimSchema INT_VEC_PARAM_PRIM_SCHEMA = {
        .requiredProperties = {
            { .name = "dimension" },
            { .name = "value" }
        }
    }; 

    int ProcessUsdMaterial(std::string_view file, const DataBuildOptions& options, const pxr::UsdPrim& prim, Vector<std::string>& outFiles)
    {
        MemoryScope SCOPE;
        data::schema::Material material{};

        if (!ValidatePrim(file, prim, MATERIAL_PRIM_SCHEMA))
        {
            return false;
        }

        auto PushReference = [](ScopedVector<std::string_view>& references, const std::string_view& ref) -> uint32_t
        {
            uint32_t idx = uint32_t(references.size());
            references.push_back(ref);
            return idx;
        };

        pxr::RnRainMaterialAPI matAPI = pxr::RnRainMaterialAPI(prim);

        ScopedVector<std::string_view> references;
        std::string shaderPath = MakeAssetReferencePath(file,
                Value<pxr::SdfAssetPath>(matAPI.GetShaderAttr()), 
                "material_shader").string();

        material.materialShader = {
            .identifier = PushReference(references, shaderPath)
        };

        pxr::UsdStageWeakPtr stage = prim.GetPrim().GetStage();
        pxr::SdfPathVector paramPaths = ResolveRelationTargets(matAPI.GetParametersRel());

        ScopedVector<data::schema::TextureMaterialParameter> textureParams;
        textureParams.reserve(paramPaths.size());
        ScopedVector<data::schema::FloatVecMaterialParameter> floatVecParams;
        floatVecParams.reserve(paramPaths.size());
        ScopedVector<data::schema::UintVecMaterialParameter> uintVecParams;
        uintVecParams.reserve(paramPaths.size());
        ScopedVector<data::schema::IntVecMaterialParameter> intVecParams;
        intVecParams.reserve(paramPaths.size());
       
        ScopedVector<std::string> assetPaths;
        assetPaths.reserve(paramPaths.size());

        ScopedVector<std::string> paramNames;
        paramNames.reserve(paramPaths.size());

        for (const pxr::SdfPath& paramPath : paramPaths)
        {
            pxr::UsdPrim paramPrim = stage->GetPrimAtPath(paramPath);

            paramNames.push_back(paramPrim.GetName().GetString());

            if (paramPrim.IsA<pxr::RnMaterialParamFloatVec>())
            {
                pxr::RnMaterialParamFloatVec param = pxr::RnMaterialParamFloatVec(paramPrim);
                auto values = Value<pxr::VtArray<float>>(param.GetValueAttr());

                float* valuesPtr = ScopedNewArray<float>(SCOPE, values.size());
                std::memcpy(valuesPtr, values.data(), values.size() * sizeof(float));
                floatVecParams.push_back({
                    .name = paramNames.back(),
                    .value = { valuesPtr, values.size() }
                });
            }
            else if (paramPrim.IsA<pxr::RnMaterialParamUintVec>())
            {
                pxr::RnMaterialParamUintVec param = pxr::RnMaterialParamUintVec(paramPrim);
                auto values = Value<pxr::VtArray<uint32_t>>(param.GetValueAttr());

                uint32_t* valuesPtr = ScopedNewArray<uint32_t>(SCOPE, values.size());
                std::memcpy(valuesPtr, values.data(), values.size() * sizeof(uint32_t));
                uintVecParams.push_back({
                    .name = paramNames.back(),
                    .value = { valuesPtr, values.size() }
                });
            }
            else if (paramPrim.IsA<pxr::RnMaterialParamIntVec>())
            {
                pxr::RnMaterialParamIntVec param = pxr::RnMaterialParamIntVec(paramPrim);
                auto values = Value<pxr::VtArray<int32_t>>(param.GetValueAttr());

                int32_t* valuesPtr = ScopedNewArray<int32_t>(SCOPE, values.size());
                std::memcpy(valuesPtr, values.data(), values.size() * sizeof(int32_t));
                intVecParams.push_back({
                    .name = paramNames.back(),
                    .value = { valuesPtr, values.size() }
                });
            }
            if (paramPrim.IsA<pxr::RnMaterialParamTexture>())
            {
                pxr::RnMaterialParamTexture param = pxr::RnMaterialParamTexture(paramPrim);
                pxr::SdfPathVector valuePaths = ResolveRelationTargets(param.GetValueRel());
                if (valuePaths.empty())
                {
                    BuildError(file) << "No texture asset specified for texture material parameter." << std::endl;
                    return false;
                }

                pxr::UsdPrim texturePrim = stage->GetPrimAtPath(valuePaths[0]);
                pxr::VtValue assetId = texturePrim.GetAssetInfo()["identifier"];

                std::filesystem::path texturePath = MakeAssetReferencePath(file,
                    assetId.Get<pxr::SdfAssetPath>(), 
                    "texture");

                assetPaths.emplace_back(texturePath.string());
                textureParams.push_back({
                    .name = paramNames.back(),
                    .value =
                    {
                        .identifier = PushReference(references, assetPaths.back())
                    }});
            }
        }

        material.textureParams = textureParams;
        material.floatVecParams = floatVecParams;
        material.uintVecParams = uintVecParams;
        material.intVecParams = intVecParams;

        uint64_t outSize = data::schema::Material::SerializedSize(material);
        Span<uint8_t> assetData = { static_cast<uint8_t*>(ScopedAlloc(outSize, CACHE_LINE_TARGET_SIZE)), outSize };
        rn::Serialize(assetData, material);

        return WriteAssetToDisk(file, ".material", options, assetData, references, outFiles);
    }
}