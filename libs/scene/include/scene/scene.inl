#pragma once

#include "scene/scene.hpp"
#include "common/memory/string.hpp"
#include <type_traits>

namespace rn::scene
{
    using FnRegisterArchetype = void(*)(Scene&);
    std::vector<FnRegisterArchetype>& ArchetypeRegistrationFunctions();

    template <typename... Cs>
    uint64_t RegisterEntityArchetype()
    {
        std::vector<FnRegisterArchetype>& archetypes = ArchetypeRegistrationFunctions();
        ArchetypeRegistrationFunctions().push_back([](Scene& scene)
        {
            scene.RegisterArchetype<Cs...>();
        });

        return (... ^ TypeID<Cs>());
    }

    template <typename... Cs>
    const uint64_t _EntityArchetype<Cs...>::ID = RegisterEntityArchetype<Cs...>();
    
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

    template <typename T>
    void ComponentStorage<T>::AddComponent()
    {
        _storage.emplace_back();
    }

    template <typename T>
    void ComponentStorage<T>::RemoveComponent(size_t idx)
    {
        _storage.erase(_storage.begin() + idx);
    }

    template <typename T>
    void* ComponentStorage<T>::Component(size_t idx)
    {
        return &_storage[idx];
    }

    template <typename T>
    const void* ComponentStorage<T>::Component(size_t idx) const
    {
        return &_storage[idx];
    }

    template <typename T>
    size_t ComponentStorage<T>::Count() const
    {
        return _storage.size();
    }


    namespace
    {
        template <typename C>
        void SetComponent(EntityBuilder* builder, size_t entityIdx, C&& component)
        {
            Span<uint8_t> ptr = builder->Component(entityIdx, TypeID<C>());
            std::memcpy(ptr.data(), &component, sizeof(C));
        }
    }

    template <typename... Cs>
    Entity Scene::AddEntity(std::string_view identifier, Cs&&... args)
    {
        uint64_t archetypeID = EntityArchetype<Cs...>::ID;
        auto it = _archetypes.find(archetypeID);
        RN_ASSERT(it != _archetypes.end());

        if (it != _archetypes.end())
        {
            Entity e = Entity(HashString(identifier));
            size_t index = it->second.builder->BuildEntity(e);
            (SetComponent<Cs>(it->second.builder, index, std::forward<Cs>(args)), ...);

            _entityToArchetype.try_emplace(e, archetypeID);
            return e;
        }

        return Entity::Invalid;
    }

    template <typename... Cs>
    std::tuple<Cs const&...> Scene::Components(Entity e) const
    {
        uint64_t archetypeID = _entityToArchetype.at(e);
        const EntityBuilder* builder = _archetypes.at(archetypeID).builder;

        size_t entityIdx = builder->EntityToIndex(e);
        return { *reinterpret_cast<const Cs*>(builder->Component(entityIdx, TypeID<Cs>()).data())... };
    }

    template <typename... Cs>
    std::tuple<Cs&...> Scene::Components(Entity e)
    {
        uint64_t archetypeID = _entityToArchetype.at(e);
        EntityBuilder* builder = _archetypes.at(archetypeID).builder;

        size_t entityIdx = builder->EntityToIndex(e);
        return { *reinterpret_cast<Cs*>(builder->Component(entityIdx, TypeID<Cs>()).data())... };
    }

    template <typename... Cs>
    QueryResult<Cs...> Scene::Query(MemoryScope& scope)
    {
        uint64_t archetypeID = EntityArchetype<Cs...>::ID;
        auto it = _archetypes.find(archetypeID);
        RN_ASSERT(it != _archetypes.end());

        ScopedVector<EntityBuilder*> builders;
        builders.reserve(it->second.superArchetypes.size());

        for (uint64_t archetypeID : it->second.superArchetypes)
        {
            builders.push_back(_archetypes.at(archetypeID).builder);
        }

        size_t entityCount = 0;
        for (const EntityBuilder* builder : builders)
        {
            entityCount += builder->EntityCount();
        }

        QueryResult<Cs...> result = { 
            ScopedNewArrayNoInit<std::tuple<Cs&...>>(scope, entityCount), entityCount
        };

        size_t offset = 0;
        for (EntityBuilder* builder : builders)
        {
            size_t countInBuilder = builder->EntityCount();
            for (size_t i = 0; i < countInBuilder; ++i, ++offset)
            {
                new (&result[offset]) std::tuple<Cs&...>({*reinterpret_cast<Cs*>(builder->Component(i, TypeID<Cs>()).data())...});
            }
        }

        return result;
    }

    template <typename C>
    ComponentDesc MakeComponentDesc()
    {
        return {
            .buildStorage = []() -> ComponentStorageBase*
            {
                return TrackedNew<ComponentStorage<C>>(MemoryCategory::Scene);
            },
            .typeID = TypeID<C>()
        };
    }

    template <typename... Cs>
    void Scene::RegisterArchetype()
    {
        return RegisterArchetype({ MakeComponentDesc<Cs>()... });
    }
}