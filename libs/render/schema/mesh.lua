import "common.lua"
import "reference.lua"

namespace "rn.render.schema"

StaticMesh = struct {
    field(Reference, "geometry"),
    field(span(Reference), "materials")
}