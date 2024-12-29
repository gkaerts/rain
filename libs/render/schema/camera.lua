import "common.lua"

namespace "rn.render.schema"

PerspectiveCamera = struct {
    field(float, "zNear"),
    field(float, "zFar"),
    field(float, "verticalFoV")
}