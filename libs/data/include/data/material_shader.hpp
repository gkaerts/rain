#pragma once

#include "common/common.hpp"
#include "common/handle.hpp"

#include "common/memory/memory.hpp"
#include "common/memory/span.hpp"
#include "common/memory/string.hpp"

#include "common/math/math.hpp"

#include "rhi/handles.hpp"
#include "rhi/pipeline.hpp"

#include "asset/asset.hpp"

namespace rn::rhi
{
    class Device;
}

namespace rn::data
{
    RN_MEMORY_CATEGORY(Data)

    RN_HANDLE(Texture);
    RN_DEFINE_HANDLE(MaterialShader, 0x42)

    enum class MaterialRenderPass : uint32_t
    {
        Depth,
        GBuffer,
        VBuffer,
        Forward,
        DDGI,
        Reflections,

        Count
    };

    enum class MaterialShaderParameterType : uint32_t
    {
        Texture,
        FloatVec,
        UintVec,
        IntVec,

        Count
    };

    template <typename T>
    struct NumericMaterialShaderParam
    {
        static const size_t CAPACITY = 16;
        uint32_t dimension;
        T defaultValue[CAPACITY];
        T minValue[CAPACITY];
        T maxValue[CAPACITY];
    };

    enum class TextureMaterialShaderDimension : uint32_t
    {
        _2D = 0,
        _3D
    };
    
    struct TextureMaterialShaderParam
    {
        TextureMaterialShaderDimension dimension;
        Texture defaultValue;
    };

    struct MaterialShaderParameter
    {
        MaterialShaderParameterType type;

        String      name;
        uint32_t    offsetInMaterialData;

        union
        {
            TextureMaterialShaderParam          pTexture;
            NumericMaterialShaderParam<float>   pFloatVec;
            NumericMaterialShaderParam<uint>    pUintVec;
            NumericMaterialShaderParam<int>     pIntVec;
        };
        
    };

    enum class RasterPassType : uint32_t
    {
        Vertex = 0,
        Mesh
    };

    struct MaterialShaderRasterPass
    {
        RasterPassType      type;
        MaterialRenderPass  renderPass;
        rhi::RasterPipeline pipeline;
    };

    struct MaterialShaderRTPass
    {
        MaterialRenderPass  renderPass;
        String              closestHit;
        String              anyHit;
        String              hitGroup;
    };


    struct MaterialShaderData
    {
        Span<const MaterialShaderRasterPass>    rasterPasses;
        Span<const MaterialShaderRTPass>        rtPasses;
        Span<const MaterialShaderParameter>     parameters;
        rhi::ShaderBytecode                     rtLibrary;
    };

    class MaterialShaderBuilder : public asset::Builder<MaterialShader, MaterialShaderData>
    {
    public:

        MaterialShaderBuilder(rhi::Device* device);

        MaterialShaderData  Build(const asset::AssetBuildDesc& desc) override;
        void                Destroy(MaterialShaderData& data) override;
        void                Finalize() override;

    private:

        rhi::Device* _device = nullptr;
    };
}