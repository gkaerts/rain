import "common.lua"
import "reference.lua"

namespace "rn.render.schema"

StaticMesh = struct {
    field(Reference, "geometry"),
    field(span(Reference), "materials")
}

StaticMeshInstance = struct {
    field(uint32, "meshIndex"),
    field(Float3, "position"),
    field(Quaternion, "rotation"),
}