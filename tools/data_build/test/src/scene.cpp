#include <gtest/gtest.h>

#include "asset/registry.hpp"

#include "scene/asset.hpp"
#include "scene/scene.hpp"

#include "data/geometry.hpp"
#include "data/material.hpp"
#include "data/material_shader.hpp"
#include "data/texture.hpp"

#include "rhi/device.hpp"
#include "rhi/rhi_d3d12.hpp"

#include "child_of_gen.hpp"
#include "transform_gen.hpp"
#include "lights_gen.hpp"

#include "mesh_gen.hpp"

using namespace rn;

TEST(DataBuildTests_Scene, IntegrationTest_Scene)
{
    rhi::Device* device = rhi::CreateD3D12Device({
            .adapterIndex = 0,
            .enableDebugLayer = true
        },
        rhi::DefaultDeviceMemorySettings());

    asset::Registry registry({
        .contentPrefix = "gen/data_build_tests/",
        .enableMultithreadedLoad = true,
        .onMapAsset = asset::MapFileAsset
    });

    data::GeometryBuilder geometryBuilder(device);
    registry.RegisterAssetType<data::GeometryData>({
        .extensionHash = HashString(".geometry"),
        .initialCapacity = 32,
        .builder = &geometryBuilder
    });

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

    scene::SceneBuilder sceneBuilder;
    registry.RegisterAssetType<scene::Scene>({
        .extensionHash = HashString(".scene"),
        .initialCapacity = 32,
        .builder = &sceneBuilder
    });

    const std::string_view scenePath = "scene/scene.scene";
    asset::AssetIdentifier sceneId = asset::MakeAssetIdentifier(scenePath);
    registry.Load(scenePath);
    

    scene::Scene* scene = registry.Resolve<scene::Scene>(sceneId);
    
    MemoryScope scope;
    auto query0 = scene->Query<scene::schema::Transform, render::schema::OmniLight>(scope);
    ASSERT_EQ(query0.size(), 1);
    for (auto [transform, light] : query0)
    {
        ASSERT_FLOAT_EQ(light.radius, 10.0f);
        ASSERT_FLOAT_EQ(light.sourceRadius, 0.5f);
        ASSERT_FLOAT_EQ(light.intensity, 5.0f);
        ASSERT_FLOAT_EQ(light.color.r, 1.0f);
        ASSERT_FLOAT_EQ(light.color.g, 1.0f);
        ASSERT_FLOAT_EQ(light.color.b, 0.0f);
        ASSERT_FALSE(light.castShadow);

        ASSERT_FLOAT_EQ(transform.localPosition.x, 0.0f);
        ASSERT_FLOAT_EQ(transform.localPosition.y, 0.0f);
        ASSERT_FLOAT_EQ(transform.localPosition.z, 0.0f);
    }

    auto query1 = scene->Query<scene::schema::Transform, scene::schema::ChildOf>(scope);
    ASSERT_EQ(query1.size(), 1);
    for (auto [transform, childOf] : query1)
    {
        auto [light] = scene->Components<render::schema::OmniLight>(childOf.parent);
        ASSERT_FLOAT_EQ(light.radius, 10.0f);
    }

    auto query2 = scene->Query<scene::schema::Transform, render::schema::RenderMesh, render::schema::RenderMaterialList>(scope);
    ASSERT_EQ(query2.size(), 1);
    for (auto [transform, mesh, materials] : query2)
    {
        ASSERT_FLOAT_EQ(transform.localPosition.x, 10.0f);
        ASSERT_FLOAT_EQ(transform.localPosition.y, 5.0f);
        ASSERT_FLOAT_EQ(transform.localPosition.z, 0.0f);

        ASSERT_EQ(mesh.geometry, asset::MakeAssetIdentifier("geometry/monkey.geometry"));
        ASSERT_EQ(materials.materials[0], asset::MakeAssetIdentifier("materials/material.material"));
    }
}