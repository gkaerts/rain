#include "gpu_test.hpp"
#include "common/common.hpp"
#include "common/memory/memory.hpp"


#include "rhi/command_list.hpp"

#include "rg/render_graph.hpp"
#include "rg/resource.hpp"
#include "rg/render_pass_context.hpp"

#include "shared/unit_test/test.h"

#include <gtest/gtest.h>
#include <iostream>

namespace rn
{
    RN_DEFINE_MEMORY_CATEGORY(GPUTest)

    void RunGPUUnitTest(rhi::Device* device, const rhi::ShaderBytecode& shader, rn::uint4 inputData)
    {
        rg::RenderGraph graph(device);
        graph.Reset({
                .x = 0,
                .y = 0,
                .width = 64,
                .height = 64
            });

        struct TestResources
        {
            rhi::ComputePipeline pipeline;
            rg::Buffer resultBuffer;
            rg::Buffer countBuffer;
            rn::uint4 inputData;
        };
        
        const uint32_t MAX_NUM_TESTS_PER_SHADER = 1024;
        TestResources testResources = {
            .pipeline = device->CreateComputePipeline({
                .computeShaderBytecode = shader
            }),

            .resultBuffer = graph.AllocateBuffer({
                .sizeInBytes = sizeof(GPUTestResult) * MAX_NUM_TESTS_PER_SHADER,
                .flags = rg::ResourceFlags::None,
                .name = "Test Result Buffer",
            }),

            .countBuffer = graph.AllocateBuffer({
                .sizeInBytes = sizeof(uint),
                .flags = rg::ResourceFlags::None,
                .name = "Test result count buffer",
            }),
            
            .inputData = inputData
        };
        ASSERT_NE(testResources.pipeline, rhi::ComputePipeline::Invalid);

        graph.AddRenderPass<TestResources>({
            .name = "Clear test result count",
            .flags = rg::RenderPassFlags::IsSmall | rg::RenderPassFlags::ComputeOnly,
            .buffers = {
                rg::CopyTo(testResources.countBuffer)
            },

            .onExecute = [](rhi::Device* device, rg::RenderPassContext* ctxt, rhi::CommandList* cl, const TestResources* passData, uint32_t)
            {
                rhi::TemporaryResource clearResource = cl->AllocateTemporaryResource(sizeof(uint));
                memset(clearResource.cpuPtr, 0, sizeof(uint));

                rg::BufferRegion dest = ctxt->Resolve(passData->countBuffer);
                cl->CopyBufferRegion(dest.buffer, dest.offset, clearResource.buffer, clearResource.offsetInBytes, sizeof(uint));
            }
        }, &testResources);

        graph.AddRenderPass<TestResources>({
            .name = "Run tests",
            .flags = rg::RenderPassFlags::IsSmall | rg::RenderPassFlags::ComputeOnly,
            .buffers = {
                rg::ShaderReadWrite(testResources.countBuffer),
                rg::ShaderReadWrite(testResources.resultBuffer)
            },

            .onExecute = [](rhi::Device* device, rg::RenderPassContext* ctxt, rhi::CommandList* cl, const TestResources* passData, uint32_t)
            {
                TestContextConstants testConstants = {
                    .rwbuffer_Results = IndexFromHandle(ctxt->ResolveRWView(passData->resultBuffer)),
                    .rwbuffer_ResultCount = IndexFromHandle(ctxt->ResolveRWView(passData->countBuffer)),
                    .resultBufferCapacity = MAX_NUM_TESTS_PER_SHADER,
                    .testInputData = passData->inputData
                };

                cl->PushComputeConstantData(0, &testConstants, sizeof(testConstants));
                cl->Dispatch({{ 
                      .pipeline = passData->pipeline,
                      .x = 1,
                      .y = 1,
                      .z = 1
                    }});
            }
        }, &testResources);

        graph.AddRenderPass<TestResources>({
            .name = "Parse test results",
            .flags = rg::RenderPassFlags::IsSmall | rg::RenderPassFlags::ComputeOnly,
            .buffers = {
                rg::CopyFrom(testResources.countBuffer),
                rg::CopyFrom(testResources.resultBuffer)
            },

            .onExecute = [](rhi::Device* device, rg::RenderPassContext* ctxt, rhi::CommandList* cl, const TestResources* passData, uint32_t)
            {
                uint* destCounter = TrackedNew<uint>(MemoryCategory::GPUTest);
                rg::BufferRegion countBuffer = ctxt->Resolve(passData->countBuffer);
                cl->QueueBufferReadback(countBuffer.buffer, countBuffer.offset, sizeof(uint), 
                    [](Span<const unsigned char> data, void* userData)
                    {
                        memcpy(userData, data.data(), sizeof(uint));
                    }, destCounter);

                rg::BufferRegion resultBuffer = ctxt->Resolve(passData->resultBuffer);
                cl->QueueBufferReadback(resultBuffer.buffer, resultBuffer.offset, sizeof(GPUTestResult) * MAX_NUM_TESTS_PER_SHADER,
                    [](Span<const unsigned char> data, void* userData)
                    {
                        uint* counterPtr = static_cast<uint*>(userData);
                        uint counter = *counterPtr;

                        const GPUTestResult* results = reinterpret_cast<const GPUTestResult*>(data.data());
                        for (uint i = 0; i < counter; ++i)
                        {
                            GPUTestResult result = results[i];
                            char testName[16] = {};
                            memcpy(testName, &result.name, 15 * sizeof(char));

                            std::cout << "[      GPU ] Checking GPU-originated test: \"" << testName << "\"" <<  std::endl;
                            EXPECT_NE(result.resultCode, 0) << "[      GPU ] Failure in shader on line: " << result.failureLine;
                        }

                        TrackedDelete(counterPtr);
                    }, destCounter);
            }
        }, &testResources);

        graph.Build();
        graph.Execute(rg::RenderGraphExecutionFlags::ForceSingleThreaded);

        device->EndFrame();
        device->DrainGPU();
    }
}