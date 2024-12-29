#include "common/math/math.hpp"

namespace rn
{
    float4x4 affine(float3 origin, quaternion rotation)
    {
        float4x4 xform = float4x4(rotation);
        xform[3] = float4(origin.xyz, 1.0f);

        return xform;
    }

    float4x4 affine(float3 origin, quaternion rotation, float3 scale)
    {
        float4x4 matScale = {
            float4(scale.x, 0.0f, 0.0f, 0.0f),
            float4(0.0f, scale.y, 0.0f, 0.0f),
            float4(0.0f, 0.0f, scale.z, 0.0f),
            float4(0.0f, 0.0f, 0.0f, 1.0f)
        };

        return matScale * affine(origin, rotation);
    }
}