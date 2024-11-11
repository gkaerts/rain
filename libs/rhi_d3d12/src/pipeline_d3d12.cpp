#define NOMINMAX

#include "device_d3d12.hpp"
#include "format_d3d12.hpp"
#include "d3d12.h"

#include "common/log/log.hpp"
#include "common/memory/memory.hpp"
#include "common/memory/vector.hpp"
#include "common/memory/string.hpp"

namespace rn::rhi
{
    RN_LOG_CATEGORY(D3D12)

    namespace
    {
        constexpr D3D12_ROOT_PARAMETER DescribeRootConstants(UINT numValues, UINT cbRegister, UINT space = 0)
        {
            return 
            {
                .ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS,
                .Constants = { cbRegister, space, numValues},
            };
        }

        constexpr const uint32_t NUM_ROOT_CONSTANTS = 8;
        constexpr const D3D12_ROOT_PARAMETER BINDLESS_ROOT_PARAMETERS[] = 
        {
            DescribeRootConstants(NUM_ROOT_CONSTANTS, 0)
        };

        constexpr D3D12_STATIC_SAMPLER_DESC DescribeStaticSampler(
            UINT reg,
            D3D12_FILTER filter,
            D3D12_TEXTURE_ADDRESS_MODE addressU,
            D3D12_TEXTURE_ADDRESS_MODE addressV,
            D3D12_TEXTURE_ADDRESS_MODE addressW)
        {
            return
            {
                .Filter = filter,
                .AddressU = addressU,
                .AddressV = addressV,
                .AddressW = addressW,
                .MipLODBias = 0.0f,
                .MaxAnisotropy = 16,
                .ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER,
                .BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK,
                .MinLOD = 0.0f,
                .MaxLOD = D3D12_FLOAT32_MAX,
                .ShaderRegister = reg,
                .RegisterSpace = 0,
                .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL,
            };
        }

        constexpr const D3D12_STATIC_SAMPLER_DESC STATIC_SAMPLERS[] =
        {
            #include "shader/static_samplers.h"
        };

        ID3D12RootSignature* CreateRootSignature(ID3D12Device10* device, const D3D12_VERSIONED_ROOT_SIGNATURE_DESC& desc, const wchar_t* name)
        {
            ID3DBlob* rootSigBlob = nullptr;
            ID3DBlob* errorBlob = nullptr;
            if (FAILED(D3D12SerializeVersionedRootSignature(&desc, &rootSigBlob, &errorBlob)))
            {
                LogError(LogCategory::D3D12, "Failed to serialize root signature: {}", (const char*)errorBlob->GetBufferPointer());
                RN_ASSERT(false);
            }

            if (errorBlob)
            {
                errorBlob->Release();
                errorBlob = nullptr;
            }

            ID3D12RootSignature* rootSig = nullptr;
            RN_ASSERT(SUCCEEDED(device->CreateRootSignature(0,
                rootSigBlob->GetBufferPointer(),
                rootSigBlob->GetBufferSize(),
                __uuidof(ID3D12RootSignature),
                (void**)(&rootSig))));

            if (rootSigBlob)
            {
                rootSigBlob->Release();
                rootSigBlob = nullptr;
            }


            rootSig->SetName(name);
            return rootSig;
        }
    }

    ID3D12RootSignature* CreateBindlessRootSignature(ID3D12Device10* device)
    {
        const D3D12_VERSIONED_ROOT_SIGNATURE_DESC desc = 
        {
            .Version = D3D_ROOT_SIGNATURE_VERSION_1_0,
            .Desc_1_0 = 
            {
                .NumParameters = RN_ARRAY_SIZE(BINDLESS_ROOT_PARAMETERS),
                .pParameters = BINDLESS_ROOT_PARAMETERS,
                .NumStaticSamplers = RN_ARRAY_SIZE(STATIC_SAMPLERS),
                .pStaticSamplers = STATIC_SAMPLERS,
                .Flags = 
                    D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | 
                    D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED
            }
        };

        return CreateRootSignature(device, desc, L"Bindless Root Signature");
    }

    ID3D12RootSignature* CreateRayTracingLocalRootSignature(ID3D12Device10* device)
    {
        const D3D12_VERSIONED_ROOT_SIGNATURE_DESC desc = {
            .Version = D3D_ROOT_SIGNATURE_VERSION_1_0,
            .Desc_1_0 = {
                .NumParameters = 0,
                .pParameters = nullptr,
                .NumStaticSamplers = 0,
                .pStaticSamplers = nullptr,
                .Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE
            }
        };

        return CreateRootSignature(device, desc, L"RT Local Root Signature");
    }

    namespace
    {
        constexpr D3D12_RASTERIZER_DESC DescribeRasterizerState(D3D12_FILL_MODE fillMode, D3D12_CULL_MODE cullMode)
        {
            return 
            {
                .FillMode = fillMode,
                .CullMode = cullMode,
                .FrontCounterClockwise = TRUE,
                .DepthBias = 0,
                .DepthBiasClamp = 0.0f,
                .SlopeScaledDepthBias = 0.0f,
                .DepthClipEnable = TRUE,
                .MultisampleEnable = FALSE,
                .AntialiasedLineEnable = FALSE,
                .ForcedSampleCount = 0,
                .ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF,
            };
        }

        constexpr const D3D12_RASTERIZER_DESC RASTERIZER_STATES[] =
        {
            DescribeRasterizerState(D3D12_FILL_MODE_SOLID, D3D12_CULL_MODE_BACK),   // SolidBackFaceCull
            DescribeRasterizerState(D3D12_FILL_MODE_SOLID, D3D12_CULL_MODE_FRONT),  // SolidFrontFaceCull
            DescribeRasterizerState(D3D12_FILL_MODE_SOLID, D3D12_CULL_MODE_NONE),   // SolidNoCull
        };
        RN_MATCH_ENUM_AND_ARRAY(RASTERIZER_STATES, RasterizerState);

        constexpr D3D12_BLEND_DESC DescribeBlendState(
            bool blendEnabled, 
            D3D12_BLEND srcBlend, 
            D3D12_BLEND destBlend, 
            D3D12_BLEND_OP blendOp,
            D3D12_BLEND srcBlendAlpha,
            D3D12_BLEND destBlendAlpha,
            D3D12_BLEND_OP blendOpAlpha)
        {
            return 
            {
                .AlphaToCoverageEnable = FALSE,
                .IndependentBlendEnable = FALSE,
                .RenderTarget = {
                    {
                        .BlendEnable = blendEnabled ? TRUE : FALSE,
                        .SrcBlend = srcBlend,
                        .DestBlend = destBlend,
                        .BlendOp = blendOp,
                        .SrcBlendAlpha = srcBlendAlpha,
                        .DestBlendAlpha = destBlendAlpha,
                        .BlendOpAlpha = blendOpAlpha,
                        .RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL,
                    }
                }
            };
        }

        constexpr const D3D12_BLEND_DESC BLEND_STATES[] =
        {
            DescribeBlendState(false, D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD, D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD),                        // Disabled
            DescribeBlendState(true, D3D12_BLEND_SRC_ALPHA, D3D12_BLEND_INV_SRC_ALPHA, D3D12_BLEND_OP_ADD, D3D12_BLEND_ONE, D3D12_BLEND_INV_SRC_ALPHA, D3D12_BLEND_OP_ADD), // AlphaBlend
            DescribeBlendState(true, D3D12_BLEND_ONE, D3D12_BLEND_ONE, D3D12_BLEND_OP_ADD, D3D12_BLEND_ONE, D3D12_BLEND_ONE, D3D12_BLEND_OP_ADD),                           // Additive
        };
        RN_MATCH_ENUM_AND_ARRAY(BLEND_STATES, BlendState);

        constexpr D3D12_DEPTH_STENCIL_DESC DescribeDepthState(bool depthEnabled, bool depthWriteEnabled, D3D12_COMPARISON_FUNC depthFunc)
        {
            return 
            {
                .DepthEnable = depthEnabled ? TRUE : FALSE,
                .DepthWriteMask = depthWriteEnabled ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO,
                .DepthFunc = depthFunc,
            };
        }

        constexpr const D3D12_DEPTH_STENCIL_DESC DEPTH_STATES[] =
        {
            DescribeDepthState(false, false, D3D12_COMPARISON_FUNC_ALWAYS),            // Disabled
            DescribeDepthState(true, true, D3D12_COMPARISON_FUNC_GREATER_EQUAL),       // DepthWriteGreaterEqual
            DescribeDepthState(true, true, D3D12_COMPARISON_FUNC_LESS_EQUAL),          // DepthWriteLessEqual
            DescribeDepthState(true, false, D3D12_COMPARISON_FUNC_GREATER_EQUAL),      // DepthReadGreaterEqual
            DescribeDepthState(true, false, D3D12_COMPARISON_FUNC_LESS_EQUAL),         // DepthReadLessEqual
            DescribeDepthState(true, false, D3D12_COMPARISON_FUNC_EQUAL),              // DepthReadEqual
        };
        RN_MATCH_ENUM_AND_ARRAY(DEPTH_STATES, DepthState);

        constexpr const D3D12_PRIMITIVE_TOPOLOGY_TYPE TOPOLOGY_TYPES[] =
        {
            D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
            D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE,
        };
        RN_MATCH_ENUM_AND_ARRAY(TOPOLOGY_TYPES, TopologyType);


        constexpr const D3D12_INPUT_ELEMENT_DESC GPU_DRIVEN_INPUT_ELEMENTS[] =
        {
            {
                .SemanticName = "RN_DRAW_ID",
                .SemanticIndex = 0,
                .Format = DXGI_FORMAT_R32_UINT,
                .InputSlot = 0,
                .AlignedByteOffset = 0,
                .InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA,
                .InstanceDataStepRate = 0xFFFFFFFF
            }
        };

        constexpr const D3D12_INPUT_LAYOUT_DESC EMPTY_INPUT_LAYOUT = {};
        constexpr const D3D12_INPUT_LAYOUT_DESC GPU_DRIVEN_INPUT_LAYOUT = {
            .pInputElementDescs = GPU_DRIVEN_INPUT_ELEMENTS,
            .NumElements = RN_ARRAY_SIZE(GPU_DRIVEN_INPUT_ELEMENTS)
        };

        template <typename T, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE D3DType>
        struct alignas(void*) D3D12StateSubobject
        {
            D3D12StateSubobject() : subobjectType(D3DType), value(T()) {}
            D3D12StateSubobject(const T& v) : subobjectType(D3DType), value(v) {}
            D3D12StateSubobject(const D3D12StateSubobject<T, D3DType>& rhs) : subobjectType(D3DType), value(rhs.value) {}

            D3D12StateSubobject& operator=(const D3D12StateSubobject<T, D3DType>& rhs)
            {
                value = rhs.value;
                return *this;
            }

            D3D12StateSubobject& operator=(const T& rhs)
            {
                value = rhs;
                return *this;
            }

            D3D12_PIPELINE_STATE_SUBOBJECT_TYPE subobjectType;
            T value;
        };

        using D3D12StateVSBytecode =    D3D12StateSubobject<D3D12_SHADER_BYTECODE,          D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VS>;
        using D3D12StatePSBytecode =    D3D12StateSubobject<D3D12_SHADER_BYTECODE,          D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PS>;
        using D3D12StateCSBytecode =    D3D12StateSubobject<D3D12_SHADER_BYTECODE,          D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_CS>;
        using D3D12StateASBytecode =    D3D12StateSubobject<D3D12_SHADER_BYTECODE,          D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_AS>;
        using D3D12StateMSBytecode =    D3D12StateSubobject<D3D12_SHADER_BYTECODE,          D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_MS>;
        using D3D12StateRootsig =       D3D12StateSubobject<ID3D12RootSignature*,           D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE>;
        using D3D12StateRasterizer =    D3D12StateSubobject<D3D12_RASTERIZER_DESC,          D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER>;
        using D3D12StateBlend =         D3D12StateSubobject<D3D12_BLEND_DESC,               D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_BLEND>;
        using D3D12StateDepthStencil =  D3D12StateSubobject<D3D12_DEPTH_STENCIL_DESC,       D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL>;
        using D3D12StateRTFormats =     D3D12StateSubobject<D3D12_RT_FORMAT_ARRAY,          D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RENDER_TARGET_FORMATS>;
        using D3D12StateDepthFormat =   D3D12StateSubobject<DXGI_FORMAT,                    D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL_FORMAT>;
        using D3D12StateTopology =      D3D12StateSubobject<D3D12_PRIMITIVE_TOPOLOGY_TYPE,  D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PRIMITIVE_TOPOLOGY>;
        using D3D12StateInputLayout =   D3D12StateSubobject<D3D12_INPUT_LAYOUT_DESC,        D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_INPUT_LAYOUT>;

        struct VertexRasterPipelineStream
        {
            D3D12StateRootsig rootSignature;
            D3D12StateVSBytecode vertexShaderBytecode;
            D3D12StatePSBytecode pixelShaderBytecode;
            D3D12StateRasterizer rasterizer;
            D3D12StateBlend blend;
            D3D12StateDepthStencil depthStencil;
            D3D12StateRTFormats rtFormats;
            D3D12StateDepthFormat depthFormat;
            D3D12StateTopology topology;
            D3D12StateInputLayout inputLayout;
        };

        struct MeshRasterPipelineStream
        {
            D3D12StateRootsig rootSignature;

            D3D12StateASBytecode amplificationShaderBytecode;
            D3D12StateMSBytecode meshShaderBytecode;
            D3D12StatePSBytecode pixelShaderBytecode;

            D3D12StateRasterizer rasterizer;
            D3D12StateBlend blend;
            D3D12StateDepthStencil depthStencil;
            D3D12StateRTFormats rtFormats;
            D3D12StateDepthFormat depthFormat;
            D3D12StateTopology topology;
        };

        struct ComputePipelineStream
        {
            D3D12StateRootsig rootSignature;
            D3D12StateCSBytecode computeShaderBytecode;
        };

        D3D12_SHADER_BYTECODE ToShaderBytecode(const ShaderBytecode& bytecode)
        {
            return {
                .pShaderBytecode = bytecode.data(),
                .BytecodeLength = bytecode.size()
            };
        }

        D3D12_RT_FORMAT_ARRAY ToRTFormatArray(std::initializer_list<RenderTargetFormat> formats)
        {
            RN_ASSERT(formats.size() <= RN_ARRAY_SIZE(D3D12_RT_FORMAT_ARRAY::RTFormats));

            D3D12_RT_FORMAT_ARRAY rtFormats = {
                .RTFormats = {},
                .NumRenderTargets = UINT(formats.size())
            };

            uint32_t idx = 0;
            for (RenderTargetFormat fmt : formats)
            {
                rtFormats.RTFormats[idx++] = ToDXGIFormat(fmt);
            }

            return rtFormats;
        }
    }

    RasterPipeline DeviceD3D12::CreateRasterPipeline(const VertexRasterPipelineDesc& desc) 
    {
        VertexRasterPipelineStream stream
        {
            .rootSignature = _bindlessRootSignature,
            .vertexShaderBytecode = ToShaderBytecode(desc.vertexShaderBytecode),
            .pixelShaderBytecode = ToShaderBytecode(desc.pixelShaderBytecode),
            .rasterizer = RASTERIZER_STATES[int(desc.rasterizerState)],
            .blend = BLEND_STATES[int(desc.blendState)],
            .depthStencil = DEPTH_STATES[int(desc.depthState)],
            .rtFormats = ToRTFormatArray(desc.renderTargetFormats),
            .depthFormat = ToDXGIFormat(desc.depthFormat),
            .topology = TOPOLOGY_TYPES[int(desc.topology)],
            .inputLayout = TestFlag(desc.flags, RasterPipelineFlags::RequiresInputLayoutForDrawID) ?
                GPU_DRIVEN_INPUT_LAYOUT :
                EMPTY_INPUT_LAYOUT
        };

        const D3D12_PIPELINE_STATE_STREAM_DESC streamDesc =
        {
            .SizeInBytes = sizeof(stream),
            .pPipelineStateSubobjectStream = &stream
        };

        ID3D12PipelineState* pso = nullptr;
        RN_ASSERT(SUCCEEDED(_d3dDevice->CreatePipelineState(&streamDesc, __uuidof(ID3D12PipelineState), (void**)&pso)));

        RasterPipelineData data = {
            .topology = desc.topology
        };

        return _rasterPipelines.Store(std::move(pso), std::move(data));
    }

    RasterPipeline DeviceD3D12::CreateRasterPipeline(const MeshRasterPipelineDesc& desc) 
    {
        MeshRasterPipelineStream stream = {
            .rootSignature = _bindlessRootSignature,
            .amplificationShaderBytecode = ToShaderBytecode(desc.amplificationShaderBytecode),
            .meshShaderBytecode = ToShaderBytecode(desc.meshShaderBytecode),
            .pixelShaderBytecode = ToShaderBytecode(desc.pixelShaderBytecode),
            .rasterizer = RASTERIZER_STATES[int(desc.rasterizerState)],
            .blend = BLEND_STATES[int(desc.blendState)],
            .depthStencil = DEPTH_STATES[int(desc.depthState)],
            .rtFormats = ToRTFormatArray(desc.renderTargetFormats),
            .depthFormat = ToDXGIFormat(desc.depthFormat),
            .topology = TOPOLOGY_TYPES[int(desc.topology)]
        };

        const D3D12_PIPELINE_STATE_STREAM_DESC streamDesc =
        {
            .SizeInBytes = sizeof(stream),
            .pPipelineStateSubobjectStream = &stream
        };

        ID3D12PipelineState* pso = nullptr;
        RN_ASSERT(SUCCEEDED(_d3dDevice->CreatePipelineState(&streamDesc, __uuidof(ID3D12PipelineState), (void**)&pso)));

        RasterPipelineData data = {
            .topology = desc.topology
        };

        return _rasterPipelines.Store(std::move(pso), std::move(data));
    }

    ComputePipeline DeviceD3D12::CreateComputePipeline(const ComputePipelineDesc& desc) 
    {
        ComputePipelineStream stream = 
        {
            .rootSignature = _bindlessRootSignature,
            .computeShaderBytecode = ToShaderBytecode(desc.computeShaderBytecode)
        };

        const D3D12_PIPELINE_STATE_STREAM_DESC streamDesc =
        {
            .SizeInBytes = sizeof(stream),
            .pPipelineStateSubobjectStream = &stream
        };

        ID3D12PipelineState* pso = nullptr;
        RN_ASSERT(SUCCEEDED(_d3dDevice->CreatePipelineState(&streamDesc, __uuidof(ID3D12PipelineState), (void**)&pso)));

        return _computePipelines.Store(std::move(pso));
    }

    namespace
    {
        namespace
    {
        WString ToWString(const char* str)
        {
            size_t len = strlen(str);
            WString wstr(len, L' ');
            if (len > 0)
            {
                size_t numConverted = 0;
                mbstowcs_s(&numConverted, wstr.data(), wstr.length() + 1, str, len);
            }

            return wstr;
        }
    }
    }

    RTPipeline DeviceD3D12::CreateRTPipeline(const RTPipelineDesc& desc) 
    {
        // 6 default subobjects + 1 per miss export + 2 per hit group (one library and one hit group)
        const size_t maxSubobjectCount = 6 + desc.missExports.size() + desc.hitGroups.size() * 2;  

        // ray gen + 1 per miss + 2 per hit group (closest-hit and any-hit)
        const size_t maxExportCount = 1 + desc.missExports.size() + desc.hitGroups.size() * 2; 
        const size_t maxNameCount = maxExportCount + desc.hitGroups.size();

        MemoryScope SCOPE;
        ScopedVector<D3D12_STATE_SUBOBJECT> subObjects;
        ScopedVector<D3D12_DXIL_LIBRARY_DESC> hitGroupLibraries;
        ScopedVector<D3D12_HIT_GROUP_DESC> hitGroups;
        ScopedVector<WString> exportNames;
        ScopedVector<const wchar_t*> exportNamePtrs;
        ScopedVector<D3D12_EXPORT_DESC> exports;

        subObjects.reserve(maxSubobjectCount);
        exportNames.reserve(maxNameCount);
        exportNamePtrs.reserve(maxNameCount);
        exports.reserve(maxExportCount);
        hitGroups.reserve(desc.hitGroups.size());
        hitGroupLibraries.reserve(desc.hitGroups.size());

        // Shader Config
        D3D12_RAYTRACING_SHADER_CONFIG shaderConfig = {
            .MaxPayloadSizeInBytes = desc.maxPayloadSize,
            .MaxAttributeSizeInBytes = D3D12_RAYTRACING_MAX_ATTRIBUTE_SIZE_IN_BYTES,
        };

        uint32_t shaderConfigIdx = uint32_t(subObjects.size());
        subObjects.push_back({
            .Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG,
            .pDesc = &shaderConfig,
        });

        // Pipeline Config
        D3D12_RAYTRACING_PIPELINE_CONFIG pipelineConfig = {
            .MaxTraceRecursionDepth = desc.maxRecursionDepth
        };
        subObjects.push_back({
            .Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG,
            .pDesc = &pipelineConfig,
        });

        // Global root signature
        subObjects.push_back({
            .Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE,
            .pDesc = &_bindlessRootSignature,
        });

        // Local root signature
        uint32_t localRootsigIdx = uint32_t(subObjects.size());
        subObjects.push_back({
            .Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE,
            .pDesc = &_rtLocalRootSignature,
        });

        // Ray gen and miss library
        uint32_t rayGenAndMissExportStartIdx = uint32_t(exports.size());
        uint32_t numRaygenMissExports = uint32_t(desc.missExports.size());
        if (desc.rayGenExport)
        {
            WString& exportName = exportNames.emplace_back(ToWString(desc.rayGenExport));
            exports.push_back({
                .Name = exportName.c_str(),
                .Flags = D3D12_EXPORT_FLAG_NONE
            });

            ++numRaygenMissExports;
        }

        for (const char* missExport : desc.missExports)
        {
            WString& exportName = exportNames.emplace_back(ToWString(missExport));
            exports.push_back({
                .Name = exportName.c_str(),
                .Flags = D3D12_EXPORT_FLAG_NONE
            });
        }

        D3D12_DXIL_LIBRARY_DESC rayGenMissLibrary = {
            .DXILLibrary = ToShaderBytecode(desc.rayGenAndMissLibrary),
            .NumExports = numRaygenMissExports,
            .pExports = &exports[rayGenAndMissExportStartIdx],
        };

        subObjects.push_back({
            .Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY,
            .pDesc = &rayGenMissLibrary,
        });

        // Hit groups
        for (const RTHitGroupDesc& hitGroup : desc.hitGroups)
        {
            const wchar_t* anyHitExportName = nullptr;
            const wchar_t* closestHitExportName = nullptr;

            // Library
            uint32_t hitGroupExportStartIdx = uint32_t(exports.size());
            uint32_t numExports = 0;
            if (hitGroup.anyHitExport)
            {
                WString& exportName = exportNames.emplace_back(ToWString(hitGroup.anyHitExport));
                exports.push_back({
                    .Name = exportName.c_str(),
                    .Flags = D3D12_EXPORT_FLAG_NONE
                });

                anyHitExportName = exportName.c_str();
                ++numExports;
            }

            if (hitGroup.closestHitExport)
            {
                WString& exportName = exportNames.emplace_back(ToWString(hitGroup.closestHitExport));
                exports.push_back({
                    .Name = exportName.c_str(),
                    .Flags = D3D12_EXPORT_FLAG_NONE
                });

                closestHitExportName = exportName.c_str();
                ++numExports;
            }

            hitGroupLibraries.push_back({
                .DXILLibrary = ToShaderBytecode(hitGroup.library),
                .NumExports = numExports,
                .pExports = &exports[hitGroupExportStartIdx],
            });
            
            subObjects.push_back({
                .Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY,
                .pDesc = &hitGroupLibraries.back(),
            });

            // Hit group
            {
                WString& exportName = exportNames.emplace_back(ToWString(hitGroup.name));
                hitGroups.push_back({
                    .HitGroupExport = exportName.c_str(),
                    .Type = D3D12_HIT_GROUP_TYPE_TRIANGLES,
                    .AnyHitShaderImport = anyHitExportName,
                    .ClosestHitShaderImport = closestHitExportName,
                });

                subObjects.push_back({
                    .Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP,
                    .pDesc = &hitGroups.back(),
                });
            }
        }

        for (const WString& name : exportNames)
        {
            exportNamePtrs.push_back(name.c_str());
        }

         // Shader config association
        D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION shaderConfigToExports = {
            .pSubobjectToAssociate = &subObjects[shaderConfigIdx],
            .NumExports = static_cast<uint32_t>(exportNamePtrs.size()),
            .pExports = &exportNamePtrs[0],
        };

        subObjects.push_back({
            .Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION,
            .pDesc = &shaderConfigToExports,
        });

        // Local root signature association
        D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION localRootSigToExports = {
            .pSubobjectToAssociate = &subObjects[localRootsigIdx],
            .NumExports = static_cast<uint32_t>(exportNamePtrs.size()),
            .pExports = &exportNamePtrs[0],
        };

        subObjects.push_back({
            .Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION,
            .pDesc = &localRootSigToExports,
        });

        D3D12_STATE_OBJECT_DESC soDesc = {
            .Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE,
            .NumSubobjects = uint32_t(subObjects.size()),
            .pSubobjects = &subObjects[0],
        };

        ID3D12StateObject* rtpso = nullptr;
        RN_ASSERT(SUCCEEDED(_d3dDevice->CreateStateObject(&soDesc, IID_PPV_ARGS(&rtpso))));

        return _rtPipelines.Store(std::move(rtpso));
    }

    void DeviceD3D12::Destroy(RasterPipeline pipeline) 
    {
        QueueFrameFinalizerAction([](DeviceD3D12* device, void* data)
            {
                RasterPipeline p = RasterPipeline(ptrdiff_t(data));

                ID3D12PipelineState* pso = device->_rasterPipelines.GetHot(p);
                if (pso)
                {
                    pso->Release();
                }

                device->_rasterPipelines.Remove(p);
            },
            reinterpret_cast<void*>(pipeline));
    }

    void DeviceD3D12::Destroy(ComputePipeline pipeline) 
    {
        QueueFrameFinalizerAction([](DeviceD3D12* device, void* data)
            {
                ComputePipeline p = ComputePipeline(ptrdiff_t(data));

                ID3D12PipelineState* pso = device->_computePipelines.GetHot(p);
                if (pso)
                {
                    pso->Release();
                }

                device->_computePipelines.Remove(p);
            },
            reinterpret_cast<void*>(pipeline));
    }

    void DeviceD3D12::Destroy(RTPipeline pipeline) 
    {
        QueueFrameFinalizerAction([](DeviceD3D12* device, void* data)
            {
                RTPipeline p = RTPipeline(ptrdiff_t(data));

                ID3D12StateObject* pso = device->_rtPipelines.GetHot(p);
                if (pso)
                {
                    pso->Release();
                }

                device->_rtPipelines.Remove(p);
            },
            reinterpret_cast<void*>(pipeline));
    }

}