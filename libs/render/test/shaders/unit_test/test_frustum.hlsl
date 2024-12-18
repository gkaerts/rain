#include "unit_test/test.hlsli"
#include "visibility/visibility.hlsli"

SHADER_TEST(frustumSphereTest, 'F', 'r', 'u','s','t','u','m', '_', 'S','p','h','e','r','e', ' ', ' ')
{
    // Frustum setup:
    // - Plane angles at 45 degrees
    // - Near value at 0.1, far value at 100
    // - Frustum width and height at far value is 200
    float3 cornerA = float3(-1, 1, 1);
    float3 cornerB = float3(1, 1, 1);
    float3 cornerC = float3(1, -1, 1);
    float3 cornerD = float3(-1, -1, 1);

    Frustum f;
    f.plane0 = MakeFrustumPlane(cornerA, cornerB);
    f.plane1 = MakeFrustumPlane(cornerB, cornerC);
    f.plane2 = MakeFrustumPlane(cornerC, cornerD);
    f.plane3 = MakeFrustumPlane(cornerD, cornerA);
    f.minMaxZ = float4(0.1f, 100.0f, 0.0f, 0.0f);

    // Sphere far away from frustum with non-intersecting radius. Query should return false.
    float4 sphere = float4(1000.0, 1000.0, 10.0, 5.0);
    EXPECT_FALSE(FrustumTest(f, sphere));

    // Sphere just in front of frustum, not touchning the near plane. Query should return false.
    sphere = float4(0.0, 0.0, -1.0, 1.05);
    EXPECT_FALSE(FrustumTest(f, sphere));

    // Sphere just behind the frustum, not touching the far plane. Query should return false.
    sphere = float4(0.0, 0.0, 110.0, 4.9);
    EXPECT_FALSE(FrustumTest(f, sphere));

    // Sphere just above top frustum plane, but within near/far bounds. Query should return false.
    sphere = float4(0.0, 3.0, 2.0, 0.7);
    EXPECT_FALSE(FrustumTest(f, sphere));

    // Sphere intersecting the near plane. Query should return true.
    sphere = float4(0.0, 0.0, -1.0, 5.0);
    EXPECT_TRUE(FrustumTest(f, sphere));

    // Sphere intersecting the far plane. Query should return true.
    sphere = float4(0.0, 0.0, 110.0, 15.0);
    EXPECT_TRUE(FrustumTest(f, sphere));

    // Sphere fully encompassed by frustum. Query should return true.
    sphere = float4(0.0, 0.0, 5.0, 2.0);
    EXPECT_TRUE(FrustumTest(f, sphere));

    // Sphere intersecting top frustum plane, but within near/far bounds. Query should return true.
    sphere = float4(0.0, 3.0, 2.0, 1.5);
    EXPECT_TRUE(FrustumTest(f, sphere));
}


BEGIN_SHADER_TEST_LIST
    RUN_SHADER_TEST(frustumSphereTest)
END_SHADER_TEST_LIST