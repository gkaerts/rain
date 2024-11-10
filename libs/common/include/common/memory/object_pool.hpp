#pragma once

#include "common/common.hpp"
#include "common/memory/memory.hpp"
#include "common/memory/vector.hpp"
#include <shared_mutex>
#include <type_traits>

namespace rn
{
    enum class MemoryCategoryID : uint8_t;

    template <typename HandleType, typename HotType, typename ColdType = void>
    class ObjectPool
    {
    private:

        static const uint32_t INVALID_INDEX = 0xFFFFFFFF;

    public:

        ObjectPool(MemoryCategoryID cat, size_t initialCapacity)
            : _cat(cat)
            , _capacity(initialCapacity)
            , _freeIndexCount(initialCapacity)
        {
            _hotStorage = static_cast<HotType*>(TrackedAlloc(cat, sizeof(HotType) * initialCapacity, 16));

            if constexpr (!std::is_void_v<ColdType>)
            {
                _coldStorage = static_cast<ColdType*>(TrackedAlloc(cat, sizeof(ColdType) * initialCapacity, 16));
            }

            _generationList = static_cast<uint8_t*>(TrackedAlloc(cat, sizeof(uint8_t) * initialCapacity, 16));
            _freeIndexList = static_cast<uint32_t*>(TrackedAlloc(cat, sizeof(uint32_t) * initialCapacity, 16));

            for (size_t i = 0; i < initialCapacity; ++i)
            {
                _freeIndexList[i] = uint32_t((initialCapacity - 1) - i);
            }

            std::memset(_generationList, 0, sizeof(uint8_t) * initialCapacity);
        }

        ~ObjectPool()
        {
            TrackedFree(_freeIndexList);
            TrackedFree(_generationList);
            TrackedFree(_hotStorage);

            if constexpr (!std::is_void_v<ColdType>)
            {
                TrackedFree(_coldStorage);
            }
        }

        ObjectPool(ObjectPool&& rhs)
        {
            std::swap(_hotStorage, rhs._hotStorage);
            std::swap(_coldStorage, rhs._coldStorage);
            std::swap(_generationList, rhs._generationList);
            std::swap(_freeIndexList, rhs._freeIndexList);
            std::swap(_freeIndexCount, rhs._freeIndexCount);
            std::swap(_capacity, rhs._capacity);
        }

        ObjectPool& operator=(ObjectPool&& rhs)
        {
            if (this != &rhs)
            {
                std::swap(_hotStorage, rhs._hotStorage);
                std::swap(_coldStorage, rhs._coldStorage);
                std::swap(_generationList, rhs._generationList);
                std::swap(_freeIndexList, rhs._freeIndexList);
                std::swap(_freeIndexCount, rhs._freeIndexCount);
                std::swap(_capacity, rhs._capacity);
            }
        }

        ObjectPool(const ObjectPool&) = delete;
        ObjectPool& operator=(const ObjectPool&) = delete;

        template <typename C = ColdType>
        requires std::is_void_v<C> HandleType Store(HotType&& args)
        {
            uint32_t index = -1;
            uint8_t generation = -1;
            {
                std::unique_lock lock(_mutex);

                if (_freeIndexCount == 0)
                {
                    GrowStorage();
                }
        
                index = _freeIndexList[_freeIndexCount - 1];
                generation = _generationList[index];

                _freeIndexList[_freeIndexCount - 1] = INVALID_INDEX;
                --_freeIndexCount;
            }

            new (&_hotStorage[index]) HotType(std::move(args));

            return AssembleHandle<HandleType>(index, generation);
        }

        template <typename C = ColdType>
        requires (!std::is_void_v<C>) HandleType Store(HotType&& hot, C&& cold)
        {
            uint32_t index = -1;
            uint8_t generation = -1;
            {
                std::unique_lock lock(_mutex);

                if (_freeIndexCount == 0)
                {
                    GrowStorage();
                }
        
                index = _freeIndexList[_freeIndexCount - 1];
                generation = _generationList[index];

                _freeIndexList[_freeIndexCount - 1] = INVALID_INDEX;
                --_freeIndexCount;
            }

            new (&_hotStorage[index]) HotType(std::move(hot));
            new (&_coldStorage[index]) ColdType(std::move(cold));

            return AssembleHandle<HandleType>(index, generation);
        }

        void Remove(HandleType handle)
        {
            std::unique_lock lock(_mutex);
            RN_ASSERT(_freeIndexCount != _capacity);

            uint64_t index = IndexFromHandle(handle);
            if (!IsValid(handle) || index >= _capacity)
            {
                return;
            }

            uint8_t generation = _generationList[index];
            if (generation != GenerationFromHandle(handle))
            {
                return;
            }

            _hotStorage[index].~HotType();

            if constexpr(!std::is_void_v<ColdType>)
            {
                _coldStorage[index].~ColdType();
            }

            ++_generationList[index];

            _freeIndexList[_freeIndexCount++] = uint32_t(index);
        }

        HotType* GetHotPtrMutable(HandleType handle) const
        {
            if (!IsValid(handle))
            {
                return nullptr;
            }

            uint64_t index = IndexFromHandle(handle);

            std::shared_lock lock(_mutex);
            if (index >= _capacity)
            {
                return nullptr;
            }

            uint8_t generation = _generationList[index];
            if (generation != GenerationFromHandle(handle))
            {
                return nullptr;
            }

            return &_hotStorage[index];
        }

        const HotType* GetHotPtr(HandleType handle) const
        {
            return GetHotPtrMutable(handle);
        }


        const HotType& GetHot(HandleType handle) const
        {
            return *GetHotPtr(handle);
        }

        HotType& GetHotMutable(HandleType handle) const
        {
            return *GetHotPtrMutable(handle);
        }

        template <typename C = ColdType>
        requires (!std::is_void_v<C>) 
        C* GetColdPtrMutable(HandleType handle) const
        {
            if (!IsValid(handle))
            {
                return nullptr;
            }

            uint64_t index = IndexFromHandle(handle);

            std::shared_lock lock(_mutex);
            if (index >= _capacity)
            {
                return nullptr;
            }

            uint8_t generation = _generationList[index];
            if (generation != GenerationFromHandle(handle))
            {
                return nullptr;
            }

            return &_coldStorage[index];
        }

        template <typename C = ColdType>
        requires (!std::is_void_v<C>) 
        const C* GetColdPtr(HandleType handle) const
        {
            return GetColdPtrMutable(handle);
        }


        template <typename C = ColdType>
        requires (!std::is_void_v<C>) 
        const C& GetCold(HandleType handle) const
        {
            return *GetColdPtr(handle);
        }

        template <typename C = ColdType>
        requires (!std::is_void_v<C>) 
        C& GetColdMutable(HandleType handle) const
        {
            return *GetColdPtrMutable(handle);
        }

    private:

        void GrowStorage()
        {
            RN_ASSERT(_freeIndexCount == 0);

            size_t newCapacity = _capacity * 2;

            HotType* newStorage = static_cast<HotType*>(TrackedAlloc(_cat, sizeof(HotType) * newCapacity, 16));
            uint8_t* newGenerationList = static_cast<uint8_t*>(TrackedAlloc(_cat, sizeof(uint8_t) * newCapacity, 16));
            uint32_t* newFreeIndexList = static_cast<uint32_t*>(TrackedAlloc(_cat, sizeof(uint32_t) * newCapacity, 16));

            for (uint32_t i = 0; i < _capacity; ++i)
            {
                new (&newStorage[i]) HotType(std::move(_hotStorage[i]));
            }

            std::memcpy(newGenerationList, _generationList, sizeof(uint8_t) * _capacity);

            std::swap(_freeIndexList, newFreeIndexList);
            std::swap(_generationList, newGenerationList);
            std::swap(_hotStorage, newStorage);

            _freeIndexCount = newCapacity - _capacity;
            for (size_t i = 0; i < _freeIndexCount; ++i)
            {
                _freeIndexList[i] = uint32_t((newCapacity - 1) - i);
            }

            if constexpr (!std::is_void_v<ColdType>)
            {
                ColdType* newColdStorage = static_cast<ColdType*>(TrackedAlloc(_cat, sizeof(ColdType) * newCapacity, 16));
                for (uint32_t i = 0; i < _capacity; ++i)
                {
                    new (&newColdStorage[i]) ColdType(std::move(_coldStorage[i]));
                }

                std::swap(_coldStorage, newColdStorage);
                TrackedFree(newColdStorage);
            }

            _capacity = newCapacity;
            TrackedFree(newFreeIndexList);
            TrackedFree(newGenerationList);
            TrackedFree(newStorage);
        }

        MemoryCategoryID _cat;

        HotType* _hotStorage = nullptr;
        ColdType* _coldStorage = nullptr;

        uint8_t* _generationList = nullptr;
        uint32_t* _freeIndexList = nullptr;

        size_t _capacity = 0;
        size_t _freeIndexCount = 0;

        mutable std::shared_mutex _mutex;
    };
}