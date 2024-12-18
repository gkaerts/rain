#ifndef _TEST_HLSLI_
#define _TEST_HLSLI_

#include "shared/unit_test/test.h"
#include "string.hlsli"

#if _D3D12_
    ConstantBuffer<TestContextConstants> _TestConstants : register(b0);

#elif _VULKAN_
    [[vk::push_constants]] TestContextConstants _TestConstants;

#endif

struct TestCase
{
    uint4   currentIdentifier;
    uint    currentResult;
    uint    failureLine;
};

uint4 LoadTestInputData() { return _TestConstants.testInputData; }

void WriteTestCaseResults(TestCase test)
{
    RWByteAddressBuffer resultCounts = ResourceDescriptorHeap[_TestConstants.rwbuffer_ResultCount];
    RWByteAddressBuffer results = ResourceDescriptorHeap[_TestConstants.rwbuffer_Results];

    uint resultIndex = 0;
    resultCounts.InterlockedAdd(0, 1, resultIndex);

    if (resultIndex < _TestConstants.resultBufferCapacity)
    {
        GPUTestResult result;
        result.name =           test.currentIdentifier;
        result.resultCode =     test.currentResult;
        result.failureLine =    test.failureLine;

        uint resultOffset = resultIndex * sizeof(GPUTestResult);
        results.Store<GPUTestResult>(resultOffset, result);
    }
    else
    {
        uint temp = 0;
        resultCounts.InterlockedExchange(0, resultIndex, temp);
    }
}

#define SHADER_TEST(localTestName, s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, s12, s13, s14, s15) \
    static const uint4 localTestName##_identifier = RN_SHADER_STRING_LONG(s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, s12, s13, s14, s15); \
    void localTestName(inout TestCase test)

#define RUN_SHADER_TEST(localTestName)                      \
{                                                           \
    test.currentIdentifier = localTestName##_identifier;    \
    localTestName(test);                                    \
    WriteTestCaseResults(test);                             \
}

#define BEGIN_SHADER_TEST_LIST                  \
[numthreads(1, 1, 1)]                           \
void test_main() {                              \
    TestCase test;                              \
    test.currentIdentifier = uint4(0, 0, 0, 0); \
    test.currentResult = 1;                     \
    test.failureLine = 0;

#define END_SHADER_TEST_LIST }

#define EXPECT_TRUE(condition)  { bool cond = (condition); test.currentResult = test.currentResult & (cond ? 1 : 0); if (!cond) test.failureLine = __LINE__; }
#define EXPECT_FALSE(condition) EXPECT_TRUE(!(condition))

#define EXPECT_EQ(val1, val2)   EXPECT_TRUE(all(val1 == val2))
#define EXPECT_NE(val1, val2)   EXPECT_TRUE(!all(val1 == val2))

#endif // _TEST_HLSLI_