import "common.lua"

namespace "rn.render.schema"

PerspectiveCamera = struct {
    field(Float3, "position"),
    field(Float3, "lookVector"),
    field(Float3, "upVector"),

    field(float, "zNear"),
    field(float, "zFar"),
    field(float, "verticalFoV")
}