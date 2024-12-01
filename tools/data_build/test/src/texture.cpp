#include <gtest/gtest.h>

#include "asset/registry.hpp"
#include "data/texture.hpp"

#include "rhi/device.hpp"
#include "rhi/rhi_d3d12.hpp"
#include "rhi/format.hpp"

using namespace rn;

TEST(DataBuildTests_Texture, IntegrationTest_Texture)
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

    data::Texture texture = registry.Load<data::Texture>("textures/texture.texture");
    EXPECT_NE(texture, data::Texture::Invalid);

    const data::TextureData* textureData = registry.Resolve<data::Texture, data::TextureData>(texture);
    EXPECT_NE(textureData, nullptr);
    EXPECT_EQ(textureData->type, data::TextureType::Texture2D);
    EXPECT_NE(textureData->gpuRegion.allocation, rhi::GPUAllocation::Invalid);
    EXPECT_EQ(textureData->format, rhi::TextureFormat::BC7);
    EXPECT_EQ(textureData->texture2D.width, 512);
    EXPECT_EQ(textureData->texture2D.height, 512);
    EXPECT_EQ(textureData->texture2D.mipLevels, 10);
    EXPECT_NE(textureData->texture2D.rhiTexture, rhi::Texture2D::Invalid);
    EXPECT_NE(textureData->texture2D.rhiView, rhi::Texture2DView::Invalid);
}