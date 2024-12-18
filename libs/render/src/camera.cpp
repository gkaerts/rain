#include "render/camera.hpp"


namespace rn::render
{
    constexpr const bool USE_REFERENCE_PATH_FOR_CAMERA_TRANSFORMS = false;

    // All transform are left-handed!
    float4x4 WorldToView(const PerspectiveCamera& camera)
    {
        float3 lookVector = { camera.lookVector.x, camera.lookVector.y, camera.lookVector.z };
        float3 upVector = { camera.upVector.x, camera.upVector.y, camera.upVector.z };
        float3 position = { camera.position.x, camera.position.y, camera.position.z };

        float3 r2 = normalize(lookVector);
        float3 r0 = normalize(cross(upVector, r2));
        float3 r1 = cross(r2, r0);

        float3 negPos = -position;
        float1 d0 = dot(r0, negPos);
        float1 d1 = dot(r1, negPos);
        float1 d2 = dot(r2, negPos);

        float4x4 xform;
        xform[0] = float4(r0.xyz, d0.x);
        xform[1] = float4(r1.xyz, d1.x);
        xform[2] = float4(r2.xyz, d2.x);
        xform[3] = float4(0.0f, 0.0f, 0.0f, 1.0f);

        return transpose(xform);
    }

    float4x4 ViewToProjection(const PerspectiveCamera& camera, float aspectRatio)
    {
        float1 sinFoV = 0.0f;
        float1 cosFoV = 0.0f;
        sincos(0.5f * camera.verticalFoV, sinFoV, cosFoV);

        float range = camera.zFar / (camera.zFar - camera.zNear);
        float height = cosFoV / sinFoV;
        float width = height / aspectRatio;
    
        float4x4 xform;
        if constexpr(USE_REFERENCE_PATH_FOR_CAMERA_TRANSFORMS)
        {
            // Reference path - Everything is a SIMD vector load :(
            xform[0] = float4(width, 0.0f, 0.0f, 0.0f);
            xform[1] = float4(0.0f, height, 0.0f, 0.0f);
            xform[2] = float4(0.0f, 0.0f, range, 1.0f);
            xform[3] = float4(0.0f, 0.0f, -range * camera.zNear, 0.0f);
        }
        else
        {
            // Note to future self: This works because accessing individual vector components does not do a store into a fp register!
            // Swizzle accessors remain in SIMD vectors, so assignments translate directly into SIMD lane assignments
            float4 values = float4(width, height, range, -range * camera.zNear);
            xform[0].x = values.x;
            xform[1].y = values.y;
            xform[2].zw = float2(values.z, 1.0f);
            xform[3].z = values.w;
        }

        return xform;
    }

    float4x4 WorldToProjection(const PerspectiveCamera& camera, float aspectRatio)
    {
        return mul(WorldToView(camera), ViewToProjection(camera, aspectRatio));
    }
}
