#include <gtest/gtest.h>

#include "common/log/log.hpp"
#include "common/memory/memory.hpp"
#include "rhi/device.hpp"
#include "rhi/format.hpp"
#include "rhi/rhi_d3d12.hpp"

using namespace rn;

RN_DEFINE_MEMORY_CATEGORY(GPUTest)

struct DeviceD3D12Tests : public ::testing::Test
{
    protected:

    virtual void SetUp() override
    {
        LoggerSettings logSettings = {};
        InitializeLogger(logSettings);

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
        TeardownLogger();
    }

    rhi::Device* device = nullptr;
};

TEST_F(DeviceD3D12Tests, CanCreateDevice)
{
    EXPECT_NE(device, nullptr);
}

namespace test_shaders
{
    #include "shaders/test_shaders_vs.hpp"
    #include "shaders/test_shaders_ps.hpp"
    #include "shaders/test_shaders_cs.hpp"
}

TEST_F(DeviceD3D12Tests, CanCreateAndDestroyRasterPipelines)
{
    rhi::RasterPipeline rasterPipeline = device->CreateRasterPipeline({
        .flags = rhi::RasterPipelineFlags::None,
        .vertexShaderBytecode = test_shaders::test_shaders_vs,
        .pixelShaderBytecode = test_shaders::test_shaders_ps,
        .rasterizerState = rhi::RasterizerState::SolidNoCull,
        .blendState = rhi::BlendState::Disabled,
        .depthState = rhi::DepthState::Disabled,
        .renderTargetFormats = {
            rhi::RenderTargetFormat::RGBA8Unorm
        },
        .depthFormat = rhi::DepthFormat::None,
        .topology = rhi::TopologyType::TriangleList
    });

    EXPECT_TRUE(IsValid(rasterPipeline));
    device->Destroy(rasterPipeline);
}

TEST_F(DeviceD3D12Tests, CanCreateAndDestroyComputePipelines)
{
    rhi::ComputePipeline computePipeline = device->CreateComputePipeline({
        .computeShaderBytecode = test_shaders::test_shaders_cs
    });

    EXPECT_TRUE(IsValid(computePipeline));
    device->Destroy(computePipeline);
}

TEST_F(DeviceD3D12Tests, CanCreateAndDestroyBuffers)
{
    // Some common use cases
    const rhi::BufferCreationFlags flags[] =
    {
        rhi::BufferCreationFlags::AllowUniformBuffer,
        rhi::BufferCreationFlags::AllowShaderReadOnly,
        rhi::BufferCreationFlags::AllowShaderReadWrite,

        rhi::BufferCreationFlags::AllowUniformBuffer | rhi::BufferCreationFlags::AllowShaderReadWrite,
        rhi::BufferCreationFlags::AllowShaderReadOnly | rhi::BufferCreationFlags::AllowShaderReadWrite,
    };

    for (rhi::BufferCreationFlags f : flags)
    {
        const uint32_t sizeInBytes = 1024;
        rhi::GPUAllocation a = device->GPUAlloc(::MemoryCategory::GPUTest, sizeInBytes, rhi::GPUAllocationFlags::DeviceAccessOptimal);

        const rhi::GPUMemoryRegion region = 
        {
            .allocation = a,
            .offsetInAllocation = 0,
            .regionSize = sizeInBytes
        };

        rhi::Buffer b = device->CreateBuffer({
                .flags = f,
                .name = "Test Buffer"
            },
            region);

        EXPECT_TRUE(IsValid(b));

        device->Destroy(b);
        device->GPUFree(a);
    }
}

TEST_F(DeviceD3D12Tests, CanCreateAndDestroyNonDepth2DTextures)
{
    const rhi::TextureCreationFlags flags[] =
    {
        rhi::TextureCreationFlags::AllowShaderReadOnly,
        rhi::TextureCreationFlags::AllowShaderReadWrite,
        rhi::TextureCreationFlags::AllowRenderTarget,
        
        rhi::TextureCreationFlags::AllowShaderReadOnly | rhi::TextureCreationFlags::AllowShaderReadWrite,
        rhi::TextureCreationFlags::AllowShaderReadOnly | rhi::TextureCreationFlags::AllowShaderReadWrite | rhi::TextureCreationFlags::AllowRenderTarget,

        rhi::TextureCreationFlags::AllowShaderReadOnly | rhi::TextureCreationFlags::AllowRenderTarget,
    };

    for (rhi::TextureCreationFlags f : flags)
    {
        rhi::Texture2DDesc desc =
        {
            .flags = f,
            .width = 128,
            .height = 128,
            .arraySize = 1,
            .mipLevels = 1,
            .format = rhi::TextureFormat::RGBA8Unorm,
            .optClearValue = nullptr,
            .name = "Test Texture"
        };

        rhi::ResourceFootprint fp = device->CalculateTextureFootprint(desc);
        rhi::GPUAllocation a = device->GPUAlloc(::MemoryCategory::GPUTest, fp.sizeInBytes, rhi::GPUAllocationFlags::DeviceAccessOptimal);

        const rhi::GPUMemoryRegion region = 
        {
            .allocation = a,
            .offsetInAllocation = 0,
            .regionSize = fp.sizeInBytes
        };

        rhi::Texture2D tex = device->CreateTexture2D(desc, region);
        EXPECT_TRUE(IsValid(tex));

        device->Destroy(tex);
        device->GPUFree(a);
    }
}

TEST_F(DeviceD3D12Tests, CanCreateAndDestroyDepth2DTextures)
{
    const rhi::TextureCreationFlags flags[] =
    {
        rhi::TextureCreationFlags::AllowDepthStencilTarget,
        rhi::TextureCreationFlags::AllowShaderReadOnly | rhi::TextureCreationFlags::AllowDepthStencilTarget
    };

    for (rhi::TextureCreationFlags f : flags)
    {
        rhi::Texture2DDesc desc =
        {
            .flags = f,
            .width = 128,
            .height = 128,
            .arraySize = 1,
            .mipLevels = 1,
            .format = rhi::TextureFormat::R32Float,
            .optClearValue = nullptr,
            .name = "Test Texture"
        };

        rhi::ResourceFootprint fp = device->CalculateTextureFootprint(desc);
        rhi::GPUAllocation a = device->GPUAlloc(::MemoryCategory::GPUTest, fp.sizeInBytes, rhi::GPUAllocationFlags::DeviceAccessOptimal);

        const rhi::GPUMemoryRegion region = 
        {
            .allocation = a,
            .offsetInAllocation = 0,
            .regionSize = fp.sizeInBytes
        };

        rhi::Texture2D tex = device->CreateTexture2D(desc, region);
        EXPECT_TRUE(IsValid(tex));

        device->Destroy(tex);
        device->GPUFree(a);
    }
}

TEST_F(DeviceD3D12Tests, CanCreateAndDestroy3DTextures)
{
    const rhi::TextureCreationFlags flags[] =
    {
        rhi::TextureCreationFlags::AllowShaderReadOnly,
        rhi::TextureCreationFlags::AllowShaderReadWrite,
        
        rhi::TextureCreationFlags::AllowShaderReadOnly | rhi::TextureCreationFlags::AllowShaderReadWrite,
    };

    for (rhi::TextureCreationFlags f : flags)
    {
        rhi::Texture3DDesc desc =
        {
            .flags = f,
            .width = 32,
            .height = 32,
            .depth = 32,
            .mipLevels = 1,
            .format = rhi::TextureFormat::RGBA8Unorm,
            .name = "Test Texture"
        };

        rhi::ResourceFootprint fp = device->CalculateTextureFootprint(desc);
        rhi::GPUAllocation a = device->GPUAlloc(::MemoryCategory::GPUTest, fp.sizeInBytes, rhi::GPUAllocationFlags::DeviceAccessOptimal);

        const rhi::GPUMemoryRegion region = 
        {
            .allocation = a,
            .offsetInAllocation = 0,
            .regionSize = fp.sizeInBytes
        };

        rhi::Texture3D tex = device->CreateTexture3D(desc, region);
        EXPECT_TRUE(IsValid(tex));

        device->Destroy(tex);
        device->GPUFree(a);
    }
}