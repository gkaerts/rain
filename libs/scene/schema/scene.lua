import "common.lua"

namespace "rn.scene.schema"

Component = struct {
    field(uint64, "typeID"),
    field(span(uint8),"data")
}

Entity = struct {
    field(String, "name"),
    field(uint64, "archetypeID"),
    field(span(Component), "components")
}

Scene = struct {
    field(span(Entity), "entities"),
}