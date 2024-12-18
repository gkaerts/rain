import "common.lua"

namespace "rn.scene.schema"

EntityContainer = struct {
    field(uint64, "entityTypeID"),
    field(span(uint8), "data")
}

PropertyContainer = struct {
    field(uint64, "propertyTypeID"),
    field(span(uint8), "data")
}

Scene = struct {
    field(span(EntityContainer), "entities"),
    field(span(PropertyContainer), "properties")
}