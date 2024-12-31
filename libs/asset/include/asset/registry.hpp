#pragma once

#include "common/common.hpp"
#include "common/memory/span.hpp"
#include "asset/asset.hpp"

namespace rn::asset
{
    class BankBase;

    enum class LoadFlags : uint32_t
    {
        None = 0x0,
        Reload = 0x01
    };
    RN_DEFINE_ENUM_CLASS_BITWISE_API(LoadFlags)

    class MappedAsset
    {
    public:
        virtual Span<const uint8_t> Ptr() const = 0;
    };

    using FnMapAsset = MappedAsset*(*)(MemoryScope& scope, const String& path);
    MappedAsset* MapFileAsset(MemoryScope& scope, const String& path);

    struct RegistryDesc
    {
        const char* contentPrefix;
        bool enableMultithreadedLoad;
        FnMapAsset onMapAsset = &MapFileAsset;
    };

    class Registry
    {
    public:

        Registry(const RegistryDesc& desc);
        ~Registry();

        template <typename DataType>
        void RegisterAssetType(const AssetTypeDesc<DataType>& desc);

        void Load(std::string_view identifier, LoadFlags flags = LoadFlags::None);

        template <typename DataType>
        const DataType* Resolve(AssetIdentifier assetId) const;

        template <typename DataType>
        DataType* Resolve(AssetIdentifier assetId);

    private:

        using BankMap = HashMap<size_t, BankBase*>;

        String      _contentPrefix;
        bool        _enableMultithreadedLoad;
        FnMapAsset  _onMapAsset;
        BankMap     _typeIDToBank = MakeHashMap<uint64_t, BankBase*>(MemoryCategory::Asset);
        BankMap     _extensionHashToBank = MakeHashMap<uint64_t, BankBase*>(MemoryCategory::Asset);
    };
}

#include "asset/registry.inl"