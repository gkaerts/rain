#ifndef _VISIBILITY_HLSLI_
#define _VISIBILITY_HLSLI_

#include "shared/visibility/visibility.h"
#include "shared/samplers.h"

float4 MakeFrustumPlane(float3 cornerA, float3 cornerB)
{
    // Assume the third point to be at (0, 0, 0) in view space
    return float4(normalize(cross(cornerA, cornerB)), 1.0);
}

bool FrustumTest(Frustum f, float4 sphere)
{
    if (sphere.z + sphere.w >  f.minMaxZ.x &&
        sphere.z - sphere.w <= f.minMaxZ.y)
    {
        return  dot(sphere.xyz, f.plane0.xyz) < sphere.w &&
                dot(sphere.xyz, f.plane1.xyz) < sphere.w &&
                dot(sphere.xyz, f.plane2.xyz) < sphere.w &&
                dot(sphere.xyz, f.plane3.xyz) < sphere.w;
    }

    return false;
}

bool FrustumTest(Frustum f, float4x4 worldToView, float3 aabbMin, float3 aabbMax)
{
    float3 center = 0.5f * (aabbMin + aabbMax);
    float1 radius = length(aabbMax - center);

    center = mul(float4(center, 1.0f), worldToView).xyz;
    return FrustumTest(f, float4(center, radius));
}

bool HiZOcclusionTest(InstanceAABB aabb, float4x4 worldToProjection, float2 viewportDimensions, Texture2D<float4> hizTexture)
{
    // Calculate clip-space bounding rect
    const float3 aabbCorners[8] = {
        float3(aabb.minCorner.x, aabb.minCorner.y, aabb.minCorner.z),
        float3(aabb.minCorner.x, aabb.minCorner.y, aabb.maxCorner.z),
        float3(aabb.minCorner.x, aabb.maxCorner.y, aabb.minCorner.z),
        float3(aabb.minCorner.x, aabb.maxCorner.y, aabb.maxCorner.z),

        float3(aabb.maxCorner.x, aabb.minCorner.y, aabb.minCorner.z),
        float3(aabb.maxCorner.x, aabb.minCorner.y, aabb.maxCorner.z),
        float3(aabb.maxCorner.x, aabb.maxCorner.y, aabb.minCorner.z),
        float3(aabb.maxCorner.x, aabb.maxCorner.y, aabb.maxCorner.z),
    };

    const float LARGE_NUMBER = 10000000.0f;
    float3 clipRectA = float3( LARGE_NUMBER,  LARGE_NUMBER,  LARGE_NUMBER);
    float3 clipRectB = float3(-LARGE_NUMBER, -LARGE_NUMBER, -LARGE_NUMBER);
    for (int j = 0; j < 8; ++j)
    {
        float4 toClip = mul(float4(aabbCorners[j], 1.0), worldToProjection);
        toClip /= toClip.w;

        toClip.xy = toClip.xy * float2(0.5, -0.5) + 0.5;

        clipRectA = min(toClip.xyz, clipRectA);
        clipRectB = max(toClip.xyz, clipRectB);
    }

    // Get max clip-space depth (higher depth == closer to camera)
    float maxOccluderDepth = clipRectB.z;

    // Calculate mip LOD
    float width = (clipRectB.x - clipRectA.x) * viewportDimensions.x;
    float height = (clipRectB.y - clipRectA.y) * viewportDimensions.y;
    float mipLOD = ceil(log2(max(width, height)));

    float lowerMipLOD = max(mipLOD - 1, 0);
    float2 scale = exp2(-lowerMipLOD);
    float2 a = floor(clipRectA.xy*scale);
    float2 b = ceil(clipRectB.xy*scale);
    float2 dims = b - a;

    if (dims.x <= 2 && dims.y <= 2)
    {
        mipLOD = lowerMipLOD;
    }


    // Pull 4 samples from Hi-Z
    float2 uv0 = float2(clipRectA.x, clipRectA.y);
    float2 uv1 = float2(clipRectA.x, clipRectB.y);
    float2 uv2 = float2(clipRectB.x, clipRectA.y);
    float2 uv3 = float2(clipRectB.x, clipRectB.y);

    // TODO: If we ever get GatherRedLevel we could definitely use it here
    float2 hizSample0 = hizTexture.SampleLevel(PointClamp, uv0, mipLOD).xy;
    float2 hizSample1 = hizTexture.SampleLevel(PointClamp, uv1, mipLOD).xy;
    float2 hizSample2 = hizTexture.SampleLevel(PointClamp, uv2, mipLOD).xy;
    float2 hizSample3 = hizTexture.SampleLevel(PointClamp, uv3, mipLOD).xy;
    float maxSample = max(max(hizSample0.y, hizSample1.y), max(hizSample2.y, hizSample3.y));
    float minSample = min(min(hizSample0.x, hizSample1.x), min(hizSample2.x, hizSample3.x));

    return minSample < maxOccluderDepth;
}

#endif // _VISIBILITY_HLSLI_