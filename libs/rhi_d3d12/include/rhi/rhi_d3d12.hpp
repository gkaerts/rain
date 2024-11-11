#pragma once

#include "common/common.hpp"

namespace rn::rhi
{
    class Device;
    struct DeviceMemorySettings;

    struct DeviceD3D12Options
    {
        uint32_t adapterIndex;
        bool enableDebugLayer;
        bool enableGPUValidation;
    };

    Device* CreateD3D12Device(const DeviceD3D12Options& options, const DeviceMemorySettings& memorySettings);
}