#pragma once

#include "common/common.hpp"
#include "common/handle.hpp"
#include "common/pack_sort.hpp"
#include "common/memory/bump_allocator.hpp"
#include "common/memory/hash_map.hpp"
#include "common/memory/span.hpp"
#include "common/memory/vector.hpp"

namespace rn::scene
{
    RN_MEMORY_CATEGORY(Scene);

    RN_DEFINE_HANDLE(Entity, 0x90);

    class EntityBuilderBase
    {
    public:

        virtual ~EntityBuilderBase() {}

        virtual size_t      BuildEntity(Entity entity) = 0;
        virtual size_t      EntityToIndex(Entity entity) const = 0;
        virtual void*       Component(size_t entityIdx, uint64_t typeID) = 0;
        virtual const void* Component(size_t entityIdx, uint64_t typeID) const = 0;
        virtual void*       ComponentArray(uint64_t typeID) = 0;
        virtual const void* ComponentArray(uint64_t typeID) const = 0;
        virtual size_t      EntityCount() const = 0;
    };

    template <typename... Components>
    class EntityBuilder : public EntityBuilderBase
    {
    public:

        ~EntityBuilder() override {}

        size_t      BuildEntity(Entity entity) override;
        size_t      EntityToIndex(Entity entity) const override;
        void*       Component(size_t entityIdx, uint64_t typeID) override;
        const void* Component(size_t entityIdx, uint64_t typeID) const override;
        void*       ComponentArray(uint64_t typeID);
        const void* ComponentArray(uint64_t typeID) const;
        size_t      EntityCount() const override;

    private:

        using EntityToIndexMap  = HashMap<Entity, size_t>;
        using ComponentVecTuple = std::tuple<Vector<Components>...>;
        using TypeIDArray       = std::array<uint64_t, sizeof...(Components)>;

        TypeIDArray         _componentTypeIDs = { TypeID<Components>()... };
        EntityToIndexMap    _entityToIndex = MakeHashMap<Entity, size_t>(MemoryCategory::Scene);
        ComponentVecTuple   _storage = { MakeVector<Components>(MemoryCategory::Scene)... };
    };

    template <typename... Cs>
    using QueryResult = Span<std::tuple<Cs&...>>;

    class Scene
    {
    private:

        struct ArchetypeDesc
        {
            EntityBuilderBase*  builder = nullptr;
            Vector<uint64_t>    componentIDs = MakeVector<uint64_t>(MemoryCategory::Scene);
            Vector<uint64_t>    superArchetypes = MakeVector<uint64_t>(MemoryCategory::Scene);
        };

        using ArchetypeMap      = HashMap<uint64_t, ArchetypeDesc>;
        using EntityToArchetype = HashMap<Entity, uint64_t>;

    public:

        Scene() = default;
        ~Scene();

        template <typename... Args>
        Entity AddEntity(Args&&... args);

        template <typename... Cs>
        std::tuple<Cs const&...> Components(Entity e) const;

        template <typename... Cs>
        std::tuple<Cs&...> Components(Entity e);

        template <typename... Cs>
        QueryResult<Cs...> Query();

    private:

        template <typename... Args>
        ArchetypeMap::iterator MakeArchetype();

        ArchetypeMap        _archetypes = MakeHashMap<uint64_t, ArchetypeDesc>(MemoryCategory::Scene);
        EntityToArchetype   _entityToArchetype = MakeHashMap<Entity, uint64_t>(MemoryCategory::Scene);
        uint64_t            _currentEntityID = 0;

        BumpAllocator       _tempAllocator = { MemoryCategory::Scene, 16 * MEGA };

    };
}

#include "scene/scene.inl"