#include "asset/asset.hpp"

namespace rn::asset
{
    RN_DEFINE_MEMORY_CATEGORY(Asset);

    void SanitizeAssetPath(Span<char> assetPath)
    {
        // Forward slashes only
        std::replace(assetPath.begin(), assetPath.end(), '\\', '/');

        // All lower case
        std::transform(assetPath.begin(), assetPath.end(), assetPath.begin(), [](char c)
        {
            return std::tolower(c);
        });
    }

    AssetIdentifier MakeAssetIdentifier(std::string_view assetPath)
    {
        MemoryScope SCOPE;
        ScopedString str(assetPath.data(), assetPath.length());
        SanitizeAssetPath(str);

        return AssetIdentifier(HashString(str));
    }
}