#pragma once

#include "common/common.hpp"
#include "common/math/math.hpp"

#include "camera_gen.hpp"

namespace rn::render
{
    using PerspectiveCamera = schema::PerspectiveCamera;

    float4x4 WorldToView(const PerspectiveCamera& camera);
    float4x4 ViewToProjection(const PerspectiveCamera& camera, float aspectRatio);
    float4x4 WorldToProjection(const PerspectiveCamera& camera, float aspectRatio);
}