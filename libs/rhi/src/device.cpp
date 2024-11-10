#include "rhi/device.hpp"

namespace rn::rhi
{
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

    
}