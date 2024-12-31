import "common.lua"
import "reference.lua"

namespace "rn.render.schema"

RenderMesh = struct {
    field(Reference, "geometry")
}

RenderMaterialList = struct {
    field(array(Reference, 16), "materials")
}