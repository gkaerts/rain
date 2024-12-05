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
    registry.RegisterAssetType<data::Texture, data::TextureData>({
        .identifierHash = HashString(".texture"),
        .initialCapacity = 32,
        .builder = &textureBuilder
    });

    data::MaterialShaderBuilder shaderBuilder(device);
    registry.RegisterAssetType<data::MaterialShader, data::MaterialShaderData>({
        .identifierHash = HashString(".material_shader"),
        .initialCapacity = 32,
        .builder = &shaderBuilder
    });

    data::MaterialBuilder materialBuilder;
    registry.RegisterAssetType<data::Material, data::MaterialData>({
        .identifierHash = HashString(".material"),
        .initialCapacity = 32,
        .builder = &materialBuilder
    });

    data::Material material = registry.Load<data::Material>("materials/material.material");
    EXPECT_NE(material, data::Material::Invalid);

    const data::MaterialData* materialData = registry.Resolve<data::Material, data::MaterialData>(material);
    EXPECT_NE(materialData, nullptr);
    EXPECT_NE(materialData->shader, data::MaterialShader::Invalid);
    EXPECT_TRUE(!materialData->uniformData.empty());
    EXPECT_NE(materialData->uniformData.data(), nullptr);
}