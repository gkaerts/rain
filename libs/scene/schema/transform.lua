import "common.lua"

namespace "rn.scene.schame"

Transform = struct {
    field(Float3, "position"),
    field(Quaternion, "rotation")
}