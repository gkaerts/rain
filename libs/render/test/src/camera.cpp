#include <gtest/gtest.h>

#include "common/common.hpp"
#include "render/camera.hpp"

using namespace rn;

TEST(CameraTests, PerspectiveCamera_WorldToViewTransforms)
{
    float4x4 camXform = affine(float3(0.0f, 5.0f, 0.0f), quaternion::identity());
    render::PerspectiveCamera camera = 
    {
        .zNear = 0.1f,
        .zFar = 1000.0f,
        .verticalFoV = HALF_PI
    };

    float4x4 worldToView = render::WorldToView(camXform, camera);
    float4 position = float4(0.0f, 5.0f, 1.0f, 1.0f);
    float4 viewPos = mul(position, worldToView);
    EXPECT_FLOAT_EQ(viewPos.f32[0], 0.0f);
    EXPECT_FLOAT_EQ(viewPos.f32[1], 0.0f);
    EXPECT_FLOAT_EQ(viewPos.f32[2], 1.0f);
    EXPECT_FLOAT_EQ(viewPos.f32[3], 1.0f);

    position = float4(-10.0f, 8.0f, -2.0f, 1.0f);
    viewPos = mul(position, worldToView);
    EXPECT_FLOAT_EQ(viewPos.f32[0], -10.0f);
    EXPECT_FLOAT_EQ(viewPos.f32[1], 3.0f);
    EXPECT_FLOAT_EQ(viewPos.f32[2], -2.0f);
    EXPECT_FLOAT_EQ(viewPos.f32[3], 1.0f);
}

TEST(CameraTests, PerspectiveCamera_ViewToProjectionTransforms)
{
    render::PerspectiveCamera camera = 
    {
        .zNear = 0.1f,
        .zFar = 1000.0f,
        .verticalFoV = HALF_PI
    };

    float4x4 viewToProjection = render::ViewToProjection(camera, 1.0f);
    float4 viewPos = float4(0.0f, 0.0f, 2.0f, 1.0f);
    float4 projPos = mul(viewPos, viewToProjection);
    projPos /= projPos.wwww;

    EXPECT_FLOAT_EQ(projPos.x, 0.0f);
    EXPECT_FLOAT_EQ(projPos.y, 0.0f);
    EXPECT_TRUE(projPos.z >= 0.0f && projPos.z <= 1.0f);
    EXPECT_FLOAT_EQ(projPos.w, 1.0f);
}

TEST(CameraTests, PerspectiveCamera_WorldToProjectionTransforms)
{
    float4x4 camXform = affine(float3(0.0f, 5.0f, 0.0f), quaternion::identity());
    render::PerspectiveCamera camera = 
    {
        .zNear = 0.1f,
        .zFar = 1000.0f,
        .verticalFoV = HALF_PI
    };

    float4x4 worldToProjection = render::WorldToProjection(camXform, camera, 1.0f);
    float4 position = float4(0.0f, 5.0f, 8.0f, 1.0f);
    float4 projPos = mul(position, worldToProjection);
    projPos /= projPos.wwww;

    EXPECT_FLOAT_EQ(projPos.x, 0.0f);
    EXPECT_FLOAT_EQ(projPos.y, 0.0f);
    EXPECT_TRUE(projPos.z >= 0.0f && projPos.z <= 1.0f);
    EXPECT_FLOAT_EQ(projPos.w, 1.0f);
}