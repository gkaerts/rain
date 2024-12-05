import "common.lua"

namespace "rn.rhi"

IndexFormat = fwd_enum()

namespace "rn.data"

VertexStreamFormat = fwd_enum()

namespace "rn.data.schema"

BufferRegion = struct {
    field(uint32, "offsetInBytes"),
    field(uint32, "sizeInBytes")
}

VertexStreamType = enum {
    value("Position"),
    value("Normal"),
    value("Tangent"),
    value("UV"),
    value("Color"),
}

VertexStream = struct {
    field(BufferRegion, "region"),
    field(VertexStreamType, "type"),
    field(VertexStreamFormat, "format"),
    field(uint32, "componentCount"),
    field(uint32, "stride")
}

Meshlet = struct {
    field(uint32, "vertexOffset"),
    field(uint32, "triangleOffset"),
    field(uint32, "vertexCount"),
    field(uint32, "triangleCount")
}

GeometryPart = struct {
    field(uint32, "materialIdx"),
    field(uint32, "baseVertex"),
    field(BufferRegion, "indices"),
    field(IndexFormat, "indexFormat"),
    field(span(Meshlet), "meshlets"),
    field(BufferRegion, "meshletVertices"),
    field(BufferRegion, "meshletIndices")
}

Geometry = struct {
    field(AABB, "aabb"),
    field(span(GeometryPart), "parts"),
    field(VertexStream, "positions"),
    field(span(VertexStream), "vertexStreams"),
    field(span(uint8), "data")
}