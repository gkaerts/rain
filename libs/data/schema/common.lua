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

Matrix4 = struct {
    field(float, "m00"),
    field(float, "m01"),
    field(float, "m02"),
    field(float, "m03"),

    field(float, "m10"),
    field(float, "m11"),
    field(float, "m12"),
    field(float, "m13"),

    field(float, "m20"),
    field(float, "m21"),
    field(float, "m22"),
    field(float, "m23"),

    field(float, "m30"),
    field(float, "m31"),
    field(float, "m32"),
    field(float, "m33")
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