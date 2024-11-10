#pragma once

#include "common/common.hpp"
#include "rhi/limits.hpp"

#include <span>
#include <initializer_list>

namespace rn::rhi
{
    enum class DepthFormat : uint32_t;
    enum class RenderTargetFormat : uint32_t;

    enum class RasterizerState : uint32_t
    {
        SolidBackFaceCull,
        SolidFrontFaceCull,
        SolidNoCull,

        Count
    };

    enum class BlendState : uint32_t
    {
        Disabled,
        AlphaBlend,
        Additive,
        Count
    };

    enum class DepthState : uint32_t
    {
        Disabled,
        DepthWriteGreaterEqual,
        DepthWriteLessEqual,
        DepthReadGreaterEqual,
        DepthReadLessEqual,
        DepthReadEqual,
        Count
    };

    enum class TopologyType : uint32_t
    {
        TriangleList,
        LineList,

        Count
    };

    enum class RasterPipelineFlags : uint32_t
    {
        None = 0x0,
        RequiresInputLayoutForDrawID = 0x01
    };
    RN_DEFINE_ENUM_CLASS_BITWISE_API(RasterPipelineFlags)

    using ShaderBytecode = std::span<const char>;

    struct VertexRasterPipelineDesc
    {
        RasterPipelineFlags flags;

        ShaderBytecode vertexShaderBytecode;
        ShaderBytecode pixelShaderBytecode;

        RasterizerState rasterizerState;
        BlendState blendState;
        DepthState depthState;

        std::initializer_list<RenderTargetFormat> renderTargetFormats;
        DepthFormat depthFormat;

        TopologyType topology;
    };

    struct MeshRasterPipelineDesc
    {
        ShaderBytecode amplificationShaderBytecode;
        ShaderBytecode meshShaderBytecode;
        ShaderBytecode pixelShaderBytecode;

        RasterizerState rasterizerState;
        BlendState blendState;
        DepthState depthState;

        std::initializer_list<RenderTargetFormat> renderTargetFormats;
        DepthFormat depthFormat;

        TopologyType topology;
    };

    struct ComputePipelineDesc
    {
        ShaderBytecode computeShaderBytecode;
    };

    struct RTHitGroupDesc
    {
        const char* name;

        ShaderBytecode library;
        const char* closestHitExport;
        const char* anyHitExport;
    };

    struct RTPipelineDesc
    {
        uint32_t maxPayloadSize;
        uint32_t maxRecursionDepth;

        ShaderBytecode rayGenAndMissLibrary;
        const char* rayGenExport;
        std::initializer_list<const char*> missExports;
        std::initializer_list<RTHitGroupDesc> hitGroups;
    };
}