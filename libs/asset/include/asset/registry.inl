#pragma once
#include "asset/asset.hpp"
#include "common/memory/object_pool.hpp"
#include "common/type_id.hpp"

namespace rn::asset
{
    RN_DEFINE_HANDLE(BankedAsset, 0x40)
    enum class Residency : uint32_t
    {
        NotResident = 0,
        Resident
    };
    
    class BankBase
    {
    public:

        virtual std::pair<bool, BankedAsset> FindOrAllocateHandle(AssetIdentifier assetId) = 0;

        virtual BankedAsset         FindHandle(AssetIdentifier assetId) const = 0;
        virtual void                Store(BankedAsset handle, const AssetBuildDesc& desc) = 0;
        virtual Residency           AssetResidency(BankedAsset handle) const = 0;
    };

    template <typename DataType>
    class Bank : public BankBase
    {
    public:

        Bank(const AssetTypeDesc<DataType>& desc);
        ~Bank();

        std::pair<bool, BankedAsset> FindOrAllocateHandle(AssetIdentifier assetId) override;
        BankedAsset     FindHandle(AssetIdentifier assetId) const override;

        void            Store(BankedAsset handle, const AssetBuildDesc& desc) override;
        void            Store(BankedAsset handle, DataType&& data, AssetIdentifier assetId);

        Residency       AssetResidency(BankedAsset handle) const override;

        const DataType* Resolve(BankedAsset handle) const;
        DataType*       Resolve(BankedAsset handle);


    private:
        struct AssetState
        {
            Residency residency = Residency::NotResident;
        };

        using AssetPool = ObjectPool<BankedAsset, DataType, AssetState>;
        using IdentifierToHandle = HashMap<AssetIdentifier, BankedAsset>;

        Builder<DataType>*  _builder;
        AssetPool           _assets;

        IdentifierToHandle          _identifierToHandle = MakeHashMap<AssetIdentifier, BankedAsset>(MemoryCategory::Asset);
        mutable std::shared_mutex   _identifierToHandleMutex;
    };


    template <typename DataType>
    Bank<DataType>::Bank(const AssetTypeDesc<DataType>& desc)
        : _builder(desc.builder)
        , _assets(MemoryCategory::Asset, desc.initialCapacity)
    {}

    template <typename DataType>
    std::pair<bool, BankedAsset> Bank<DataType>::FindOrAllocateHandle(AssetIdentifier assetId)
    {
        BankedAsset handle = BankedAsset::Invalid;
        bool allocatedNewAsset = false;
        IdentifierToHandle::const_iterator it;
        {
            std::shared_lock lock(_identifierToHandleMutex);
            it = _identifierToHandle.find(assetId);
        }

        if (it == _identifierToHandle.end())
        {
            std::unique_lock lock(_identifierToHandleMutex);

            // Double check that no other thread got here first
            it = _identifierToHandle.find(assetId);
            if (it == _identifierToHandle.end())
            {
                AssetState newState = {
                    .residency = Residency::NotResident
                };
 
                handle = _assets.Store(std::move(DataType()), std::move(newState));
                _identifierToHandle[assetId] = handle;
                allocatedNewAsset = true;
            }
        }
        else
        {
            handle = it->second;
        }

        return std::make_pair(allocatedNewAsset, handle);
    }

    template <typename DataType>
    BankedAsset Bank<DataType>::FindHandle(AssetIdentifier assetId) const
    {
        BankedAsset handle = BankedAsset::Invalid;
        IdentifierToHandle::const_iterator it;
        {
            std::shared_lock lock(_identifierToHandleMutex);
            it = _identifierToHandle.find(assetId);
        }

        if (it != _identifierToHandle.end())
        {
            handle = it->second;
        }

        return handle;
    }

    template <typename DataType>
    void Bank<DataType>::Store(BankedAsset handle, const AssetBuildDesc& desc)
    {
        Store(handle, std::move(_builder->Build(desc)), MakeAssetIdentifier(desc.identifier));
    }

    template <typename DataType>
    void Bank<DataType>::Store(BankedAsset handle, DataType&& data, AssetIdentifier assetId)
    {
        DataType* storedData = _assets.GetHotPtrMutable(handle);
        AssetState* state = _assets.GetColdPtrMutable(handle);
        RN_ASSERT(storedData);

        if (state->residency != Residency::NotResident)
        {
            _builder->Destroy(*storedData);
            storedData->~DataType();
        }

        new (storedData) DataType(std::move(data));

        {
            std::shared_lock lock(_identifierToHandleMutex);
            _identifierToHandle[assetId] = handle;
            state->residency = Residency::Resident;
        }

    }

    template <typename DataType>
    Residency Bank<DataType>::AssetResidency(BankedAsset handle) const
    {
        const AssetState* state = _assets.GetColdPtr(handle);
        RN_ASSERT(state);
        
        return state->residency;
    }

    template <typename DataType>
    const DataType* Bank<DataType>::Resolve(BankedAsset handle) const
    {
        return _assets.GetHotPtr(handle);
    }

    template <typename DataType>
    DataType* Bank<DataType>::Resolve(BankedAsset handle)
    {
        return _assets.GetHotPtrMutable(handle);
    }


    template <typename DataType>
    void Registry::RegisterAssetType(const AssetTypeDesc<DataType>& desc)
    {
        // Make sure we don't have a hash or handle type collision
        RN_ASSERT(_typeIDToBank.find(TypeID<DataType>()) == _typeIDToBank.end());
        RN_ASSERT(_extensionHashToBank.find(desc.extensionHash) == _extensionHashToBank.end());

        BankBase* newBank = TrackedNew<Bank<DataType>>(MemoryCategory::Asset, desc);
        _typeIDToBank[TypeID<DataType>()] = newBank;
        _extensionHashToBank[desc.extensionHash] = newBank;
    }

    template <typename DataType>
    const DataType* Registry::Resolve(AssetIdentifier assetId) const
    {
        auto bankIt = _typeIDToBank.find(TypeID<DataType>());

        // Did you forget to register this asset type?
        RN_ASSERT(bankIt != _typeIDToBank.end());
        const Bank<DataType>* bank = static_cast<const Bank<DataType>*>(bankIt->second);

        BankedAsset handle = bank->FindHandle(assetId);
        return bank->Resolve(handle);
    }

    template <typename DataType>
    DataType* Registry::Resolve(AssetIdentifier assetId)
    {
        auto bankIt = _typeIDToBank.find(TypeID<DataType>());

        // Did you forget to register this asset type?
        RN_ASSERT(bankIt != _typeIDToBank.end());
        Bank<DataType>* bank = static_cast<Bank<DataType>*>(bankIt->second);

        BankedAsset handle = bank->FindHandle(assetId);
        return bank->Resolve(handle);
    }
}