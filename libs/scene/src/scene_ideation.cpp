#include "common/common.hpp"
#include "common/type_id.hpp"

#include "common/memory/hash_map.hpp"
#include "common/memory/span.hpp"
#include "common/memory/vector.hpp"
#include "common/pack_sort.hpp"

#include <algorithm>
#include <array>

using namespace std::literals;

namespace rn::scene
{
    RN_DEFINE_MEMORY_CATEGORY(Scene);

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

    enum class Entity : uint32_t 
    {
        Invalid = 0xFFFFFFFF
    };

    class EntityBuilderBase
    {
    public:

        virtual void        BuildEntity(Entity entity) = 0;
        virtual size_t      EntityToIndex(Entity entity) const = 0;
        virtual void*       Component(size_t entityIdx, uint64_t typeID) = 0;
        virtual const void* Component(size_t entityIdx, uint64_t typeID) const = 0;
    };

    template <typename... Components>
    class EntityBuilder : public EntityBuilderBase
    {
    public:

        void BuildEntity(Entity entity) override
        {
            auto it = _entityToIndex.find(entity);
            if (it == _entityToIndex.end())
            {
                size_t index = std::get<0>(_storage).size();
                _entityToIndex[entity] = index;

                (std::get<Vector<Components>>(_storage).emplace_back(), ...);
            }
        };

        size_t EntityToIndex(Entity entity) const override
        {
            return _entityToIndex.at(entity);
        }

        void* Component(size_t entityIdx, uint64_t typeID) override
        {
            void* ptr = nullptr;
            (ResolvePtrAtIndex(std::get<Vector<Components>>(_storage), typeID, entityIdx, ptr), ...);

            return ptr;
        }

        const void* Component(size_t entityIdx, uint64_t typeID) const override
        {
            const void* ptr = nullptr;
            (ResolveConstPtrAtIndex(std::get<Vector<Components>>(_storage), typeID, entityIdx, ptr), ...);

            return ptr;
        }

    private:

        using EntityToIndexMap = HashMap<Entity, size_t>;
        using ComponentVecTuple = std::tuple<Vector<Components>...>;
        using TypeIDArray = std::array<uint64_t, sizeof...(Components)>;

        TypeIDArray         _componentTypeIDs = { TypeID<Components>()... };
        EntityToIndexMap    _entityToIndex = MakeHashMap<Entity, size_t>(MemoryCategory::Scene);
        ComponentVecTuple   _storage = { MakeVector<Components>(MemoryCategory::Scene)... };
    };

    class Scene
    {
    private:

        struct ArchetypeDesc
        {
            EntityBuilderBase* builder = nullptr;
        };

        using ArchetypeMap = HashMap<uint64_t, ArchetypeDesc>;
        using EntityToArchetype = HashMap<Entity, uint64_t>;

    public:

        template <typename... Args>
        Entity AddEntity(Args&&... args)
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
                Entity e = Entity(_currentEntityID++);
                it->second.builder->BuildEntity(e);
                return e;
            }

            return Entity::Invalid;
        }

        template <typename... Cs>
        std::tuple<Cs const&...> Components(Entity e) const
        {
            uint64_t archetypeID = _entityToArchetype.at(e);
            const EntityBuilderBase* builder = _archetypes.at(archetypeID).builder;

            size_t entityIdx = builder->EntityToIndex(e);
            return { *static_cast<const Cs*>(builder->Component(entityIdx, TypeID<Cs>()))... };
        }

        template <typename... Cs>
        std::tuple<Cs&...> Components(Entity e)
        {
            uint64_t archetypeID = _entityToArchetype.at(e);
            EntityBuilderBase* builder = _archetypes.at(archetypeID).builder;

            size_t entityIdx = builder->EntityToIndex(e);
            return { *static_cast<Cs*>(builder->Component(entityIdx, TypeID<Cs>()))... };
        }

    private:

        template <typename... Args>
        ArchetypeMap::iterator MakeArchetype()
        {
            constexpr uint64_t archetypeID = MergeTypeIDs<Args...>();
            auto it = _archetypes.find(archetypeID);
            RN_ASSERT(it == _archetypes.end());

            if (it == _archetypes.end())
            {
                using Builder = meta::InstantiateSortedT<EntityBuilder, Args...>;
                it = _archetypes.try_emplace(archetypeID, TrackedNew<Builder>(MemoryCategory::Scene)).first;
            }

            return it;
        }

        ArchetypeMap        _archetypes = MakeHashMap<uint64_t, ArchetypeDesc>(MemoryCategory::Scene);
        EntityToArchetype   _entityToArchetype = MakeHashMap<Entity, uint64_t>(MemoryCategory::Scene);
        uint64_t            _currentEntityID = 0;

    };
    struct ComponentA
    {
        uint32_t foo;
    };

    struct ComponentB
    {
        float bar;
    };

    struct ComponentC
    {
        std::string_view baz;
    };

    void SceneStuff()
    {
        Scene scene;
        Entity e = scene.AddEntity<ComponentA, ComponentB>({}, {});

        {
            auto [a, b, c] = scene.Components<ComponentA, ComponentB, ComponentC>(e);
            a = { .foo = 0xBABADAFA };
            b = { .bar = 1.5f };
            c = { .baz = "Hello World"sv };
        }

        {
            auto [c, a] = scene.Components<ComponentC, ComponentA>(e);
        }
        
    }
}