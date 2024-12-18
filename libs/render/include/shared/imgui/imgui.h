#ifndef _IMGUI_H_
#define _IMGUI_H_

#include "shared/common.h"
#include "shared/globals.h"

struct ImGuiConstants
{
    ShaderResource uniform_PassData;
    uint offsetInDrawData;
};
static_assert(sizeof(ImGuiConstants) <= MAX_GLOBAL_UNIFORMS_SIZE);

struct ImGuiVertex
{
    float2 position;
    float2 uv;
    uint color;
};

struct ImGuiPassData
{
    row_major float4x4 projectionMatrix;
    ShaderResource buffer_Vertices;
    ShaderResource buffer_DrawData;
};

struct ImGuiDrawData
{
    ShaderResource tex2d_Texture0;
    uint baseVertex;
};


#endif // _IMGUI_H_