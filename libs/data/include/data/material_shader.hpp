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

    // See texture.hpp
    enum class TextureType : uint32_t;

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
    
    struct TextureMaterialShaderParam
    {
        TextureType             type;
        asset::AssetIdentifier  defaultValue;
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
        uint32_t                                uniformBufferSize;
    };

    class MaterialShaderBuilder : public asset::Builder<MaterialShaderData>
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