#pragma once

#include "scene/scene.hpp"
#include <type_traits>

namespace rn::scene
{
    template <typename... Args>
    constexpr uint64_t MergeTypeIDs()
    {
        return (... ^ TypeID<Args>());
    }

    template <typename T>
    void ResolvePtrAtIndex(Vector<T>& vec, uint64_t typeID, size_t index, void*& outPtr)
    {
        if (TypeID<T>() == typeID)
        {
            outPtr = &vec[index];
        }
    }

    template <typename T>
    void ResolveConstPtrAtIndex(const Vector<T>& vec, uint64_t typeID, size_t index, const void*& outPtr)
    {
        if (TypeID<T>() == typeID)
        {
            outPtr = &vec[index];
        }
    }

    template <typename... Components>
    size_t EntityBuilder<Components...>::BuildEntity(Entity entity)
    {
        size_t index = std::numeric_limits<size_t>::max();
        auto it = _entityToIndex.find(entity);
        if (it == _entityToIndex.end())
        {
            index = std::get<0>(_storage).size();
            _entityToIndex[entity] = index;

            (std::get<Vector<Components>>(_storage).emplace_back(), ...);

        }

        return index;
    };

    template <typename... Components>
    size_t EntityBuilder<Components...>::EntityToIndex(Entity entity) const
    {
        return _entityToIndex.at(entity);
    }

    template <typename... Components>
    void* EntityBuilder<Components...>::Component(size_t entityIdx, uint64_t typeID)
    {
        void* ptr = nullptr;
        (ResolvePtrAtIndex(std::get<Vector<Components>>(_storage), typeID, entityIdx, ptr), ...);

        return ptr;
    }

    template <typename... Components>
    const void* EntityBuilder<Components...>::Component(size_t entityIdx, uint64_t typeID) const
    {
        const void* ptr = nullptr;
        (ResolveConstPtrAtIndex(std::get<Vector<Components>>(_storage), typeID, entityIdx, ptr), ...);

        return ptr;
    }

    template <typename... Components>
    void* EntityBuilder<Components...>::ComponentArray(uint64_t typeID)
    {
        return Component(0, typeID);
    }

    template <typename... Components>
    const void* EntityBuilder<Components...>::ComponentArray(uint64_t typeID) const
    {
        return Component(0, typeID);
    }

    template <typename... Components>
    size_t EntityBuilder<Components...>::EntityCount() const
    {
        return std::get<0>(_storage).size();
    }

    namespace
    {
        template <typename C>
        void SetComponent(EntityBuilderBase* builder, size_t entityIdx, C&& component)
        {
            void* ptr = builder->Component(entityIdx, TypeID<C>());
            std::memcpy(ptr, &component, sizeof(C));
        }
    }

    template <typename... Args>
    Entity Scene::AddEntity(Args&&... args)
    {
        constexpr uint64_t archetypeID = MergeTypeIDs<Args...>();
        auto it = _archetypes.find(archetypeID);
        if (it == _archetypes.end())
        {
            // I'm not really a fan of lazy initialization like this, but I guess it's ok
            it = MakeArchetype<Args...>();
        }
        RN_ASSERT(it != _archetypes.end());

        if (it != _archetypes.end())
        {
            Entity e = AssembleHandle<Entity>(_currentEntityID++, 0);
            size_t index = it->second.builder->BuildEntity(e);
            (SetComponent<Args>(it->second.builder, index, std::forward<Args>(args)), ...);

            _entityToArchetype.try_emplace(e, archetypeID);
            return e;
        }

        return Entity::Invalid;
    }

    template <typename... Cs>
    std::tuple<Cs const&...> Scene::Components(Entity e) const
    {
        uint64_t archetypeID = _entityToArchetype.at(e);
        const EntityBuilderBase* builder = _archetypes.at(archetypeID).builder;

        size_t entityIdx = builder->EntityToIndex(e);
        return { *static_cast<const Cs*>(builder->Component(entityIdx, TypeID<Cs>()))... };
    }

    template <typename... Cs>
    std::tuple<Cs&...> Scene::Components(Entity e)
    {
        uint64_t archetypeID = _entityToArchetype.at(e);
        EntityBuilderBase* builder = _archetypes.at(archetypeID).builder;

        size_t entityIdx = builder->EntityToIndex(e);
        return { *static_cast<Cs*>(builder->Component(entityIdx, TypeID<Cs>()))... };
    }

    template <typename... Cs>
    QueryResult<Cs...> Scene::Query()
    {
        constexpr uint64_t archetypeID = MergeTypeIDs<Cs...>();
        auto it = _archetypes.find(archetypeID);
        if (it == _archetypes.end())
        {
            it = MakeArchetype<Cs...>();
        }

        MemoryScope SCOPE;
        ScopedVector<EntityBuilderBase*> builders;
        builders.reserve(it->second.superArchetypes.size());

        for (uint64_t archetypeID : it->second.superArchetypes)
        {
            builders.push_back(_archetypes.at(archetypeID).builder);
        }

        size_t entityCount = 0;
        for (const EntityBuilderBase* builder : builders)
        {
            entityCount += builder->EntityCount();
        }

        QueryResult<Cs...> result = { 
            _tempAllocator.AllocatePODArray<std::tuple<Cs&...>>(entityCount), entityCount
        };

        size_t offset = 0;
        for (EntityBuilderBase* builder : builders)
        {
            size_t countInBuilder = builder->EntityCount();
            for (size_t i = 0; i < countInBuilder; ++i, ++offset)
            {
                new (&result[offset]) std::tuple<Cs&...>({*static_cast<Cs*>(builder->Component(i, TypeID<Cs>()))...});
            }
        }

        return result;
    }

    template <typename... Args>
    Scene::ArchetypeMap::iterator Scene::MakeArchetype()
    {
        constexpr uint64_t archetypeID = MergeTypeIDs<Args...>();
        auto it = _archetypes.find(archetypeID);
        RN_ASSERT(it == _archetypes.end());

        if (it == _archetypes.end())
        {
            using Builder = meta::InstantiateSortedT<EntityBuilder, std::remove_cvref_t<Args>...>;

            ArchetypeDesc desc = {
                .builder = TrackedNew<Builder>(MemoryCategory::Scene),
                .componentIDs = { TypeID<Args>()... },
                .superArchetypes = { archetypeID }
            };
            std::sort(desc.componentIDs.begin(), desc.componentIDs.end());

            for (auto& p : _archetypes)
            {
                MemoryScope SCOPE;
                ScopedVector<uint64_t> intersection;
                intersection.reserve(desc.componentIDs.size());

                std::set_intersection(desc.componentIDs.begin(), desc.componentIDs.end(),
                    p.second.componentIDs.begin(), p.second.componentIDs.end(),
                    std::back_inserter(intersection));
                    
                if (intersection.size() == desc.componentIDs.size())
                {
                    desc.superArchetypes.emplace_back(p.first);
                }
                else if (intersection.size() == p.second.componentIDs.size())
                {
                    p.second.superArchetypes.emplace_back(archetypeID);
                }
                
            }

            it = _archetypes.try_emplace(archetypeID, std::move(desc)).first;
        }

        return it;
    }
}