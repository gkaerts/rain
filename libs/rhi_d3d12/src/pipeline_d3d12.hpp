#pragma once

#include "common/common.hpp"
#include "d3d12_fwd.hpp"

namespace rn::rhi
{
    ID3D12RootSignature* CreateBindlessRootSignature(ID3D12Device10* device);
    ID3D12RootSignature* CreateRayTracingLocalRootSignature(ID3D12Device10* device);
}