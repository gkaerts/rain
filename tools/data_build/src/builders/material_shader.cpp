#pragma warning(push, 1)
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/sdf/assetPath.h>
#include "rn/materialShader.h"
#include "rn/materialShaderRenderPass.h"
#include "rn/vertexRasterPass.h"
#include "rn/meshRasterPass.h"
#include "rn/rayTracingPass.h"
#include "rn/materialShaderParam.h"
#include "rn/materialShaderParamFloatVec.h"
#include "rn/materialShaderParamIntVec.h"
#include "rn/materialShaderParamUintVec.h"
#include "rn/materialShaderParamTexture.h"
#include "rn/materialShaderParamGroup.h"
#pragma warning(pop)

#include "build.hpp"
#include "usd.hpp"
#include "material_shader.hpp"

#include "common/math/math.hpp"
#include "common/memory/vector.hpp"
#include "common/memory/span.hpp"
#include "common/memory/hash_map.hpp"

#include "data/material_shader.hpp"
#include "data/texture.hpp"
#include "material_shader_gen.hpp"
#include "luagen/schema.hpp"

#include "shader.hpp"
#include "d3d12shader.h"
#include <string>

using namespace Microsoft::WRL;

namespace rn
{
    constexpr const uint32_t INVALID_ENTRYPOINT = 0xFFFFFFFF;
    struct VertexRasterPass
    {
        data::MaterialRenderPass renderPass;
        uint32_t vertexShader = INVALID_ENTRYPOINT;
        uint32_t pixelShader = INVALID_ENTRYPOINT;
    };

    struct MeshRasterPass
    {
        data::MaterialRenderPass renderPass;
        uint32_t ampShader = INVALID_ENTRYPOINT;
        uint32_t meshShader = INVALID_ENTRYPOINT;
        uint32_t pixelShader = INVALID_ENTRYPOINT;
    };

    struct RayTracingPass
    {
        data::MaterialRenderPass renderPass;
        std::string hitGroupName;
        std::string closestHitShader;
        std::string anyHitShader;
    };

    template <typename T>
    struct TypedNumericalParameter
    {
        static const size_t CAPACITY = 16;
        T defaultValue[CAPACITY];
        T min[CAPACITY];
        T max[CAPACITY];

        uint32_t dimension;
    };
    struct Parameter
    {
        data::MaterialShaderParameterType type;
        std::string name;
        std::string param;
        uint32_t offsetInBuffer;

        std::string texture = "";
        union
        {
            data::TextureType textureType;
            TypedNumericalParameter<float> pFloat;
            TypedNumericalParameter<uint> pUint;
            TypedNumericalParameter<int> pInt;
        };
        
    };

    struct ParameterGroup
    {
        std::string name;
        Vector<Parameter> parameters;
    };

    struct MaterialShader
    {
        std::wstring source;
        Vector<std::wstring> defines;
        Vector<std::wstring> includeDirs;

        Vector<VertexRasterPass> vertexRasterPasses;
        Vector<MeshRasterPass> meshRasterPasses;
        Vector<RayTracingPass> rayTracingPasses;
        Vector<ParameterGroup> parameterGroups;
    };

    std::wstring ToWString(const std::string& str)
    {
        return std::filesystem::path(str).wstring();
    }

    constexpr const std::string_view MATERIAL_RENDER_PASS_NAMES[] = 
    {
        "depth",
        "gbuffer",
        "vbuffer",
        "forward",
        "ddgi",
        "reflections",
    };
    RN_MATCH_ENUM_AND_ARRAY(MATERIAL_RENDER_PASS_NAMES, data::MaterialRenderPass);

    data::MaterialRenderPass ToRenderPass(std::string_view typeStr)
    {
        std::string paramStr = { typeStr.data(), typeStr.size() };
        std::transform(paramStr.cbegin(), paramStr.cend(), paramStr.begin(), [](char c) { return std::tolower(c); });
        for (int i = 0; i < CountOf<data::MaterialRenderPass>(); ++i)
        {
            if (paramStr == MATERIAL_RENDER_PASS_NAMES[i])
            {
                return data::MaterialRenderPass(i);
            }
        }

        return data::MaterialRenderPass::Count;
    }

    bool IsValidRenderPassName(const DataBuildContext& ctxt, const pxr::UsdAttribute& prop)
    {
        std::string paramStr = Value<pxr::TfToken>(prop).GetString();
        std::transform(paramStr.cbegin(), paramStr.cend(), paramStr.begin(), [](char c) { return std::tolower(c); });

        for (int i = 0; i < CountOf<data::MaterialRenderPass>(); ++i)
        {
            if (paramStr == MATERIAL_RENDER_PASS_NAMES[i])
            {
                return true;
            }
        }

        return false;
    }

    const pxr::TfToken TOKEN_2D = pxr::TfToken("2D");
    const pxr::TfToken TOKEN_3D = pxr::TfToken("3D");

    bool IsValidTextureDimension(const DataBuildContext& ctxt, const pxr::UsdAttribute& prop)
    {
        pxr::TfToken dim = Value<pxr::TfToken>(prop);
        return dim == TOKEN_2D || dim == TOKEN_3D;
    }

    bool IsValidVecDimension(const DataBuildContext& ctxt, const pxr::UsdAttribute& prop)
    {
        return Value<uint32_t>(prop) > 0 && Value<uint32_t>(prop) <= 16;
    }

    const PrimSchema MATERIAL_SHADER_PRIM_SCHEMA = {
        .requiredProperties = {
            { .name = "source", .fnIsValidPropertyValue = IsNonEmptyAssetPath },
            { .name = "renderPasses", .fnIsValidRelationship = RelatedPrimIsA<pxr::RnVertexRasterPass, pxr::RnMeshRasterPass, pxr::RnRayTracingPass> },
            { .name = "paramGroups", .fnIsValidRelationship = RelatedPrimIsA<pxr::RnMaterialShaderParamGroup> }
        },

        .optionalProperties = {
            { .name = "defines" },
            { .name = "includeDirs" }
        }
    };

    const PrimSchema VERTEX_RASTER_PASS_PRIM_SCHEMA = {
        .requiredProperties = {
            { .name = "engineRenderPass", .fnIsValidPropertyValue = IsValidRenderPassName },
            { .name = "vertexShader", .fnIsValidPropertyValue = IsNonEmptyString },
        },

        .optionalProperties = {
            { .name = "pixelShader" },
        }
    };

    const PrimSchema MESH_RASTER_PASS_PRIM_SCHEMA = {
        .requiredProperties = {
            { .name = "engineRenderPass", .fnIsValidPropertyValue = IsValidRenderPassName },
            { .name = "meshShader", .fnIsValidPropertyValue = IsNonEmptyString},
        },

        .optionalProperties = {
            { .name = "pixelShader" },
            { .name = "amplificationShader" },
        }
    };

    const PrimSchema RAY_TRACING_PASS_PRIM_SCHEMA = {
        .requiredProperties = {
            { .name = "engineRenderPass", .fnIsValidPropertyValue = IsValidRenderPassName },
            { .name = "hitGroupNameExport", .fnIsValidPropertyValue = IsNonEmptyString },
            { .name = "closestHitExport", .fnIsValidPropertyValue = IsNonEmptyString },
        },

        .optionalProperties = {
            { .name = "anyHitExport" },
        }
    };

    const PrimSchema PARAMETER_GROUP_PRIM_SCHEMA = {
        .requiredProperties = {
            { 
                .name = "parameters", 
                .fnIsValidRelationship = RelatedPrimIsA<
                    pxr::RnMaterialShaderParamFloatVec,
                    pxr::RnMaterialShaderParamUintVec,
                    pxr::RnMaterialShaderParamIntVec,
                    pxr::RnMaterialShaderParamTexture>  
            }
        }
    };

    const PrimSchema TEXTURE_PARAM_PRIM_SCHEMA = {
        .requiredProperties = {
            { .name = "hlslParam", .fnIsValidPropertyValue = IsNonEmptyString },
            { .name = "dimension", .fnIsValidPropertyValue = IsValidTextureDimension },
            { .name = "defaultValue", .fnIsValidPropertyValue = IsNonEmptyAssetPath },
        },

    };

    const PrimSchema FLOAT_VEC_PARAM_PRIM_SCHEMA = {
        .requiredProperties = {
            { .name = "hlslParam", .fnIsValidPropertyValue = IsNonEmptyString },
            { .name = "dimension", .fnIsValidPropertyValue = IsValidVecDimension }
        },
        .optionalProperties = {
            { .name = "defaultValue" },
            { .name = "minValue" },
            { .name = "maxValue" }
        }
    };

    const PrimSchema INT_VEC_PARAM_PRIM_SCHEMA = {
        .requiredProperties = {
            { .name = "hlslParam", .fnIsValidPropertyValue = IsNonEmptyString },
            { .name = "dimension", .fnIsValidPropertyValue = IsValidVecDimension }
        },
        .optionalProperties = {
            { .name = "defaultValue" },
            { .name = "minValue" },
            { .name = "maxValue" }
        }
    };

    const PrimSchema UINT_VEC_PARAM_PRIM_SCHEMA = {
        .requiredProperties = {
            { .name = "hlslParam", .fnIsValidPropertyValue = IsNonEmptyString },
            { .name = "dimension", .fnIsValidPropertyValue = IsValidVecDimension }
        },
        .optionalProperties = {
            { .name = "defaultValue" },
            { .name = "minValue" },
            { .name = "maxValue" }
        }
    };

    bool ParseMaterialShader(const DataBuildContext& ctxt, MaterialShader& outShader, const pxr::UsdPrim& prim, Vector<std::string>& outFiles)
    {
        if (!ValidatePrim(ctxt, prim, MATERIAL_SHADER_PRIM_SCHEMA))
        {
            return false;
        }

        pxr::RnMaterialShader msPrim = pxr::RnMaterialShader(prim);

        pxr::SdfAssetPath source = Value<pxr::SdfAssetPath>(msPrim.GetSourceAttr());
        auto defines = Value<pxr::VtArray<std::string>>(msPrim.GetDefinesAttr());
        auto includeDirs = Value<pxr::VtArray<std::string>>(msPrim.GetIncludeDirsAttr());

        outShader.source = ToWString(source.GetAssetPath());

        std::filesystem::path relBuildFileDirectory = std::filesystem::path(ctxt.file).parent_path();
        for (const std::string& tok : defines)
        {
            outShader.defines.push_back(ToWString(tok));
        }

        for (const std::string& tok : includeDirs)
        {
            std::filesystem::path absInclude = relBuildFileDirectory / tok;
            outShader.includeDirs.push_back(absInclude.wstring());
        }

        outFiles.push_back(MakeRelativeTo(ctxt.file, outShader.source).string());
        return true;
    }

    uint32_t PushEntryPoint(Vector<EntryPoint>& entryPoints, ShaderTarget target, const std::wstring& name)
    {
        uint32_t idx = INVALID_ENTRYPOINT;
        if (!name.empty() || target == ShaderTarget::Library)
        {
            idx = uint32_t(entryPoints.size());
            entryPoints.push_back({
                .target = target,
                .name = name,
            });
        }

        return idx;
    }


    bool ParseRenderPasses(const DataBuildContext& ctxt, const pxr::RnMaterialShader& prim, MaterialShader& outShader, Vector<EntryPoint>& entryPoints, Vector<std::string>& outFiles)
    {
        pxr::UsdStageWeakPtr stage = prim.GetPrim().GetStage();

        pxr::SdfPathVector renderPassPaths = ResolveRelationTargets(prim.GetRenderPassesRel());
        if (renderPassPaths.empty())
        {
            BuildError(ctxt) << "USD: No render passes specified for RnMaterialShader (\"" << prim.GetPath() << "\".)" << std::endl;
            return false;
        }

        for (const pxr::SdfPath& path : renderPassPaths)
        {
            pxr::UsdPrim passPrim = stage->GetPrimAtPath(path);
            if (passPrim.IsA<pxr::RnVertexRasterPass>())
            {
                if (!ValidatePrim(ctxt, passPrim, VERTEX_RASTER_PASS_PRIM_SCHEMA))
                {
                    return false;
                }

                pxr::RnVertexRasterPass vertexPass(passPrim);
                outShader.vertexRasterPasses.push_back({
                    .renderPass =   ToRenderPass(Value<pxr::TfToken>(vertexPass.GetEngineRenderPassAttr()).GetString()),
                    .vertexShader = PushEntryPoint(entryPoints, ShaderTarget::Vertex,   ToWString(Value<pxr::TfToken>(vertexPass.GetVertexShaderAttr()).GetString())),
                    .pixelShader =  PushEntryPoint(entryPoints, ShaderTarget::Pixel,    ToWString(Value<pxr::TfToken>(vertexPass.GetPixelShaderAttr()).GetString())),
                });
            }
            else if (passPrim.IsA<pxr::RnMeshRasterPass>())
            {
                if (!ValidatePrim(ctxt, passPrim, MESH_RASTER_PASS_PRIM_SCHEMA))
                {
                    return false;
                }
                
                pxr::RnMeshRasterPass meshPass(passPrim);
                outShader.meshRasterPasses.push_back({
                    .renderPass =   ToRenderPass(Value<pxr::TfToken>(meshPass.GetEngineRenderPassAttr()).GetString()),
                    .ampShader =    PushEntryPoint(entryPoints, ShaderTarget::Amplification,    ToWString(Value<pxr::TfToken>(meshPass.GetAmplificationShaderAttr()).GetString())),
                    .meshShader =   PushEntryPoint(entryPoints, ShaderTarget::Mesh,             ToWString(Value<pxr::TfToken>(meshPass.GetMeshShaderAttr()).GetString())),
                    .pixelShader =  PushEntryPoint(entryPoints, ShaderTarget::Pixel,            ToWString(Value<pxr::TfToken>(meshPass.GetPixelShaderAttr()).GetString())),
                });
            }
            else if (passPrim.IsA<pxr::RnRayTracingPass>())
            {
                if (!ValidatePrim(ctxt, passPrim, RAY_TRACING_PASS_PRIM_SCHEMA))
                {
                    return false;
                }

                pxr::RnRayTracingPass rtPass(passPrim);
                outShader.rayTracingPasses.push_back({
                    .renderPass =   ToRenderPass(Value<pxr::TfToken>(rtPass.GetEngineRenderPassAttr()).GetString()),
                    .hitGroupName =     Value<pxr::TfToken>(rtPass.GetHitGroupNameExportAttr()).GetString(),
                    .closestHitShader = Value<pxr::TfToken>(rtPass.GetClosestHitExportAttr()).GetString(),
                    .anyHitShader =     Value<pxr::TfToken>(rtPass.GetAnyHitExportAttr()).GetString(),
                });
            }
            else
            {
                BuildError(ctxt) << "USD: Prim \"" << passPrim.GetPath() << "\" is not a valid render pass - RnMaterialShader (\"" << prim.GetPath() << "\".)" << std::endl;
                return false;
            }
        }

        return true;
    }

    bool ParseParameter(const DataBuildContext& ctxt, const pxr::UsdPrim& prim, Parameter& outParam)
    {
        pxr::UsdStageWeakPtr stage = prim.GetPrim().GetStage();
        if (prim.IsA<pxr::RnMaterialShaderParamFloatVec>())
        {
            if (!ValidatePrim(ctxt, prim, FLOAT_VEC_PARAM_PRIM_SCHEMA))
            {
                return false;
            }

            pxr::RnMaterialShaderParamFloatVec param(prim);
            auto defaultValues =    Value<pxr::VtArray<float>>(param.GetDefaultValueAttr());
            auto minValues =        Value<pxr::VtArray<float>>(param.GetMinValueAttr());
            auto maxValues =        Value<pxr::VtArray<float>>(param.GetMaxValueAttr());

            outParam = {
                .type = data::MaterialShaderParameterType::FloatVec,
                .name = prim.GetName().GetString(),
                .param = Value<pxr::TfToken>(param.GetHlslParamAttr()).GetString(),
                .pFloat = {
                    .dimension = size_t(Value<uint32_t>(param.GetDimensionAttr()))
                }
            };

            for (int idx = 0; float v : defaultValues)  { outParam.pFloat.defaultValue[idx++] = v; }   
            for (int idx = 0; float v : minValues)      { outParam.pFloat.min[idx++] = v; } 
            for (int idx = 0; float v : maxValues)      { outParam.pFloat.max[idx++] = v; } 
        }
        else if (prim.IsA<pxr::RnMaterialShaderParamUintVec>())
        {
            if (!ValidatePrim(ctxt, prim, UINT_VEC_PARAM_PRIM_SCHEMA))
            {
                return false;
            }

            pxr::RnMaterialShaderParamUintVec param(prim);
            auto defaultValues =    Value<pxr::VtArray<uint32_t>>(param.GetDefaultValueAttr());
            auto minValues =        Value<pxr::VtArray<uint32_t>>(param.GetMinValueAttr());
            auto maxValues =        Value<pxr::VtArray<uint32_t>>(param.GetMaxValueAttr());

            outParam = {
                .type = data::MaterialShaderParameterType::UintVec,
                .name = prim.GetName().GetString(),
                .param = Value<pxr::TfToken>(param.GetHlslParamAttr()).GetString(),
                .pUint = {
                    .dimension = size_t(Value<uint32_t>(param.GetDimensionAttr()))
                }
            };

            for (int idx = 0; uint32_t v : defaultValues)  { outParam.pUint.defaultValue[idx++] = v; }   
            for (int idx = 0; uint32_t v : minValues)      { outParam.pUint.min[idx++] = v; } 
            for (int idx = 0; uint32_t v : maxValues)      { outParam.pUint.max[idx++] = v; } 
        }
        else if (prim.IsA<pxr::RnMaterialShaderParamIntVec>())
        {
            if (!ValidatePrim(ctxt, prim, INT_VEC_PARAM_PRIM_SCHEMA))
            {
                return false;
            }

            pxr::RnMaterialShaderParamIntVec param(prim);
            auto defaultValues =    Value<pxr::VtArray<int32_t>>(param.GetDefaultValueAttr());
            auto minValues =        Value<pxr::VtArray<int32_t>>(param.GetMinValueAttr());
            auto maxValues =        Value<pxr::VtArray<int32_t>>(param.GetMaxValueAttr());

            outParam = {
                .type = data::MaterialShaderParameterType::IntVec,
                .name = prim.GetName().GetString(),
                .param = Value<pxr::TfToken>(param.GetHlslParamAttr()).GetString(),
                .pUint = {
                    .dimension = size_t(Value<uint32_t>(param.GetDimensionAttr()))
                }
            };

            for (int idx = 0; int32_t v : defaultValues)  { outParam.pInt.defaultValue[idx++] = v; }   
            for (int idx = 0; int32_t v : minValues)      { outParam.pInt.min[idx++] = v; } 
            for (int idx = 0; int32_t v : maxValues)      { outParam.pInt.max[idx++] = v; } 
        }
        else if (prim.IsA<pxr::RnMaterialShaderParamTexture>())
        {
            if (!ValidatePrim(ctxt, prim, TEXTURE_PARAM_PRIM_SCHEMA))
            {
                return false;
            }

            pxr::RnMaterialShaderParamTexture param(prim);
            
            pxr::SdfPathVector valuePaths = ResolveRelationTargets(param.GetDefaultValueRel());
            if (valuePaths.empty())
            {
                BuildError(ctxt) << "No texture asset specified for texture material shader parameter." << std::endl;
                return false;
            }

            pxr::UsdPrim texturePrim = stage->GetPrimAtPath(valuePaths[0]);
            pxr::VtValue assetId = texturePrim.GetAssetInfo()["identifier"];
            if (!assetId.IsHolding<pxr::SdfAssetPath>())
            {
                BuildError(ctxt) << "Material shader texture not pointing to a valid asset." << std::endl;
                return false;
            }

            std::filesystem::path texturePath = MakeAssetReferencePath(ctxt,
                assetId.Get<pxr::SdfAssetPath>(), 
                "texture");

            pxr::TfToken dimension = Value<pxr::TfToken>(param.GetDimensionAttr());
            outParam = {
                .type = data::MaterialShaderParameterType::Texture,
                .name = prim.GetName().GetString(),
                .param = Value<pxr::TfToken>(param.GetHlslParamAttr()).GetString(),
                .texture = texturePath.string(),
                .textureType = dimension == TOKEN_3D ?
                    data::TextureType::Texture3D :
                    data::TextureType::Texture2D
            };
        }
        else
        {
            BuildError(ctxt) << "USD: Prim \"""\" is not a valid material parameter type." << std::endl;
            return false;
        }

        return true;
    }

    bool ParseParameterGroups(const DataBuildContext& ctxt, MaterialShader& outShader, const pxr::RnMaterialShader& prim)
    {
        pxr::UsdStageWeakPtr stage = prim.GetPrim().GetStage();
        pxr::SdfPathVector groupPaths = ResolveRelationTargets(prim.GetParamGroupsRel());
        if (groupPaths.empty())
        {
            BuildError(ctxt) << "USD: No parameter groups specified for RnMaterialShader (\"" << prim.GetPath() << "\".)" << std::endl;
            return false;
        }

        for (const pxr::SdfPath& groupPath : groupPaths)
        {
            pxr::UsdPrim maybeGroupPrim = stage->GetPrimAtPath(groupPath);
            if (!maybeGroupPrim.IsA<pxr::RnMaterialShaderParamGroup>())
            {
                BuildError(ctxt) << "USD: Prim \"" << groupPath << "\" is not a valid parameter group." << std::endl;
                return false;
            }
            
            if (!ValidatePrim(ctxt, maybeGroupPrim, PARAMETER_GROUP_PRIM_SCHEMA))
            {
                return false;
            }

            pxr::RnMaterialShaderParamGroup group = pxr::RnMaterialShaderParamGroup(maybeGroupPrim);
            ParameterGroup outGroup = {
                .name = maybeGroupPrim.GetName().GetString()
            };

            pxr::SdfPathVector paramPaths = ResolveRelationTargets(group.GetParametersRel());
            for (const pxr::SdfPath& paramPath : paramPaths)
            {
                pxr::UsdPrim paramPrim = stage->GetPrimAtPath(paramPath);

                Parameter outParam;
                if (!ParseParameter(ctxt, paramPrim, outParam))
                {
                    return false;
                }
                outGroup.parameters.push_back(outParam);
            }

            outShader.parameterGroups.push_back(outGroup);
        }

        return true;
    }

    enum UniformParameterType
    {
        Float,
        Uint,
        Int,

        Count
    };

    struct UniformParameterReflection
    {
        uint32_t  offsetInBytes;
        uint32_t  sizeInBytes;

        UniformParameterType type;
        uint32_t rows;
        uint32_t columns;
    };

    using UniformParameterMap = HashMap<std::string, UniformParameterReflection>;

    void AddUniformParameter(
        UniformParameterMap& uniforms, 
        const std::string& name,
        uint32_t baseOffset,
        ID3D12ShaderReflectionType* type)
    {
        D3D12_SHADER_TYPE_DESC typeDesc{};
        type->GetDesc(&typeDesc);

        switch (typeDesc.Class)
        {
        case D3D_SVC_SCALAR:
        case D3D_SVC_VECTOR:
        case D3D_SVC_MATRIX_COLUMNS:
        case D3D_SVC_MATRIX_ROWS:
        {
            if (typeDesc.Elements == 0 &&            // No array support just yet
                (typeDesc.Type == D3D_SVT_UINT      ||
                    typeDesc.Type == D3D_SVT_FLOAT  ||
                    typeDesc.Type == D3D_SVT_INT))
            {
                UniformParameterReflection parameter = 
                {
                    .offsetInBytes =    typeDesc.Offset + baseOffset,
                    .sizeInBytes =      sizeof(float) * typeDesc.Rows * typeDesc.Columns,
                    .rows =             typeDesc.Rows,
                    .columns =          typeDesc.Columns
                };

                switch (typeDesc.Type)
                {
                case D3D_SVT_UINT:
                    parameter.type = UniformParameterType::Uint;
                    break;
                case D3D_SVT_FLOAT:
                    parameter.type = UniformParameterType::Float;
                    break;
                case D3D_SVT_INT:
                    parameter.type = UniformParameterType::Int;
                    break;
                default:
                    break;
                }

                uniforms[name] = parameter;
            }
        }
            break;
        case D3D_SVC_STRUCT:
        {
            uint32_t memberCount = typeDesc.Members;
            for (uint32_t memberIdx = 0; memberIdx < memberCount; ++memberIdx)
            {
                LPCSTR memberName = type->GetMemberTypeName(memberIdx);
                ID3D12ShaderReflectionType* memberType = type->GetMemberTypeByIndex(memberIdx);

                std::string concatName = name + "." + memberName;
                AddUniformParameter(uniforms, concatName, baseOffset + typeDesc.Offset, memberType);
            }
        }
            break;
        default:
            break;
        }

    }

    constexpr const UniformParameterType CORRESPONDING_UNIFORM_TYPES[] =
    {
        UniformParameterType::Uint,     // ParameterType::TextureParameter
        UniformParameterType::Float,    // ParameterType::FloatVecParameter
        UniformParameterType::Uint,     // ParameterType::UintVecParameter
        UniformParameterType::Int,      // ParameterType::IntVecParameter
    };
    RN_MATCH_ENUM_AND_ARRAY(CORRESPONDING_UNIFORM_TYPES, data::MaterialShaderParameterType)

    bool ReflectMaterialFromExportShader(const DataBuildContext& ctxt, IDxcResult* result, Span<ParameterGroup> paramGroups, uint32_t& outUniformBufferSize)
    {
        ComPtr<IDxcUtils> dxcUtils;
        DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(dxcUtils.GetAddressOf()));

        ComPtr<IDxcBlob> obj;
        if (FAILED(result->GetOutput(DXC_OUT_REFLECTION, IID_PPV_ARGS(obj.GetAddressOf()), nullptr)))
        {
            BuildError(ctxt) << "Failed to retrieve shader reflection from material export shader!" << std::endl;
            return false;
        }

        DxcBuffer reflectionBuffer = {
            .Ptr = obj->GetBufferPointer(),
            .Size = obj->GetBufferSize(),
            .Encoding = 0
        };

        ComPtr<ID3D12ShaderReflection> reflection;
        if (FAILED(dxcUtils->CreateReflection(&reflectionBuffer, IID_PPV_ARGS(reflection.GetAddressOf()))))
        {
            BuildError(ctxt) << "Failed to create D3D12 shader reflection." << std::endl;
            return false;
        }

        ID3D12ShaderReflectionConstantBuffer* cbuffer = reflection->GetConstantBufferByIndex(0);
        D3D12_SHADER_BUFFER_DESC cbufferDesc{};
        cbuffer->GetDesc(&cbufferDesc);

        outUniformBufferSize = cbufferDesc.Size;

        UniformParameterMap reflectedUniforms;
        if (cbufferDesc.Variables > 0)
        {
            ID3D12ShaderReflectionVariable* firstVar = cbuffer->GetVariableByIndex(0);
            ID3D12ShaderReflectionType* firstVarType = firstVar->GetType();

            D3D12_SHADER_TYPE_DESC firstVarTypeDesc{};
            firstVarType->GetDesc(&firstVarTypeDesc);
  
            if (cbufferDesc.Variables == 1 && firstVarTypeDesc.Class == D3D_SVC_STRUCT)
            {
                // New cbuffer syntax: Single variable in cbuffer of type struct
                for (uint32_t memberIdx = 0; memberIdx < firstVarTypeDesc.Members; ++memberIdx)
                {
                    LPCSTR memberName = firstVarType->GetMemberTypeName(memberIdx);
                    ID3D12ShaderReflectionType* memberType = firstVarType->GetMemberTypeByIndex(memberIdx);
                    AddUniformParameter(reflectedUniforms, memberName, 0, memberType);
                }
            }
            else
            {
                // Legacy cbuffer syntax
                for (uint32_t varIdx = 0; varIdx < cbufferDesc.Variables; ++varIdx)
                {
                    ID3D12ShaderReflectionVariable* variable = cbuffer->GetVariableByIndex(varIdx);
                    D3D12_SHADER_VARIABLE_DESC variableDesc{};
                    variable->GetDesc(&variableDesc);

                    LPCSTR varName = variableDesc.Name;
                    ID3D12ShaderReflectionType* varType = variable->GetType();
                    AddUniformParameter(reflectedUniforms, varName, variableDesc.StartOffset, varType);
                }
            }
        }

        for (ParameterGroup& group : paramGroups)
        {
            for (Parameter& param : group.parameters)
            {
                auto it = reflectedUniforms.find(param.param);
                if (it == reflectedUniforms.end())
                {
                    BuildError(ctxt) << "Parameter \"" << param.name << "\" in group \"" << group.name << "\" with shader parameter \"" << param.param << "\" not found in material reflection!" << std::endl;
                    return false;
                }

                if (it->second.type != CORRESPONDING_UNIFORM_TYPES[int(param.type)])
                {
                    BuildError(ctxt) << "Type mismatch for material parameter \"" << param.name << "\" during material reflection!" << std::endl;
                    return false;
                }

                uint32_t uniformDimensions = it->second.rows * it->second.columns;
                uint32_t paramDimensions = 0;
                uint32_t paramElementSize = 0;
                switch (param.type)
                {
                    case data::MaterialShaderParameterType::FloatVec:
                        paramDimensions = param.pFloat.dimension;
                        paramElementSize = sizeof(param.pFloat.defaultValue[0]);
                    break;

                    case data::MaterialShaderParameterType::UintVec:
                        paramDimensions = param.pUint.dimension;
                        paramElementSize = sizeof(param.pUint.defaultValue[0]);
                    break;

                    case data::MaterialShaderParameterType::IntVec:
                        paramDimensions = param.pInt.dimension;
                        paramElementSize = sizeof(param.pInt.defaultValue[0]);
                    break;
                    default:
                        paramDimensions = 1;
                        paramElementSize = sizeof(uint);
                    break;
                }

                if (uniformDimensions != paramDimensions)
                {
                    BuildError(ctxt) << "Dimension mismatch for material parameter \"" << param.name << "\" during material reflection!" << std::endl;
                    return false;
                }

                if (it->second.sizeInBytes != paramDimensions * paramElementSize)
                {
                    BuildError(ctxt) << "Size mismatch for material parameter \"" << param.name << "\" during material reflection!" << std::endl;
                    return false;
                }

                param.offsetInBuffer = it->second.offsetInBytes;
            }
        }

        return true;
    }

    int BuildShadersAndWriteAsset(const DataBuildContext& ctxt, MaterialShader& shader, Vector<EntryPoint>& entryPoints, Vector<std::string>& outFiles)
    {
        using namespace data;

        uint32_t rayTracingLib = INVALID_ENTRYPOINT;
        if (!shader.rayTracingPasses.empty())
        {
            rayTracingLib = PushEntryPoint(entryPoints, ShaderTarget::Library, L"");
        }

        uint32_t materialExport = PushEntryPoint(entryPoints, ShaderTarget::Compute, L"__material_export_main");

        MemoryScope SCOPE;
        ScopedVector<ShaderCompilationResult> results;
        results.reserve(entryPoints.size());
        
        if (!CompileShadersForTargetAPI(ctxt, TargetAPI::D3D12, shader.source, entryPoints, shader.defines, shader.includeDirs, results, outFiles))
        {
            return 1;
        }

        uint32_t uniformDataSize = 0;
        if (!ReflectMaterialFromExportShader(ctxt, results[materialExport].result.Get(), shader.parameterGroups, uniformDataSize))
        {
            return 1;
        }

        auto ResultToByteCode = [&](uint32_t entryPointIdx) -> schema::Bytecode
        {
            if (entryPointIdx != INVALID_ENTRYPOINT)
            {
                ComPtr<IDxcBlob> obj;
                results[entryPointIdx].result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(obj.GetAddressOf()), nullptr);

                Span<uint8_t> bytecode = { static_cast<uint8_t*>(ScopedAlloc(obj->GetBufferSize(), CACHE_LINE_TARGET_SIZE)), obj->GetBufferSize() };
                std::memcpy(bytecode.data(), obj->GetBufferPointer(), obj->GetBufferSize());

                return {
                    .d3d12 = bytecode,
                    .vulkan = {}
                };
            }

            return {};
        };
        
        ScopedVector<schema::VertexRasterPass> vertexRasterPasses;
        vertexRasterPasses.reserve(shader.vertexRasterPasses.size());

        for (const VertexRasterPass& pass : shader.vertexRasterPasses)
        {
            vertexRasterPasses.push_back({
                .renderPass = pass.renderPass,
                .vertexShader = ResultToByteCode(pass.vertexShader),
                .pixelShader = ResultToByteCode(pass.pixelShader)
            });

        }

        ScopedVector<schema::MeshRasterPass> meshRasterPasses;
        meshRasterPasses.reserve(shader.meshRasterPasses.size());
        for (const MeshRasterPass& pass : shader.meshRasterPasses)
        {
            meshRasterPasses.push_back({
                .renderPass = pass.renderPass,
                .meshShader = ResultToByteCode(pass.meshShader),
                .ampShader = ResultToByteCode(pass.ampShader),
                .pixelShader = ResultToByteCode(pass.pixelShader)
            });
        }

        ScopedVector<schema::RayTracingPass> rtPasses;
        rtPasses.reserve(shader.rayTracingPasses.size());
        for (const RayTracingPass& pass : shader.rayTracingPasses)
        {
            rtPasses.push_back({
                .renderPass = pass.renderPass,
                .hitGroupName = pass.hitGroupName,
                .closestHitExport = pass.closestHitShader,
                .anyHitExport = pass.anyHitShader
            });
        }

        ScopedVector<std::string_view> references;
        references.reserve(32);
        auto PushReference = [](ScopedVector<std::string_view>& references, const std::string_view& ref) -> uint32_t
        {
            uint32_t idx = uint32_t(references.size());
            references.push_back(ref);
            return idx;
        };

        ScopedVector<schema::ParameterGroup> paramGroups;
        paramGroups.reserve(shader.parameterGroups.size());

        ScopedVector<schema::TextureParameter> textureParams;
        textureParams.reserve(32);
        size_t textureOffset = 0;

        ScopedVector<schema::FloatVecParameter> floatVecParams;
        floatVecParams.reserve(32);
        size_t floatVecOffset = 0;

        ScopedVector<schema::UintVecParameter> uintVecParams;
        uintVecParams.reserve(32);
        size_t uintVecOffset = 0;

        ScopedVector<schema::IntVecParameter> intVecParams;
        intVecParams.reserve(32);
        size_t intVecOffset = 0;

        for (ParameterGroup& paramGroup : shader.parameterGroups)
        {
            size_t textureCount = 0;
            size_t floatVecCount = 0;
            size_t uintVecCount = 0;
            size_t intVecCount = 0;
            for (Parameter& param : paramGroup.parameters)
            {
                switch(param.type)
                {
                    case data::MaterialShaderParameterType::Texture:
                    {
                        asset::schema::Reference ref = {};
                        if (!param.texture.empty())
                        {
                            ref = { .identifier = PushReference(references, param.texture) };
                        }

                        textureParams.push_back({
                            .name = param.name,
                            .offsetInBuffer = param.offsetInBuffer,
                            .type = param.textureType,
                            .defaultValue = ref
                        });

                        ++textureCount;
                    }
                    break;
                    case data::MaterialShaderParameterType::FloatVec:
                    {
                        floatVecParams.push_back({
                            .name = param.name,
                            .offsetInBuffer = param.offsetInBuffer,
                            .dimension = param.pFloat.dimension,
                            .defaultValue = { param.pFloat.defaultValue, param.pFloat.dimension },
                            .minValue = { param.pFloat.min, param.pFloat.dimension },
                            .maxValue = { param.pFloat.max, param.pFloat.dimension }
                        });

                        ++floatVecCount;
                    }
                    break;
                    case data::MaterialShaderParameterType::UintVec:
                    {
                        uintVecParams.push_back({
                            .name = param.name,
                            .offsetInBuffer = param.offsetInBuffer,
                            .dimension = param.pFloat.dimension,
                            .defaultValue = { param.pUint.defaultValue, param.pUint.dimension },
                            .minValue = { param.pUint.min, param.pUint.dimension },
                            .maxValue = { param.pUint.max, param.pUint.dimension },
                        });

                        ++uintVecCount;
                    }
                    break;
                    case data::MaterialShaderParameterType::IntVec:
                    {
                        intVecParams.push_back({
                            .name = param.name,
                            .offsetInBuffer = param.offsetInBuffer,
                            .dimension = param.pFloat.dimension,
                            .defaultValue = { param.pInt.defaultValue, param.pInt.dimension },
                            .minValue = { param.pInt.min, param.pInt.dimension },
                            .maxValue = { param.pInt.max, param.pInt.dimension },
                        });

                        ++intVecCount;
                    }
                    break;
                }
            }

            paramGroups.push_back({
                .name = paramGroup.name,
                .textureParameters = { textureParams.data() + textureOffset, textureCount },
                .floatVecParameters = { floatVecParams.data() + floatVecOffset, floatVecCount },
                .uintVecParameters = { uintVecParams.data() + uintVecOffset, uintVecCount },
                .intVecParameters = { intVecParams.data() + intVecOffset, intVecCount },
            });

            textureOffset += textureCount;
            floatVecOffset += floatVecCount;
            uintVecOffset += uintVecCount;
            intVecOffset += intVecCount;
        }

        schema::MaterialShader outShader = {
            .vertexRasterPasses = vertexRasterPasses,
            .meshRasterPasses = meshRasterPasses,
            .rayTracingPasses = rtPasses,
            .parameterGroups = paramGroups,
            .uniformDataSize = uniformDataSize,
            .rayTracingLibrary = ResultToByteCode(rayTracingLib),
        };

        size_t serializedSize = schema::MaterialShader::SerializedSize(outShader);
        Span<uint8_t> outData = { static_cast<uint8_t*>(ScopedAlloc(serializedSize, CACHE_LINE_TARGET_SIZE)), serializedSize };
        rn::Serialize<schema::MaterialShader>(outData, outShader);
        
        return WriteAssetToDisk(ctxt, ".material_shader", outData, references, outFiles);
    }

    int ProcessUsdMaterialShader(const DataBuildContext& ctxt, const pxr::UsdPrim& prim, Vector<std::string>& outFiles)
    {
        MaterialShader shader = {};
        Vector<EntryPoint> entryPoints;

        pxr::RnMaterialShader materialShaderPrim(prim);
        if (!ParseMaterialShader(ctxt, shader, prim, outFiles) ||
            !ParseRenderPasses(ctxt, materialShaderPrim, shader, entryPoints, outFiles) ||
            !ParseParameterGroups(ctxt, shader, materialShaderPrim))
        {
            return 1;
        }
        
        return BuildShadersAndWriteAsset(ctxt, shader, entryPoints, outFiles);
    }
}