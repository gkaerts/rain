namespace "rn.asset"

AssetIdentifier = fwd_enum_64()

namespace "rn.asset.schema"

Reference = struct {
    field(uint32, "identifier")
}