#pragma once

#include "common/common.hpp"
#include "common/pack_sort.hpp"
#include "common/memory/hash_map.hpp"
#include "common/memory/span.hpp"
#include "common/memory/vector.hpp"

namespace rn::scene
{
    class Scene;

    RN_MEMORY_CATEGORY(Scene);
    enum class Entity : uint64_t
    {
        Invalid = 0
    };

    inline bool IsValid(Entity e) { return e != Entity::Invalid; }

    template <typename... Cs>
    struct _EntityArchetype
    {
        static const uint64_t ID;
    };

    template <typename... Cs>
    using EntityArchetype = meta::InstantiateSortedT<_EntityArchetype, std::remove_cvref_t<Cs>...>;

    class ComponentStorageBase
    {
    public:

        virtual ~ComponentStorageBase() {};

        virtual void        AddComponent() = 0;
        virtual void        RemoveComponent(size_t idx) = 0;
        virtual void*       Component(size_t idx) = 0;
        virtual const void* Component(size_t idx) const = 0;
        virtual size_t      Count() const = 0;
        virtual uint64_t    ID() const = 0;
        virtual size_t      ComponentSize() const = 0;
    };

    template <typename T>
    class ComponentStorage : public ComponentStorageBase
    {
    public:

        ~ComponentStorage() override {};

        void        AddComponent() override;
        void        RemoveComponent(size_t idx) override;
        void*       Component(size_t idx) override;
        const void* Component(size_t idx) const override;
        size_t      Count() const override;
        uint64_t    ID() const { return TypeID<T>(); }
        size_t      ComponentSize() const override { return sizeof(T); }

    private:

        Vector<T> _storage = MakeVector<T>(MemoryCategory::Scene);
    };

    struct ComponentDesc
    {
        using FnBuildStorage = ComponentStorageBase*(*)();

        FnBuildStorage buildStorage;
        uint64_t typeID;
    };

    template <typename C>
    ComponentDesc MakeComponentDesc();

    class EntityBuilder
    {
    public:

        EntityBuilder(Span<ComponentStorageBase*> componentStorage);
        ~EntityBuilder();

        size_t              BuildEntity(Entity entity);
        size_t              EntityToIndex(Entity entity) const;
        Span<uint8_t>       Component(size_t entityIdx, uint64_t typeID);
        Span<const uint8_t> Component(size_t entityIdx, uint64_t typeID) const;
        size_t              EntityCount() const;

    private:

        using EntityToIndexMap  = HashMap<Entity, size_t>;
        using StorageVector     = Vector<ComponentStorageBase*>;

        EntityToIndexMap    _entityToIndex = MakeHashMap<Entity, size_t>(MemoryCategory::Scene);
        StorageVector       _storage = MakeVector<ComponentStorageBase*>(MemoryCategory::Scene);
    };

    template <typename... Cs>
    using QueryResult = Span<std::tuple<Cs&...>>;

    class Scene
    {
    private:

        struct ArchetypeDesc
        {
            EntityBuilder*      builder = nullptr;
            Vector<uint64_t>    componentIDs = MakeVector<uint64_t>(MemoryCategory::Scene);
            Vector<uint64_t>    superArchetypes = MakeVector<uint64_t>(MemoryCategory::Scene);
        };

        using ArchetypeMap      = HashMap<uint64_t, ArchetypeDesc>;
        using EntityToArchetype = HashMap<Entity, uint64_t>;

    public:

        Scene();
        Scene(Scene&& rhs);
        Scene(const Scene&) = delete;
        ~Scene();

        Scene& operator=(const Scene&) = delete;
        Scene& operator=(Scene&& rhs);

        void RegisterArchetype(Span<const ComponentDesc> components);

        template <typename... Args>
        void RegisterArchetype();

        template <typename... Cs>
        Entity AddEntity(std::string_view identifier, Cs&&... args);

        Entity AddEntityFromDynamicArchetype(std::string_view identifier, Span<const ComponentDesc> components);

        Span<uint8_t>       Component(Entity e, uint64_t typeID);
        Span<const uint8_t> Component(Entity e, uint64_t typeID) const;

        template <typename... Cs>
        std::tuple<Cs const&...> Components(Entity e) const;

        template <typename... Cs>
        std::tuple<Cs&...> Components(Entity e);

        template <typename... Cs>
        QueryResult<Cs...> Query(MemoryScope& scope);

    private:

        ArchetypeMap        _archetypes = MakeHashMap<uint64_t, ArchetypeDesc>(MemoryCategory::Scene);
        EntityToArchetype   _entityToArchetype = MakeHashMap<Entity, uint64_t>(MemoryCategory::Scene);
    };
}

#include "scene/scene.inl"