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

    enum class Asset : uint64_t
    {
        Invalid = 0xFFFFFFFF
    };

    struct AssetBuildDesc
    {
        const std::string_view identifier;
        Span<const uint8_t> data;
        Span<const Asset> dependencies;
        const Registry* registry;
    };

    template <typename HandleType, typename DataType>
    class Builder
    {
    public:

        virtual DataType    Build(const AssetBuildDesc& desc) = 0;
        virtual void        Destroy(DataType& data) = 0;
        virtual void        Finalize() = 0;
    };

    template <typename HandleType, typename DataType>
    struct AssetTypeDesc
    {
        StringHash identifierHash;
        size_t initialCapacity;
        Builder<HandleType, DataType>* builder;
    };
}