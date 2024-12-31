#include <gtest/gtest.h>

#include "asset/registry.hpp"

#include "scene/asset.hpp"
#include "scene/scene.hpp"

#include "child_of_gen.hpp"
#include "transform_gen.hpp"
#include "lights_gen.hpp"


using namespace rn;

TEST(DataBuildTests_Scene, IntegrationTest_Scene)
{
    asset::Registry registry({
        .contentPrefix = "gen/data_build_tests/",
        .enableMultithreadedLoad = true,
        .onMapAsset = asset::MapFileAsset
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
}