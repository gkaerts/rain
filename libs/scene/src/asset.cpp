#include "scene/asset.hpp"

#include "scene_gen.hpp"
#include "luagen/schema.hpp"

namespace rn::scene
{
    SceneBuilder::SceneBuilder()
    {}

    Scene SceneBuilder::Build(const asset::AssetBuildDesc& desc)
    {
        MemoryScope SCOPE;
        schema::Scene asset = rn::Deserialize<schema::Scene>(desc.data,
            [](size_t size) { 
                return ScopedAlloc(size, CACHE_LINE_TARGET_SIZE); 
            });

        const asset::Registry* registry = desc.registry;

        Scene scene;
        for (const schema::Entity& schemaEntity : asset.entities)
        {
            MemoryScope SCOPE;
            ScopedVector<scene::ComponentDesc> components;
            components.reserve(schemaEntity.components.size());

            for (const schema::Component& c : schemaEntity.components)
            {
                components.push_back({
                    .buildStorage = nullptr,
                    .typeID = c.typeID
                });
            }

            Entity e = scene.AddEntityFromDynamicArchetype(schemaEntity.name, components);
            if (IsValid(e))
            {
                for (const schema::Component& c : schemaEntity.components)
                {
                    Span<uint8_t> componentData = scene.Component(e, c.typeID);
                    Span<const uint8_t> srcData = c.data;
                    RN_ASSERT(componentData.size() == srcData.size());

                    if (componentData.size() == srcData.size())
                    {
                        std::memcpy(componentData.data(), srcData.data(), componentData.size());
                    }
                }
            }
        }

        return scene;
    }

    void SceneBuilder::Destroy(Scene& scene)
    {}

    void SceneBuilder::Finalize()
    {}

}