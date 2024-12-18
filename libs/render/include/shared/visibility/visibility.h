#ifndef _VISIBILITY_H_
#define _VISIBILITY_H_

#include "shared/common.h"
#include "shared/visibility/frustum.h"

static const uint VISIBILITY_PASS_GROUP_SIZE = 64;

struct InstanceAABB
{
    float3 minCorner;
    float3 maxCorner;
};

struct InstanceToDrawMapping
{
    uint firstDrawIndex;
    uint drawCount;
};

struct VisibilityPassData
{
    float4x4        worldToView;
    float4x4        worldToProjection;

    ShaderResource  tex2D_HiZTexture;
    ShaderResource  buffer_SourceInstanceIndices;
    ShaderResource  rwbuffer_DestCulledInstanceCount;
    ShaderResource  rwbuffer_DestCulledInstanceIndices;

    ShaderResource  rwbuffer_DestDrawArgs;
    ShaderResource  buffer_InstanceCount;
    ShaderResource  buffer_InstanceAABBS;
    ShaderResource  buffer_InstanceToDrawMappings;

    Frustum         frustum;

    uint2           viewportDimensions;
    uint            sourceInstanceCount;
    uint            pad0;
};

struct MergeDrawArgsData
{
    ShaderResource  buffer_SrcDrawsA;
    ShaderResource  buffer_SrcDrawsB;
    ShaderResource  rwbuffer_DestDraws;
    uint            drawCount;
};

#endif // _VISIBILITY_H_