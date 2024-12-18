#pragma once

#include "common/common.hpp"
#include "common/math/math.hpp"
#include "rhi/device.hpp"
#include "rhi/pipeline.hpp"


namespace rn
{
    void RunGPUUnitTest(rhi::Device* device, const rhi::ShaderBytecode& shader, uint4 inputData);
}