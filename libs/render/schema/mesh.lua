import "common.lua"
import "reference.lua"

namespace "rn.render.schema"

RenderMesh = struct {
    field(AssetIdentifier, "geometry")
}

RenderMaterialList = struct {
    field(array(AssetIdentifier, 16), "materials")
}