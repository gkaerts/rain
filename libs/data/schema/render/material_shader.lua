import "common.lua"
import "reference.lua"

namespace "rn.data"
MaterialRenderPass = fwd_enum()
TextureType = fwd_enum()

namespace "rn.data.schema"


Bytecode = struct {
    field(span(uint8), "d3d12"),
    field(span(uint8), "vulkan"),
}

VertexRasterPass = struct {
    field(MaterialRenderPass, "renderPass"),
    field(Bytecode, "vertexShader"),
    field(Bytecode, "pixelShader")
}

MeshRasterPass = struct {
    field(MaterialRenderPass, "renderPass"),
    field(Bytecode, "meshShader"),
    field(Bytecode, "ampShader"),
    field(Bytecode, "pixelShader")
}

RayTracingPass = struct {
    field(MaterialRenderPass, "renderPass"),
    field(String, "hitGroupName"),
    field(String, "closestHitExport"),
    field(String, "anyHitExport")
}

TextureParameter = struct {
    field(String, "name"),
    field(uint32, "offsetInBuffer"),
    field(TextureType, "type"),
    field(Reference, "defaultValue")
}

FloatVecParameter = struct {
    field(String, "name"),
    field(uint32, "offsetInBuffer"),
    field(uint32, "dimension"),
    field(span(float), "defaultValue"),
    field(span(float), "minValue"),
    field(span(float), "maxValue")
}

UintVecParameter = struct {
    field(String, "name"),
    field(uint32, "offsetInBuffer"),
    field(uint32, "dimension"),
    field(span(uint32), "defaultValue"),
    field(span(uint32), "minValue"),
    field(span(uint32), "maxValue")
}

IntVecParameter = struct {
    field(String, "name"),
    field(uint32, "offsetInBuffer"),
    field(uint32, "dimension"),
    field(span(int32), "defaultValue"),
    field(span(int32), "minValue"),
    field(span(int32), "maxValue")
}

ParameterGroup = struct {
    field(String, "name"),
    field(span(TextureParameter), "textureParameters"),
    field(span(FloatVecParameter), "floatVecParameters"),
    field(span(UintVecParameter), "uintVecParameters"),
    field(span(IntVecParameter), "intVecParameters"),
}

MaterialShader = struct {
    field(span(VertexRasterPass), "vertexRasterPasses"),
    field(span(MeshRasterPass), "meshRasterPasses"),
    field(span(RayTracingPass), "rayTracingPasses"),
    field(span(ParameterGroup), "parameterGroups"),
    field(uint32, "uniformDataSize"),
    field(Bytecode, "rayTracingLibrary"),
}