namespace "rn.data.schema"

TextureDataFormat = enum {
    value("Basis")
}

TextureUsage = enum {
    value("Diffuse"),
    value("Normal"),
    value("Control"),
    value("UI"),
    value("LUT"),
    value("Heightmap"),
}

Texture = struct {
    field(TextureDataFormat, "dataFormat"),
    field(TextureUsage, "usage"),
    field(span(uint8), "data")
}