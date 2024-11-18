#include <gtest/gtest.h>

#include "common/log/log.hpp"
#include "common/memory/memory.hpp"
#include "common/memory/vector.hpp"
#include "rhi/device.hpp"
#include "rhi/format.hpp"
#include "rhi/command_list.hpp"
#include "rhi/rhi_d3d12.hpp"

using namespace rn;

RN_MEMORY_CATEGORY(GPUTest)

struct CommandListD3D12Tests : public ::testing::Test
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

TEST_F(CommandListD3D12Tests, CanSubmitCommandList)
{
    rhi::CommandList* commandList = device->AllocateCommandList();
    device->SubmitCommandLists({&commandList, 1});
}

TEST_F(CommandListD3D12Tests, CanUploadBuffer)
{
    rhi::CommandList* commandList = device->AllocateCommandList();
    rhi::GPUAllocation alloc = device->GPUAlloc(::MemoryCategory::GPUTest, 1024, rhi::GPUAllocationFlags::DeviceAccessOptimal);

    rhi::BufferDesc desc = {
        .flags = rhi::BufferCreationFlags::AllowShaderReadOnly,
        .size = 1024,
        .name = "Test Buffer"
    };

    rhi::Buffer buffer = device->CreateBuffer(desc, { alloc, 0, 1024 });

    const unsigned char testData[1024] = {};
    commandList->UploadBufferData(buffer, 0, testData);

    device->SubmitCommandLists({&commandList, 1});

    device->Destroy(buffer);
    device->GPUFree(alloc);
}

TEST_F(CommandListD3D12Tests, CanUploadTexture2D)
{
    rhi::Texture2DDesc desc = {
        .flags = rhi::TextureCreationFlags::AllowShaderReadOnly,
        .width = 128,
        .height = 128,
        .arraySize = 1,
        .mipLevels = 1,
        .format = rhi::TextureFormat::RGBA8Unorm,
        .optClearValue = nullptr,
        .name = "Test Texture"
    };

    rhi::ResourceFootprint fp = device->CalculateTextureFootprint(desc);
    rhi::GPUAllocation alloc = device->GPUAlloc(::MemoryCategory::GPUTest, fp.sizeInBytes, rhi::GPUAllocationFlags::DeviceAccessOptimal);
    rhi::Texture2D texture = device->CreateTexture2D(desc, { alloc, 0, fp.sizeInBytes});

    rhi::MipUploadDesc uploadDesc[1] = {};
    uint64_t totalUploadSize = device->CalculateMipUploadDescs(desc, uploadDesc);
    
    Vector<unsigned char> textureData(totalUploadSize);

    rhi::BarrierDesc barriers = {
        .texture2DBarriers = {
            {
                .fromStage = rhi::PipelineSyncStage::None,
                .toStage = rhi::PipelineSyncStage::Copy,
                .fromAccess = rhi::PipelineAccess::None,
                .toAccess = rhi::PipelineAccess::CopyWrite,
                .fromLayout = rhi::TextureLayout::Undefined,
                .toLayout = rhi::TextureLayout::CopyWrite,
                .handle = texture,
                .firstMipLevel = 0,
                .numMips = 1,
                .firstArraySlice = 0,
                .numArraySlices = 1
            }
        }
    };

    rhi::CommandList* commandList = device->AllocateCommandList();
    commandList->Barrier(barriers);
    commandList->UploadTextureData(texture, 0, uploadDesc, textureData);

    device->SubmitCommandLists({&commandList, 1});

    device->Destroy(texture);
    device->GPUFree(alloc);
}

TEST_F(CommandListD3D12Tests, CanUploadTexture3D)
{
    rhi::Texture3DDesc desc = {
        .flags = rhi::TextureCreationFlags::AllowShaderReadOnly,
        .width = 128,
        .height = 128,
        .depth = 128,
        .mipLevels = 1,
        .format = rhi::TextureFormat::RGBA8Unorm,
        .optClearValue = nullptr,
        .name = "Test Texture"
    };

    rhi::ResourceFootprint fp = device->CalculateTextureFootprint(desc);
    rhi::GPUAllocation alloc = device->GPUAlloc(::MemoryCategory::GPUTest, fp.sizeInBytes, rhi::GPUAllocationFlags::DeviceAccessOptimal);
    rhi::Texture3D texture = device->CreateTexture3D(desc, { alloc, 0, fp.sizeInBytes});

    rhi::MipUploadDesc uploadDesc[1] = {};
    uint64_t totalUploadSize = device->CalculateMipUploadDescs(desc, uploadDesc);
    
    Vector<unsigned char> textureData(totalUploadSize);

    rhi::BarrierDesc barriers = {
        .texture3DBarriers = {
            {
                .fromStage = rhi::PipelineSyncStage::None,
                .toStage = rhi::PipelineSyncStage::Copy,
                .fromAccess = rhi::PipelineAccess::None,
                .toAccess = rhi::PipelineAccess::CopyWrite,
                .fromLayout = rhi::TextureLayout::Undefined,
                .toLayout = rhi::TextureLayout::CopyWrite,
                .handle = texture,
                .firstMipLevel = 0,
                .numMips = 1
            }
        }
    };

    rhi::CommandList* commandList = device->AllocateCommandList();
    commandList->Barrier(barriers);
    commandList->UploadTextureData(texture, 0, uploadDesc, textureData);

    device->SubmitCommandLists({&commandList, 1});

    device->Destroy(texture);
    device->GPUFree(alloc);
}