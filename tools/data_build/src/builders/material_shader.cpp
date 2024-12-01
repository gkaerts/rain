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

#include "asset/schema/reference_generated.h"
#include "data/schema/common_generated.h"
#include "data/schema/render_pass_generated.h"
#include "data/schema/material_shader_generated.h"

#include "shader.hpp"
#include "d3d12shader.h"
#include <string>

using namespace Microsoft::WRL;

namespace rn
{
    constexpr const uint32_t INVALID_ENTRYPOINT = 0xFFFFFFFF;
    struct VertexRasterPass
    {
        schema::render::MaterialRenderPass renderPass;
        uint32_t vertexShader = INVALID_ENTRYPOINT;
        uint32_t pixelShader = INVALID_ENTRYPOINT;
    };

    struct MeshRasterPass
    {
        schema::render::MaterialRenderPass renderPass;
        uint32_t ampShader = INVALID_ENTRYPOINT;
        uint32_t meshShader = INVALID_ENTRYPOINT;
        uint32_t pixelShader = INVALID_ENTRYPOINT;
    };

    struct RayTracingPass
    {
        schema::render::MaterialRenderPass renderPass;
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
        schema::render::ParameterType type;
        std::string name;
        std::string param;
        uint32_t offsetInBuffer;

        std::string texture = "";
        union
        {
            schema::render::TextureParameterDimension textureDimension;
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

    schema::render::MaterialRenderPass ToRenderPass(std::string_view typeStr)
    {
        uint32_t idx = 0;
        for (int i = 0; i < CountOf<schema::render::MaterialRenderPass>(); ++i)
        {
            if (typeStr == schema::render::EnumNameMaterialRenderPass(schema::render::MaterialRenderPass(i)))
            {
                return schema::render::MaterialRenderPass(idx);
            }
            ++idx;
        }

        return schema::render::MaterialRenderPass::Count;
    }

    bool IsValidRenderPassName(std::string_view file, const pxr::UsdAttribute& prop)
    {
        std::string paramStr = Value<pxr::TfToken>(prop).GetString();
        for (int i = 0; i < CountOf<schema::render::MaterialRenderPass>(); ++i)
        {
            if (paramStr == schema::render::EnumNameMaterialRenderPass(schema::render::MaterialRenderPass(i)))
            {
                return true;
            }
        }

        return false;
    }

    const pxr::TfToken TOKEN_2D = pxr::TfToken("2D");
    const pxr::TfToken TOKEN_3D = pxr::TfToken("3D");

    bool IsValidTextureDimension(std::string_view file, const pxr::UsdAttribute& prop)
    {
        pxr::TfToken dim = Value<pxr::TfToken>(prop);
        return dim == TOKEN_2D || dim == TOKEN_3D;
    }

    bool IsValidVecDimension(std::string_view file, const pxr::UsdAttribute& prop)
    {
        return Value<uint32_t>(prop) > 0 && Value<uint32_t>(prop) <= 16;
    }

    const PrimSchema MATERIAL_SHADER_PRIM_SCHEMA = {
        .requiredProperties = {
            { .name = "source", .fnIsValidPropertyValue = IsNonEmptyAssetPath },
            { .name = "renderPasses" },
            { .name = "paramGroups" }
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
            { .name = "parameters" }
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

    bool ParseMaterialShader(MaterialShader& outShader, std::string_view file, const pxr::UsdPrim& prim, Vector<std::string>& outFiles)
    {
        if (!ValidatePrim(file, prim, MATERIAL_SHADER_PRIM_SCHEMA))
        {
            return false;
        }

        pxr::RnMaterialShader msPrim = pxr::RnMaterialShader(prim);

        pxr::SdfAssetPath source = Value<pxr::SdfAssetPath>(msPrim.GetSourceAttr());
        auto defines = Value<pxr::VtArray<std::string>>(msPrim.GetDefinesAttr());
        auto includeDirs = Value<pxr::VtArray<std::string>>(msPrim.GetIncludeDirsAttr());

        outShader.source = ToWString(source.GetAssetPath());

        for (const std::string& tok : defines)
        {
            outShader.defines.push_back(ToWString(tok));
        }

        for (const std::string& tok : includeDirs)
        {
            outShader.includeDirs.push_back(ToWString(tok));
        }

        outFiles.push_back(MakeRelativeTo(file, outShader.source).string());
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


    bool ParseRenderPasses(std::string_view file, const pxr::RnMaterialShader& prim, MaterialShader& outShader, Vector<EntryPoint>& entryPoints, Vector<std::string>& outFiles)
    {
        pxr::UsdStageWeakPtr stage = prim.GetPrim().GetStage();

        pxr::SdfPathVector renderPassPaths = ResolveRelationTargets(prim.GetRenderPassesRel());
        if (renderPassPaths.empty())
        {
            BuildError(file) << "USD: No render passes specified for RnMaterialShader (\"" << prim.GetPath() << "\".)" << std::endl;
            return false;
        }

        for (const pxr::SdfPath& path : renderPassPaths)
        {
            pxr::UsdPrim passPrim = stage->GetPrimAtPath(path);
            if (passPrim.IsA<pxr::RnVertexRasterPass>())
            {
                if (!ValidatePrim(file, passPrim, VERTEX_RASTER_PASS_PRIM_SCHEMA))
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
                if (!ValidatePrim(file, passPrim, MESH_RASTER_PASS_PRIM_SCHEMA))
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
                if (!ValidatePrim(file, passPrim, RAY_TRACING_PASS_PRIM_SCHEMA))
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
                BuildError(file) << "USD: Prim \"" << passPrim.GetPath() << "\" is not a valid render pass - RnMaterialShader (\"" << prim.GetPath() << "\".)" << std::endl;
                return false;
            }
        }

        return true;
    }

    bool ParseParameter(std::string_view file, const DataBuildOptions& options, const pxr::UsdPrim& prim, Parameter& outParam)
    {
        if (prim.IsA<pxr::RnMaterialShaderParamFloatVec>())
        {
            if (!ValidatePrim(file, prim, FLOAT_VEC_PARAM_PRIM_SCHEMA))
            {
                return false;
            }

            pxr::RnMaterialShaderParamFloatVec param(prim);
            auto defaultValues =    Value<pxr::VtArray<float>>(param.GetDefaultValueAttr());
            auto minValues =        Value<pxr::VtArray<float>>(param.GetMinValueAttr());
            auto maxValues =        Value<pxr::VtArray<float>>(param.GetMaxValueAttr());

            outParam = {
                .type = schema::render::ParameterType::FloatVecParameter,
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
            if (!ValidatePrim(file, prim, UINT_VEC_PARAM_PRIM_SCHEMA))
            {
                return false;
            }

            pxr::RnMaterialShaderParamUintVec param(prim);
            auto defaultValues =    Value<pxr::VtArray<uint32_t>>(param.GetDefaultValueAttr());
            auto minValues =        Value<pxr::VtArray<uint32_t>>(param.GetMinValueAttr());
            auto maxValues =        Value<pxr::VtArray<uint32_t>>(param.GetMaxValueAttr());

            outParam = {
                .type = schema::render::ParameterType::UintVecParameter,
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
            if (!ValidatePrim(file, prim, INT_VEC_PARAM_PRIM_SCHEMA))
            {
                return false;
            }

            pxr::RnMaterialShaderParamIntVec param(prim);
            auto defaultValues =    Value<pxr::VtArray<int32_t>>(param.GetDefaultValueAttr());
            auto minValues =        Value<pxr::VtArray<int32_t>>(param.GetMinValueAttr());
            auto maxValues =        Value<pxr::VtArray<int32_t>>(param.GetMaxValueAttr());

            outParam = {
                .type = schema::render::ParameterType::IntVecParameter,
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
            if (!ValidatePrim(file, prim, TEXTURE_PARAM_PRIM_SCHEMA))
            {
                return false;
            }

            pxr::RnMaterialShaderParamTexture param(prim);
            std::string resolvedDefault = Value<pxr::SdfAssetPath>(param.GetDefaultValueAttr()).GetAssetPath();
            std::filesystem::path defaultPath = MakeRelativeTo(file, resolvedDefault);
            defaultPath.replace_extension("texture");

            pxr::TfToken dimension = Value<pxr::TfToken>(param.GetDimensionAttr());
            outParam = {
                .type = schema::render::ParameterType::TextureParameter,
                .name = prim.GetName().GetString(),
                .param = Value<pxr::TfToken>(param.GetHlslParamAttr()).GetString(),
                .texture = defaultPath.string(),
                .textureDimension = dimension == TOKEN_3D ?
                    schema::render::TextureParameterDimension::_3D :
                    schema::render::TextureParameterDimension::_2D
            };
        }
        else
        {
            BuildError(file) << "USD: Prim \"""\" is not a valid material parameter type." << std::endl;
            return false;
        }

        return true;
    }

    bool ParseParameterGroups(MaterialShader& outShader, const DataBuildOptions& options, std::string_view file, const pxr::RnMaterialShader& prim)
    {
        pxr::UsdStageWeakPtr stage = prim.GetPrim().GetStage();
        pxr::SdfPathVector groupPaths = ResolveRelationTargets(prim.GetParamGroupsRel());
        if (groupPaths.empty())
        {
            BuildError(file) << "USD: No parameter groups specified for RnMaterialShader (\"" << prim.GetPath() << "\".)" << std::endl;
            return false;
        }

        for (const pxr::SdfPath& groupPath : groupPaths)
        {
            pxr::UsdPrim maybeGroupPrim = stage->GetPrimAtPath(groupPath);
            if (!maybeGroupPrim.IsA<pxr::RnMaterialShaderParamGroup>())
            {
                BuildError(file) << "USD: Prim \"" << groupPath << "\" is not a valid parameter group." << std::endl;
                return false;
            }
            
            if (!ValidatePrim(file, maybeGroupPrim, PARAMETER_GROUP_PRIM_SCHEMA))
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
                if (!ParseParameter(file, options, paramPrim, outParam))
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
        UniformParameterType::Count,    // ParameterType::NONE
        UniformParameterType::Uint,     // ParameterType::TextureParameter
        UniformParameterType::Float,    // ParameterType::FloatVecParameter
        UniformParameterType::Uint,     // ParameterType::UintVecParameter
        UniformParameterType::Int,      // ParameterType::IntVecParameter
    };
    static_assert(RN_ARRAY_SIZE(CORRESPONDING_UNIFORM_TYPES) == int(schema::render::ParameterType::MAX) + 1);

    bool ReflectMaterialFromExportShader(std::string_view file, IDxcResult* result, Span<ParameterGroup> paramGroups)
    {
        ComPtr<IDxcUtils> dxcUtils;
        DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(dxcUtils.GetAddressOf()));

        ComPtr<IDxcBlob> obj;
        if (FAILED(result->GetOutput(DXC_OUT_REFLECTION, IID_PPV_ARGS(obj.GetAddressOf()), nullptr)))
        {
            BuildError(file) << "Failed to retrieve shader reflection from material export shader!" << std::endl;
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
            BuildError(file) << "Failed to create D3D12 shader reflection." << std::endl;
            return false;
        }

        ID3D12ShaderReflectionConstantBuffer* cbuffer = reflection->GetConstantBufferByIndex(0);
        D3D12_SHADER_BUFFER_DESC cbufferDesc{};
        cbuffer->GetDesc(&cbufferDesc);

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
                    BuildError(file) << "Parameter \"" << param.name << "\" in group \"" << group.name << "\" with shader parameter \"" << param.param << "\" not found in material reflection!" << std::endl;
                    return false;
                }

                if (it->second.type != CORRESPONDING_UNIFORM_TYPES[int(param.type)])
                {
                    BuildError(file) << "Type mismatch for material parameter \"" << param.name << "\" during material reflection!" << std::endl;
                    return false;
                }

                uint32_t uniformDimensions = it->second.rows * it->second.columns;
                uint32_t paramDimensions = 0;
                uint32_t paramElementSize = 0;
                switch (param.type)
                {
                    case schema::render::ParameterType::FloatVecParameter:
                        paramDimensions = param.pFloat.dimension;
                        paramElementSize = sizeof(param.pFloat.defaultValue[0]);
                    break;

                    case schema::render::ParameterType::UintVecParameter:
                        paramDimensions = param.pUint.dimension;
                        paramElementSize = sizeof(param.pUint.defaultValue[0]);
                    break;

                    case schema::render::ParameterType::IntVecParameter:
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
                    BuildError(file) << "Dimension mismatch for material parameter \"" << param.name << "\" during material reflection!" << std::endl;
                    return false;
                }

                if (it->second.sizeInBytes != paramDimensions * paramElementSize)
                {
                    BuildError(file) << "Size mismatch for material parameter \"" << param.name << "\" during material reflection!" << std::endl;
                    return false;
                }

                param.offsetInBuffer = it->second.offsetInBytes;
            }
        }

        return true;
    }

    int BuildShadersAndWriteAsset(std::string_view file, MaterialShader& shader, Vector<EntryPoint>& entryPoints, const DataBuildOptions& options, Vector<std::string>& outFiles)
    {
        uint32_t rayTracingLib = INVALID_ENTRYPOINT;
        if (!shader.rayTracingPasses.empty())
        {
            rayTracingLib = PushEntryPoint(entryPoints, ShaderTarget::Library, L"");
        }

        uint32_t materialExport = PushEntryPoint(entryPoints, ShaderTarget::Compute, L"__material_export_main");

        MemoryScope SCOPE;
        ScopedVector<ShaderCompilationResult> results;
        results.reserve(entryPoints.size());
        
        if (!CompileShadersForTargetAPI(file, options, TargetAPI::D3D12, shader.source, entryPoints, shader.defines, shader.includeDirs, results, outFiles))
        {
            return 1;
        }

        if (!ReflectMaterialFromExportShader(file, results[materialExport].result.Get(), shader.parameterGroups))
        {
            return 1;
        }
        
        flatbuffers::FlatBufferBuilder fbb;
        auto ResultToFBByteCode = [&](uint32_t entryPointIdx) -> flatbuffers::Offset<schema::render::Bytecode>
        {
            flatbuffers::Offset<flatbuffers::Vector<uint8_t>> vec = 0;
            if (entryPointIdx != INVALID_ENTRYPOINT)
            {
                ComPtr<IDxcBlob> obj;
                results[entryPointIdx].result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(obj.GetAddressOf()), nullptr);
                vec = fbb.CreateVector(static_cast<const uint8_t*>(obj->GetBufferPointer()), obj->GetBufferSize());
                return schema::render::CreateBytecode(fbb, vec, 0);
            }

            return 0;
        };

        Vector<flatbuffers::Offset<schema::render::VertexRasterPass>> fbVertexRasterPasses;
        for (const VertexRasterPass& pass : shader.vertexRasterPasses)
        {
            fbVertexRasterPasses.push_back(
                schema::render::CreateVertexRasterPass(
                    fbb,
                    pass.renderPass,
                    ResultToFBByteCode(pass.vertexShader),
                    ResultToFBByteCode(pass.pixelShader)));
        }

        Vector<flatbuffers::Offset<schema::render::MeshRasterPass>> fbMeshRasterPasses;
        for (const MeshRasterPass& pass : shader.meshRasterPasses)
        {
            fbMeshRasterPasses.push_back(
                schema::render::CreateMeshRasterPass(
                    fbb,
                    pass.renderPass,
                    ResultToFBByteCode(pass.ampShader),
                    ResultToFBByteCode(pass.meshShader),
                    ResultToFBByteCode(pass.pixelShader)));
        }

        Vector<flatbuffers::Offset<schema::render::RayTracingPass>> fbRTPasses;
        for (const RayTracingPass& pass : shader.rayTracingPasses)
        {
            fbRTPasses.push_back(
                    schema::render::CreateRayTracingPassDirect(
                    fbb,
                    pass.renderPass,
                    pass.hitGroupName.c_str(),
                    pass.closestHitShader.c_str(),
                    pass.anyHitShader.c_str()));
        }

        Vector<std::string_view> references;
        auto PushReference = [](Vector<std::string_view>& references, const std::string_view& ref) -> uint32_t
        {
            uint32_t idx = uint32_t(references.size());
            references.push_back(ref);
            return idx;
        };

        Vector<flatbuffers::Offset<schema::render::ParameterGroup>> fbParamGroups;
        for (const ParameterGroup& paramGroup : shader.parameterGroups)
        {
            MemoryScope SCOPE;
            ScopedVector<flatbuffers::Offset<schema::render::Parameter>> fbParams;
            fbParams.reserve(paramGroup.parameters.size());

            for (const Parameter& param : paramGroup.parameters)
            {
                flatbuffers::Offset<> paramData = 0;
                switch(param.type)
                {
                    case schema::render::ParameterType::TextureParameter:
                    {
                        schema::Reference ref;
                        schema::Reference* refPtr = nullptr;
                        if (!param.texture.empty())
                        {
                            ref = schema::Reference(PushReference(references, param.texture));
                            refPtr = &ref;
                        }

                        paramData = schema::render::CreateTextureParameter(fbb,
                            param.textureDimension,
                            refPtr).Union();
                    }
                    break;
                    case schema::render::ParameterType::FloatVecParameter:
                    {
                        paramData = schema::render::CreateFloatVecParameter(fbb,
                            param.pFloat.dimension,
                            fbb.CreateVector<float>(param.pFloat.defaultValue, param.pFloat.dimension), 
                            fbb.CreateVector<float>(param.pFloat.min, param.pFloat.dimension),
                            fbb.CreateVector<float>(param.pFloat.max, param.pFloat.dimension)).Union();
                    }
                    break;
                    case schema::render::ParameterType::UintVecParameter:
                    {
                        paramData = schema::render::CreateUintVecParameter(fbb,
                            param.pUint.dimension,
                            fbb.CreateVector<uint32_t>(param.pUint.defaultValue, param.pUint.dimension), 
                            fbb.CreateVector<uint32_t>(param.pUint.min, param.pUint.dimension),
                            fbb.CreateVector<uint32_t>(param.pUint.max, param.pUint.dimension)).Union();
                    }
                    break;
                    case schema::render::ParameterType::IntVecParameter:
                    {
                        paramData = schema::render::CreateIntVecParameter(fbb, 
                            param.pInt.dimension,
                            fbb.CreateVector<int32_t>(param.pInt.defaultValue, param.pInt.dimension), 
                            fbb.CreateVector<int32_t>(param.pInt.min, param.pInt.dimension),
                            fbb.CreateVector<int32_t>(param.pInt.max, param.pInt.dimension)).Union();
                    }
                    break;
                }

                fbParams.push_back(schema::render::CreateParameterDirect(
                    fbb,
                    param.name.c_str(),
                    param.offsetInBuffer,
                    param.type,
                    paramData));
            }

            auto fbParamVec = fbb.CreateVector(fbParams.data(), fbParams.size());
            fbParamGroups.push_back(
                schema::render::CreateParameterGroup(
                    fbb,
                    fbb.CreateString(paramGroup.name),
                    fbParamVec));
        }


        auto fbRoot = schema::render::CreateMaterialShader(
            fbb,
            fbb.CreateVector(fbVertexRasterPasses.data(), fbVertexRasterPasses.size()),
            fbb.CreateVector(fbMeshRasterPasses.data(), fbMeshRasterPasses.size()),
            ResultToFBByteCode(rayTracingLib),
            fbb.CreateVector(fbRTPasses.data(), fbRTPasses.size()),
            fbb.CreateVector(fbParamGroups.data(), fbParamGroups.size()));
        
        fbb.Finish(fbRoot, schema::render::MaterialShaderIdentifier());
        return WriteAssetToDisk(file, schema::render::MaterialShaderExtension(), options, fbb.GetBufferSpan(), references, outFiles);
    }

    int ProcessUsdMaterialShader(std::string_view file, const DataBuildOptions& options, const pxr::UsdPrim& prim, Vector<std::string>& outFiles)
    {
        MaterialShader shader = {};
        Vector<EntryPoint> entryPoints;

        pxr::RnMaterialShader materialShaderPrim(prim);
        if (!ParseMaterialShader(shader, file, prim, outFiles) ||
            !ParseRenderPasses(file, materialShaderPrim, shader, entryPoints, outFiles) ||
            !ParseParameterGroups(shader, options, file, materialShaderPrim))
        {
            return 1;
        }
        
        return BuildShadersAndWriteAsset(file, shader, entryPoints, options, outFiles);
    }
}