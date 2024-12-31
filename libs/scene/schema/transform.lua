import "common.lua"

namespace "rn.scene.schema"

Transform = struct {
    field(Float3, "localPosition"),
    field(Quaternion, "localRotation"),
    field(Matrix4, "globalTransform")
}