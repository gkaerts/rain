#include "material_shader_include.hlsli"
#include "globals.hlsli"

struct PixelInput
{
    float4 Position : SV_POSITION0;
    float2 UV       : TEXCOORD0;
};

struct PixelOutput
{
    float4 Color : SV_TARGET0;
};

struct Constants
{
    uint vertexBuffer;
    uint uvBuffer;
    uint indexBuffer;
    uint outTexture;
};

struct Material
{
    uint colorTexture;
    float4 colorBlend;
};

ExportMaterialType(Material)

SamplerState _Sampler : register(s0);

PixelInput vs_main(uint vertexID : SV_VertexID)
{
    ConstantBuffer<Constants> constants = ResourceDescriptorHeap[_Globals.uniform_FrameData];
    
    Buffer<float3> vertices = ResourceDescriptorHeap[constants.vertexBuffer];
    Buffer<float2> uvs = ResourceDescriptorHeap[constants.uvBuffer];

    PixelInput OUT = (PixelInput)0;
    OUT.Position = float4(vertices.Load(vertexID), 1.0);
    OUT.UV = uvs.Load(vertexID);

    return OUT;
}

PixelOutput ps_main(PixelInput IN)
{
    ConstantBuffer<Material> material = ResourceDescriptorHeap[_Globals.uniform_PassData];
    Texture2D<float4> colorTexture = ResourceDescriptorHeap[material.colorTexture];

    PixelOutput OUT = (PixelOutput)0;
    OUT.Color = material.colorBlend * colorTexture.Sample(_Sampler, IN.UV);

    return OUT;
}
