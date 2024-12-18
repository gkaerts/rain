#include <gtest/gtest.h>

#include "gpu_test.hpp"
#include "rhi/device.hpp"
#include "rhi/rhi_d3d12.hpp"

namespace FrustumShader
{
    #include "unit_test/test_frustum.d3d12.hpp"
}


using namespace rn;
struct GPUUnitTests : public ::testing::Test
{
    protected:

    virtual void SetUp() override
    {
        rhi::DeviceD3D12Options options =
        {
            .adapterIndex = 0,
            .enableDebugLayer = true,
            .enableGPUValidation = true
        };

        device = rhi::CreateD3D12Device(options, rhi::DefaultDeviceMemorySettings());
    }

    virtual void TearDown() override
    {
        rhi::DestroyDevice(device);
    }

    rhi::Device* device = nullptr;
};


TEST_F(GPUUnitTests, FrustumTests)
{
    RunGPUUnitTest(device, FrustumShader::CS, uint4());
}