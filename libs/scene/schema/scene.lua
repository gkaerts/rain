import "common.lua"

namespace "rn.scene.schema"

Blob = struct {
    field(span(uint8),"data")
}

Component = struct {
    field(uint64, "typeID"),
    field(Blob, "data")
}

Entity = struct {
    field(String, "name"),
    field(span(Component), "components")
}

Scene = struct {
    field(span(Entity), "entities"),
    field(span(Blob), "properties"),
    field(span(Blob), "additionalData")
}