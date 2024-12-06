#pragma once

#include "common/common.hpp"
#include "common/math/math.hpp"

namespace rn::render
{
    struct PerspectiveCamera
    {
        float3 position;
        float3 lookVector;
        float3 upVector;

        float zNear;
        float zFar;
        float verticalFoV;
    };

    float4x4 WorldToView(const PerspectiveCamera& camera);
    float4x4 ViewToProjection(const PerspectiveCamera& camera, float aspectRatio);
    float4x4 WorldToProjection(const PerspectiveCamera& camera, float aspectRatio);
}