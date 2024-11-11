struct PixelInput
{
    float4 Position : SV_POSITION0;
};

struct PixelOutput
{
    float4 Color : SV_TARGET0;
};

struct Constants
{
    uint vertexBuffer;
    uint outTexture;
};

ConstantBuffer<Constants> _Constants : register(b0);

PixelInput vs_main(uint vertexID : SV_VertexID)
{
    Buffer<float3> vertices = ResourceDescriptorHeap[_Constants.vertexBuffer];

    PixelInput OUT = (PixelInput)0;
    OUT.Position = float4(vertices.Load(vertexID), 1.0);

    return OUT;
}

PixelOutput ps_main(PixelInput)
{
    PixelOutput OUT = (PixelOutput)0;
    OUT.Color = float4(1.0, 0.0, 0.0, 1.0);

    return OUT;
}

[numthreads(8,8,1)]
void cs_main(uint2 dtid : SV_DispatchThreadID)
{
    RWTexture2D<float4> outTexture = ResourceDescriptorHeap[_Constants.outTexture];
    outTexture[dtid] = float4(1.0, 0.0, 0.0, 1.0);
}