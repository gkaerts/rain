#ifndef _FRUSTUM_H_
#define _FRUSTUM_H_

#include "shared/common.h"

struct Frustum
{
    float4 plane0;
    float4 plane1;
    float4 plane2;
    float4 plane3;
    float4 minMaxZ;
};

#endif // _FRUSTUM_H_