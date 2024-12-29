#pragma once

#include "hlsl++.h"

namespace rn
{
    using namespace hlslpp;

    const float PI = 3.14159265f;
    const float TWO_PI = 6.28318530f;
    const float HALF_PI = 1.57079632f;

    const float ONE_OVER_PI = 0.31830988f;
    const float TWO_OVER_PI = 0.63661977f;

    float4x4 affine(float3 origin, quaternion rotation);
    float4x4 affine(float3 origin, quaternion rotation, float3 scale);
}