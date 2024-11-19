#pragma once

#include "common/common.hpp"
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
        virtual const void* Ptr() const = 0;
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

        template <typename HandleType, typename DataType>
        void RegisterAssetType(const AssetTypeDesc<HandleType, DataType>& desc);

        template <typename HandleType>
        HandleType Load(const char* identifier, LoadFlags flags = LoadFlags::None);

        template <typename HandleType, typename DataType>
        const DataType* Resolve(HandleType handle) const;

    private:

        Asset LoadInternal(const char* identifier, LoadFlags flags);

        using BankMap = HashMap<size_t, BankBase*>;

        String _contentPrefix;
        bool _enableMultithreadedLoad;
        FnMapAsset _onMapAsset;
        BankMap _extensionHashToBank = MakeHashMap<size_t, BankBase*>(MemoryCategory::Asset);
        BankMap _handleIDToBank = MakeHashMap<size_t, BankBase*>(MemoryCategory::Asset);
    };
}

#include "asset/registry.inl"