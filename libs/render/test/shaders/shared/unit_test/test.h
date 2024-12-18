#ifndef _TEST_H_
#define _TEST_H_

#include "shared/common.h"
#include "shared/globals.h"

struct GPUTestResult
{
    uint4   name;
    uint    resultCode;
    uint    failureLine;
};

struct TestContextConstants
{
    ShaderResource  rwbuffer_Results;
    ShaderResource  rwbuffer_ResultCount;

    uint            resultBufferCapacity;
    uint            pad0;

    uint4           testInputData;
};
static_assert(sizeof(TestContextConstants) <= MAX_GLOBAL_UNIFORMS_SIZE);


#endif // _TEST_H_