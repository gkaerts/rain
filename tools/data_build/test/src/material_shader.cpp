#include <gtest/gtest.h>

#include "asset/registry.hpp"
#include "data/material_shader.hpp"
#include "data/texture.hpp"

#include "rhi/device.hpp"
#include "rhi/rhi_d3d12.hpp"
#include "rhi/format.hpp"

using namespace rn;

TEST(DataBuildTests_MaterialShader, IntegrationTest_MaterialShader)
{
    asset::Registry registry({
        .contentPrefix = "gen/data_build_tests/",
        .enableMultithreadedLoad = true,
        .onMapAsset = asset::MapFileAsset
    });

    rhi::Device* device = rhi::CreateD3D12Device({
            .adapterIndex = 0,
            .enableDebugLayer = true
        },
        rhi::DefaultDeviceMemorySettings());

    // Need a texture builder to handle dependencies!
    data::TextureBuilder textureBuilder(device);
    registry.RegisterAssetType<data::TextureData>({
        .extensionHash = HashString(".texture"),
        .initialCapacity = 32,
        .builder = &textureBuilder
    });

    data::MaterialShaderBuilder shaderBuilder(device);
    registry.RegisterAssetType<data::MaterialShaderData>({
        .extensionHash = HashString(".material_shader"),
        .initialCapacity = 32,
        .builder = &shaderBuilder
    });

    constexpr const std::string_view shaderPath = "material_shaders/material_shader.material_shader";
    asset::AssetIdentifier shaderId = asset::MakeAssetIdentifier(shaderPath);
    registry.Load(shaderPath);

    const data::MaterialShaderData* shaderData = registry.Resolve<data::MaterialShaderData>(shaderId);
    EXPECT_EQ(shaderData->rasterPasses.size(), 1);
    EXPECT_EQ(shaderData->parameters.size(), 2);
    EXPECT_NE(shaderData->uniformBufferSize, 0);
    
    const data::MaterialShaderRasterPass& pass = shaderData->rasterPasses[0];
    EXPECT_NE(pass.pipeline, rhi::RasterPipeline::Invalid);
    EXPECT_EQ(pass.renderPass, data::MaterialRenderPass::Forward);
    EXPECT_EQ(pass.type, data::RasterPassType::Vertex);
    
    asset::AssetIdentifier textureId = asset::MakeAssetIdentifier("textures/texture.texture");
    const data::MaterialShaderParameter& param0 = shaderData->parameters[0];
    EXPECT_EQ(param0.type, data::MaterialShaderParameterType::Texture);
    EXPECT_EQ(param0.name, "ColorTexture");
    EXPECT_EQ(param0.pTexture.type, data::TextureType::Texture2D);
    EXPECT_EQ(param0.pTexture.defaultValue, textureId);

    const data::MaterialShaderParameter& param1 = shaderData->parameters[1];
    EXPECT_EQ(param1.type, data::MaterialShaderParameterType::FloatVec);
    EXPECT_EQ(param1.name, "Color");
    EXPECT_EQ(param1.pFloatVec.dimension, 4);
    EXPECT_FLOAT_EQ(param1.pFloatVec.defaultValue[0], 1.0f);
    EXPECT_FLOAT_EQ(param1.pFloatVec.defaultValue[1], 1.0f);
    EXPECT_FLOAT_EQ(param1.pFloatVec.defaultValue[2], 1.0f);
    EXPECT_FLOAT_EQ(param1.pFloatVec.defaultValue[3], 1.0f);
    EXPECT_FLOAT_EQ(param1.pFloatVec.minValue[0], 0.0f);
    EXPECT_FLOAT_EQ(param1.pFloatVec.minValue[1], 0.0f);
    EXPECT_FLOAT_EQ(param1.pFloatVec.minValue[2], 0.0f);
    EXPECT_FLOAT_EQ(param1.pFloatVec.minValue[3], 0.0f);
    EXPECT_FLOAT_EQ(param1.pFloatVec.maxValue[0], 1.0f);
    EXPECT_FLOAT_EQ(param1.pFloatVec.maxValue[1], 1.0f);
    EXPECT_FLOAT_EQ(param1.pFloatVec.maxValue[2], 1.0f);
    EXPECT_FLOAT_EQ(param1.pFloatVec.maxValue[3], 1.0f);
}