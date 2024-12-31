#include <gtest/gtest.h>

#include "asset/registry.hpp"
#include "data/texture.hpp"
#include "data/material.hpp"
#include "data/material_shader.hpp"

#include "rhi/device.hpp"
#include "rhi/rhi_d3d12.hpp"
#include "rhi/format.hpp"

using namespace rn;

TEST(DataBuildTests_Material, IntegrationTest_Material)
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

    data::MaterialBuilder materialBuilder;
    registry.RegisterAssetType<data::MaterialData>({
        .extensionHash = HashString(".material"),
        .initialCapacity = 32,
        .builder = &materialBuilder
    });

    constexpr std::string_view materialPath = "materials/material.material";
    asset::AssetIdentifier materialId = asset::MakeAssetIdentifier(materialPath);
    registry.Load(materialPath);

    const data::MaterialData* materialData = registry.Resolve<data::MaterialData>(materialId);
    EXPECT_NE(materialData, nullptr);

    asset::AssetIdentifier shaderId = asset::MakeAssetIdentifier("material_shaders/material_shader.material_shader");
    EXPECT_EQ(materialData->shader, shaderId);
    EXPECT_TRUE(!materialData->uniformData.empty());
    EXPECT_NE(materialData->uniformData.data(), nullptr);
}