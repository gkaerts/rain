#include "shared/imgui/imgui.h"
#include "shared/samplers.h"
#include "globals.hlsli"

struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv       : TEXCOORD0;
    float4 color    : COLOR0;
};

PushConstants(ImGuiConstants, _ImGuiConstants)
static ConstantBuffer<ImGuiPassData> _PassData = ResourceDescriptorHeap[_ImGuiConstants.uniform_PassData];
static ByteAddressBuffer _DrawData = ResourceDescriptorHeap[_PassData.buffer_DrawData];
static ByteAddressBuffer _VertexBuffer = ResourceDescriptorHeap[_PassData.buffer_Vertices];

float4 UnpackColor(uint color)
{
    float w = (float)((color & 0xFF000000) >> 24) / 255.0;
    float z = (float)((color & 0x00FF0000) >> 16) / 255.0;
    float y = (float)((color & 0x0000FF00) >> 8) / 255.0;
    float x = (float)(color & 0x000000FF) / 255.0;

    return float4(x, y, z, w);
}

PSInput vs_main(uint vertexID : SV_VERTEXID)
{
    ImGuiDrawData drawData = _DrawData.Load<ImGuiDrawData>(_ImGuiConstants.offsetInDrawData);
    
    uint vertexIndex = drawData.baseVertex + vertexID;
    ImGuiVertex vertex = _VertexBuffer.Load<ImGuiVertex>(vertexIndex * sizeof(ImGuiVertex));

    PSInput output = (PSInput)0;
    output.position = mul(float4(vertex.position.xy, 0.0, 1.0), _PassData.projectionMatrix);
    output.uv = vertex.uv;
    output.color = UnpackColor(vertex.color);

    return output;
}

float4 ps_main(PSInput input) : SV_TARGET0
{
    ImGuiDrawData drawData = _DrawData.Load<ImGuiDrawData>(_ImGuiConstants.offsetInDrawData);
    Texture2D texture0 = ResourceDescriptorHeap[drawData.tex2d_Texture0];
    return input.color *  texture0.Sample(TrilinearWrap, input.uv);
}