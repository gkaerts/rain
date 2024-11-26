#pragma once
#include "asset/asset.hpp"
#include "common/memory/object_pool.hpp"

namespace rn::asset
{
    enum class Residency : uint32_t
    {
        NotResident = 0,
        Resident
    };
    
    class BankBase
    {
    public:

        virtual std::pair<bool, Asset>  FindOrAllocateHandle(StringHash identifier) = 0;
        virtual void                    Store(Asset handle, const AssetBuildDesc& desc) = 0;
        virtual Residency               AssetResidency(Asset handle) = 0;
    };

    template <typename HandleType, typename DataType>
    class Bank : public BankBase
    {
    public:

        Bank(const AssetTypeDesc<HandleType, DataType>& desc);
        ~Bank();

        std::pair<bool, Asset>  FindOrAllocateHandle(StringHash identifier) override;
        void                    Store(Asset handle, const AssetBuildDesc& desc) override;
        void                    Store(Asset handle, DataType&& data, StringHash identifier);

        Residency               AssetResidency(Asset handle) override;

        const DataType*         Resolve(HandleType handle) const;
        DataType*               Resolve(HandleType handle);


    private:
        struct AssetState
        {
            Residency residency = Residency::NotResident;
        };

        Builder<HandleType, DataType>* _builder;
        ObjectPool<HandleType, DataType, AssetState> _assets;

        std::shared_mutex _identifierToHandleMutex;
        HashMap<StringHash, HandleType> _identifierToHandle = MakeHashMap<StringHash, HandleType>(MemoryCategory::Asset);
    };


    template <typename HandleType, typename DataType>
    Bank<HandleType, DataType>::Bank(const AssetTypeDesc<HandleType, DataType>& desc)
        : _builder(desc.builder)
        , _assets(MemoryCategory::Asset, desc.initialCapacity)
    {}

    template <typename HandleType, typename DataType>
    std::pair<bool, Asset> Bank<HandleType, DataType>::FindOrAllocateHandle(StringHash identifier)
    {
        HandleType handle = HandleType::Invalid;
        bool allocatedNewAsset = false;
        typename HashMap<StringHash, HandleType>::iterator it;
        {
            std::shared_lock lock(_identifierToHandleMutex);
            it = _identifierToHandle.find(identifier);
        }

        if (it == _identifierToHandle.end())
        {
            std::unique_lock lock(_identifierToHandleMutex);

            // Double check that no other thread got here first
            it = _identifierToHandle.find(identifier);
            if (it == _identifierToHandle.end())
            {
                AssetState newState = {
                    .residency = Residency::NotResident
                };
 
                handle = _assets.Store(std::move(DataType()), std::move(newState));
                _identifierToHandle[identifier] = handle;
                allocatedNewAsset = true;
            }
        }
        else
        {
            handle = it->second;
        }

        return std::make_pair(allocatedNewAsset, Asset(handle));
    }

    template <typename HandleType, typename DataType>
    void Bank<HandleType, DataType>::Store(Asset handle, const AssetBuildDesc& desc)
    {
        Store(handle, std::move(_builder->Build(desc)), HashString(desc.identifier));
    }

    template <typename HandleType, typename DataType>
    void Bank<HandleType, DataType>::Store(Asset handle, DataType&& data, StringHash identifier)
    {
        HandleType typedHandle = HandleType(handle);
        DataType* storedData = _assets.GetHotPtrMutable(typedHandle);
        RN_ASSERT(storedData);

        _builder->Destroy(*storedData);
        *storedData = std::move(data);

        {
            std::shared_lock lock(_identifierToHandleMutex);
            _identifierToHandle[identifier] = typedHandle;
        }
    }

    template <typename HandleType, typename DataType>
    Residency Bank<HandleType, DataType>::AssetResidency(Asset handle)
    {
        const AssetState* state = _assets.GetColdPtr(HandleType(handle));
        RN_ASSERT(state);
        
        return state->residency;
    }

    template <typename HandleType, typename DataType>
    const DataType* Bank<HandleType, DataType>::Resolve(HandleType handle) const
    {
        return _assets.GetHotPtr(handle);
    }

    template <typename HandleType, typename DataType>
    DataType* Bank<HandleType, DataType>::Resolve(HandleType handle)
    {
        return _assets.GetHotPtrMutable(handle);
    }


    template <typename HandleType, typename DataType>
    void Registry::RegisterAssetType(const AssetTypeDesc<HandleType, DataType>& desc)
    {
        // Make sure we don't have a hash or handle type collision
        RN_ASSERT(_extensionHashToBank.find(desc.identifierHash) == _extensionHashToBank.end());
        RN_ASSERT(_handleIDToBank.find(size_t(HandleType::Salt)) == _handleIDToBank.end());

        BankBase* newBank = TrackedNew<Bank<HandleType, DataType>>(MemoryCategory::Asset, desc);
        _extensionHashToBank[desc.identifierHash] = newBank;
        _handleIDToBank[size_t(HandleType::Salt)] = newBank;
    }

    template <typename HandleType>
    HandleType Registry::Load(const char* identifier, LoadFlags flags)
    {
        return HandleType(LoadInternal(identifier, flags));
    }

    template <typename HandleType, typename DataType>
    const DataType* Registry::Resolve(HandleType handle) const
    {
        auto bankIt = _handleIDToBank.find(size_t(HandleType::Salt));

        // Did you forget to register this asset type?
        RN_ASSERT(bankIt != _handleIDToBank.end());
        const Bank<HandleType, DataType>* bank = static_cast<const Bank<HandleType, DataType>*>(bankIt->second);
        return bank->Resolve(handle);
    }
}