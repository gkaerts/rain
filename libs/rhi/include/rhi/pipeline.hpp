#pragma once

#include "common/common.hpp"
#include "common/memory/span.hpp"
#include "rhi/limits.hpp"

namespace rn::rhi
{
    enum class DepthFormat : uint32_t;
    enum class RenderTargetFormat : uint32_t;

    using ShaderBytecode = Span<const uint8_t>;

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

    struct VertexRasterPipelineDesc
    {
        RasterPipelineFlags flags;

        ShaderBytecode vertexShaderBytecode;
        ShaderBytecode pixelShaderBytecode;

        RasterizerState rasterizerState;
        BlendState blendState;
        DepthState depthState;

        Span<const RenderTargetFormat> renderTargetFormats;
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

        Span<const RenderTargetFormat> renderTargetFormats;
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
        Span<const char*> missExports;
        Span<RTHitGroupDesc> hitGroups;
    };
}