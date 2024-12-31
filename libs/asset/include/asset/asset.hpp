#pragma once

#include "common/common.hpp"
#include "common/handle.hpp"
#include "common/memory/hash_map.hpp"
#include "common/memory/span.hpp"
#include "common/memory/string.hpp"

#include <mutex>
#include <shared_mutex>

namespace rn::asset
{
    class Registry;

    RN_MEMORY_CATEGORY(Asset);

    enum class AssetIdentifier : uint64_t
    {
        Invalid = 0
    };
    inline bool IsValid(AssetIdentifier a) { return a != AssetIdentifier::Invalid; }

    void            SanitizeAssetPath(Span<char> assetPath);
    AssetIdentifier MakeAssetIdentifier(std::string_view assetPath);

    struct AssetBuildDesc
    {
        const std::string_view identifier;
        Span<const uint8_t> data;
        Span<const AssetIdentifier> dependencies;
        const Registry* registry;
    };

    template <typename DataType>
    class Builder
    {
    public:

        virtual DataType    Build(const AssetBuildDesc& desc) = 0;
        virtual void        Destroy(DataType& data) = 0;
        virtual void        Finalize() = 0;
    };

    template <typename DataType>
    struct AssetTypeDesc
    {
        StringHash extensionHash;
        size_t initialCapacity;
        Builder<DataType>* builder;
    };
}