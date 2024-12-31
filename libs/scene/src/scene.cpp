#include "scene/scene.hpp"
#include "common/memory/string.hpp"


namespace rn::scene
{
    RN_DEFINE_MEMORY_CATEGORY(Scene);

    EntityBuilder::EntityBuilder(Span<ComponentStorageBase*> componentStorage)
        : _storage(componentStorage.begin(), componentStorage.end())
    {
        RN_ASSERT(!_storage.empty());
    }

    EntityBuilder::~EntityBuilder()
    {
        for (ComponentStorageBase* storage : _storage)
        {
            TrackedDelete(storage);
        }
        _storage.clear();
    }

    size_t EntityBuilder::BuildEntity(Entity entity)
    {
        size_t index = std::numeric_limits<size_t>::max();
        auto it = _entityToIndex.find(entity);
        if (it == _entityToIndex.end())
        {
            index = _storage[0]->Count();
            _entityToIndex[entity] = index;

            for (ComponentStorageBase* storage : _storage)
            {
                storage->AddComponent();
            }
        }

        return index;
    };

    size_t EntityBuilder::EntityToIndex(Entity entity) const
    {
        return _entityToIndex.at(entity);
    }

    Span<uint8_t> EntityBuilder::Component(size_t entityIdx, uint64_t typeID)
    {
        for (ComponentStorageBase* storage : _storage)
        {
            if (typeID == storage->ID())
            {
                return { static_cast<uint8_t*>(storage->Component(entityIdx)), storage->ComponentSize() };
            }
        }
        
        return {};
    }

    Span<const uint8_t> EntityBuilder::Component(size_t entityIdx, uint64_t typeID) const
    {
        for (const ComponentStorageBase* storage : _storage)
        {
            if (typeID == storage->ID())
            {
                return { static_cast<const uint8_t*>(storage->Component(entityIdx)), storage->ComponentSize() };
            }
        }
        
        return {};
    }

    size_t EntityBuilder::EntityCount() const
    {
        return _storage[0]->Count();
    }

    std::vector<FnRegisterArchetype>& ArchetypeRegistrationFunctions()
    {
        // This is a plain std::vector because memory tracking might not be online yet by the time this is calles
        static std::vector<FnRegisterArchetype> archetypeFns;
        return archetypeFns;
    }

    Scene::Scene()
    {
        const std::vector<FnRegisterArchetype>& registerFns = ArchetypeRegistrationFunctions();
        for (FnRegisterArchetype fn : registerFns)
        {
            fn(*this);
        }
    }

    Scene::Scene(Scene&& rhs)
        : _archetypes(std::move(rhs._archetypes))
        , _entityToArchetype(std::move(rhs._entityToArchetype))
    {
        
    }

    Scene::~Scene()
    {
        for (const auto& p : _archetypes)
        {
            TrackedDelete(p.second.builder);
        }
    }

    Scene& Scene::operator=(Scene&& rhs)
    {
        if (this != &rhs)
        {
            _archetypes = std::move(rhs._archetypes);
            _entityToArchetype = std::move(rhs._entityToArchetype);
        }

        return *this;
    }

    void Scene::RegisterArchetype(Span<const ComponentDesc> components)
    {
        MemoryScope SCOPE;
        ScopedVector<ComponentDesc> descs(components.begin(), components.end());
        std::sort(descs.begin(), descs.end(), [](const ComponentDesc& a, const ComponentDesc& b)
        {
            return a.typeID < b.typeID;
        });

        uint64_t archetypeID = components[0].typeID;
        for (size_t i = 1; i < components.size(); ++i)
        {
            archetypeID = archetypeID ^ components[i].typeID;
        }

        auto it = _archetypes.find(archetypeID);
        if (it == _archetypes.end())
        {
            ScopedVector<ComponentStorageBase*> componentStorage;
            componentStorage.reserve(components.size());

            ScopedVector<uint64_t> componentIDs;
            componentIDs.reserve(components.size());

            for (const ComponentDesc& desc : descs)
            {
                componentStorage.push_back(desc.buildStorage());
                componentIDs.push_back(desc.typeID);
            }
            

            ArchetypeDesc desc = {
                .builder = TrackedNew<EntityBuilder>(MemoryCategory::Scene, componentStorage),
                .componentIDs = { componentIDs.begin(), componentIDs.end() },
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

            _archetypes.try_emplace(archetypeID, std::move(desc)).first;
        }
    }

    Entity Scene::AddEntityFromDynamicArchetype(std::string_view identifier, Span<const ComponentDesc> components)
    {
        uint64_t archetypeID = components[0].typeID;
        for (size_t i = 1; i < components.size(); ++i)
        {
            archetypeID = archetypeID ^ components[i].typeID;
        }

        auto it = _archetypes.find(archetypeID);
        RN_ASSERT(it != _archetypes.end());

        if (it != _archetypes.end())
        {
            Entity e = Entity(HashString(identifier));
            it->second.builder->BuildEntity(e);
            _entityToArchetype.try_emplace(e, archetypeID);
            return e;
        }

        return Entity::Invalid;
    }

    Span<uint8_t> Scene::Component(Entity e, uint64_t typeID)
    {
        uint64_t archetypeID = _entityToArchetype.at(e);
        EntityBuilder* builder = _archetypes.at(archetypeID).builder;
        size_t entityIdx = builder->EntityToIndex(e);
        return builder->Component(entityIdx, typeID);
    }

    Span<const uint8_t> Scene::Component(Entity e, uint64_t typeID) const
    {
        uint64_t archetypeID = _entityToArchetype.at(e);
        EntityBuilder* builder = _archetypes.at(archetypeID).builder;
        size_t entityIdx = builder->EntityToIndex(e);
        return builder->Component(entityIdx, typeID);
    }
}