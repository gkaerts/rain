import "common.lua"

namespace "rn.render.schema"

OmniLight = struct {
    field(Float3,   "position"),
    field(ColorRGB, "color"),
    field(float,    "radius"),
    field(float,    "sourceRadius"),
    field(float,    "intensity"),
    field(uint8,    "castShadow")
}

SpotLight = struct {
    field(Float3, "position"),
    field(Float4, "orientation"),
    field(Float3, "color"),
    field(float, "radius"),
    field(float, "sourceRadius"),
    field(float, "outerAngle"),
    field(float, "innerAngle"),
    field(float, "intensity"),
    field(uint8, "castShadow"),
}

RectLight = struct {
    field(Float3, "position"),
    field(Float4, "orientation"),
    field(Float3, "color"),
    field(float, "radius"),
    field(float, "intensity"),
    field(float, "width"),
    field(float, "height"),
    field(uint8, "castShadow"),
}