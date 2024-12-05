namespace "rn.data.schema"

Float2 = struct {
    field(float, "x"),
    field(float, "y")
}

Float3 = struct {
    field(float, "x"),
    field(float, "y"),
    field(float, "z")
}

Float4 = struct {
    field(float, "x"),
    field(float, "y"),
    field(float, "z"),
    field(float, "w")
}

Uint2 = struct {
    field(uint32, "x"),
    field(uint32, "y")
}

Uint3 = struct {
    field(uint32, "x"),
    field(uint32, "y"),
    field(uint32, "z")
}

Uint4 = struct {
    field(uint32, "x"),
    field(uint32, "y"),
    field(uint32, "z"),
    field(uint32, "w")
}

Int2 = struct {
    field(int32, "x"),
    field(int32, "y")
}

Int3 = struct {
    field(int32, "x"),
    field(int32, "y"),
    field(int32, "z")
}

Int4 = struct {
    field(int32, "x"),
    field(int32, "y"),
    field(int32, "z"),
    field(int32, "w")
}

ColorRGB = struct {
    field(float, "r"),
    field(float, "g"),
    field(float, "b")
}

Quaternion = struct {
    field(float, "x"),
    field(float, "y"),
    field(float, "z"),
    field(float, "w")
}

AABB = struct {
    field(Float3, "min"),
    field(Float3, "max")
}