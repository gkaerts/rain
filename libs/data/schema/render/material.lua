import "reference.lua"

namespace "rn.data.schema"

TextureMaterialParameter = struct {
    field(String, "name"),
    field(Reference, "value")
}

FloatVecMaterialParameter = struct {
    field(String, "name"),
    field(span(float), "value")
}

UintVecMaterialParameter = struct {
    field(String, "name"),
    field(span(uint32), "value")
}

IntVecMaterialParameter = struct {
    field(String, "name"),
    field(span(int32), "value")
}

Material = struct {
    field(Reference, "materialShader"),
    field(span(TextureMaterialParameter), "textureParams"),
    field(span(FloatVecMaterialParameter), "floatVecParams"),
    field(span(UintVecMaterialParameter), "uintVecParams"),
    field(span(IntVecMaterialParameter), "intVecParams"),
}