#include "rhi/device.hpp"
#include "common/memory/memory.hpp"

namespace rn::rhi
{
    RN_DEFINE_MEMORY_CATEGORY(RHI)
    DeviceMemorySettings DefaultDeviceMemorySettings()
    {
        return {
            .rasterPipelineCapacity = 1024,
            .computePipelineCapacity = 1024,
            .raytracingPipelineCapacity = 64,
            .gpuAllocationCapacity = 8192,
            .bufferCapacity = 8192,
            .texture2dCapacity = 8192,
            .texture3dCapacity = 512,
            .blasCapacity = 4096,
            .tlasCapacity = 512,
        };
    }

    void DestroyDevice(Device* device)
    {
        TrackedDelete(device);
    }

}