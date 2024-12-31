#include <gtest/gtest.h>

#include "asset/registry.hpp"
#include "data/geometry.hpp"

#include "rhi/device.hpp"
#include "rhi/rhi_d3d12.hpp"

using namespace rn;

TEST(DataBuildTests_Geometry, IntegrationTest_Monkey)
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

    data::GeometryBuilder geometryBuilder(device);
    registry.RegisterAssetType<data::GeometryData>({
        .extensionHash = HashString(".geometry"),
        .initialCapacity = 32,
        .builder = &geometryBuilder
    });

    constexpr const std::string_view geometryPath = "geometry/monkey.Suzanne.geometry";
    asset::AssetIdentifier geometryId = asset::MakeAssetIdentifier(geometryPath);
    registry.Load(geometryPath);

    const data::GeometryData* monkeyData = registry.Resolve<data::GeometryData>(geometryId);
    EXPECT_NE(monkeyData, nullptr);
    EXPECT_NE(monkeyData->dataGPURegion.allocation, rhi::GPUAllocation::Invalid);
    EXPECT_NE(monkeyData->dataBuffer, rhi::Buffer::Invalid);

    if (device->Capabilities().hasHardwareRaytracing)
    {
        EXPECT_NE(monkeyData->blasGPURegion.allocation, rhi::GPUAllocation::Invalid);
        EXPECT_NE(monkeyData->blasBuffer, rhi::Buffer::Invalid);
        EXPECT_NE(monkeyData->blas, rhi::BLASView::Invalid);
    }

    EXPECT_NE(monkeyData->positions.view, rhi::BufferView::Invalid);
    EXPECT_NE(monkeyData->normals.view, rhi::BufferView::Invalid);
    EXPECT_NE(monkeyData->tangents.view, rhi::BufferView::Invalid);
    EXPECT_NE(monkeyData->uvs.view, rhi::BufferView::Invalid);
    EXPECT_EQ(monkeyData->colors.view, rhi::BufferView::Invalid);

    EXPECT_EQ(monkeyData->parts.size(), 1);
    for (const data::GeometryPart& part : monkeyData->parts)
    {
        EXPECT_NE(part.indices, rhi::BufferView::Invalid);
    }
}

TEST(DataBuildTests_Geometry, IntegrationTest_CubeSphere)
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

    data::GeometryBuilder geometryBuilder(device);
    registry.RegisterAssetType<data::GeometryData>({
        .extensionHash = HashString(".geometry"),
        .initialCapacity = 32,
        .builder = &geometryBuilder
    });

    constexpr const std::string_view geometryPath = "geometry/cube_sphere.Cube.geometry";
    asset::AssetIdentifier geometryId = asset::MakeAssetIdentifier(geometryPath);
    registry.Load(geometryPath);

    const data::GeometryData* cubeSphereData = registry.Resolve<data::GeometryData>(geometryId);
    EXPECT_NE(cubeSphereData, nullptr);
    EXPECT_NE(cubeSphereData->dataGPURegion.allocation, rhi::GPUAllocation::Invalid);
    EXPECT_NE(cubeSphereData->dataBuffer, rhi::Buffer::Invalid);

    if (device->Capabilities().hasHardwareRaytracing)
    {
        EXPECT_NE(cubeSphereData->blasGPURegion.allocation, rhi::GPUAllocation::Invalid);
        EXPECT_NE(cubeSphereData->blasBuffer, rhi::Buffer::Invalid);
        EXPECT_NE(cubeSphereData->blas, rhi::BLASView::Invalid);
    }

    EXPECT_NE(cubeSphereData->positions.view, rhi::BufferView::Invalid);
    EXPECT_NE(cubeSphereData->normals.view, rhi::BufferView::Invalid);
    EXPECT_NE(cubeSphereData->tangents.view, rhi::BufferView::Invalid);
    EXPECT_NE(cubeSphereData->uvs.view, rhi::BufferView::Invalid);
    EXPECT_EQ(cubeSphereData->colors.view, rhi::BufferView::Invalid);

    EXPECT_EQ(cubeSphereData->parts.size(), 2);
    for (const data::GeometryPart& part : cubeSphereData->parts)
    {
        EXPECT_NE(part.indices, rhi::BufferView::Invalid);
    }
}