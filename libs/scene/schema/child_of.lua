import "common.lua"

namespace "rn.scene"
Entity = fwd_enum_64()

namespace "rn.scene.schema"

ChildOf = struct {
    field(Entity, "parent")
}