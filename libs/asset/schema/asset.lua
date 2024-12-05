import "reference.lua"
namespace "rn.asset.schema"

Asset = struct {
    field(String, "identifier"),
    field(span(String), "references"),
    field(span(uint8), "assetData"),
}