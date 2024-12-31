#pragma warning(push, 1)
#include <pxr/usd/usdGeom/xform.h>
#pragma warning(pop)

#include "builders/build.hpp"
#include "builders/usd.hpp"
#include "scene.hpp"
#include "common/memory/string.hpp"

#include "luagen/schema.hpp"
#include "scene_gen.hpp"

// Components
#include "transform_gen.hpp"
#include "child_of_gen.hpp"



namespace rn
{
    RN_DEFINE_MEMORY_CATEGORY(TestCategory);
    namespace
    {
        struct EntityData
        {
            std::string name;
            uint64_t archetypeID;
            size_t startComponentIdx;
            size_t componentCount;
        };
       

        std::vector<ComponentHandler>& ComponentHandlers()
        {
            static std::vector<ComponentHandler> handlers;
            return handlers;
        }  

        pxr::GfMatrix4d BuildGlobalTransform(std::string_view file, const pxr::UsdPrim& prim, const pxr::GfMatrix4d& parentXform)
        {
            pxr::UsdGeomXform xform(prim);
            pxr::GfMatrix4d xformMatrix;

            {
                bool resetStack = false;
                if (!xform.GetLocalTransformation(&xformMatrix, &resetStack))
                {
                    BuildError(file) << "Failed to extract local transform for entity prim." << std::endl;
                }
            }

            return parentXform * xformMatrix;
        }

        bool BuildTransformComponent(std::string_view file, const pxr::UsdPrim& prim, const pxr::GfMatrix4d& globalXform, BumpAllocator& blobAllocator, Vector<scene::schema::Component>& outComponents)
        {
            pxr::UsdGeomXform xform(prim);
            pxr::GfMatrix4d xformMatrix;

            {
                bool resetStack = false;
                if (!xform.GetLocalTransformation(&xformMatrix, &resetStack))
                {
                    BuildError(file) << "Failed to extract local transform for entity prim." << std::endl;
                    return false;
                }
            }

            pxr::GfQuatd rotation = xformMatrix.ExtractRotationQuat();
            pxr::GfVec3d translation = xformMatrix.ExtractTranslation();

            scene::schema::Transform transform = 
            {
                .localPosition = { float(translation[0]), float(translation[1]), float(translation[2]) },
                .localRotation = { float(rotation.GetReal()), float(rotation.GetImaginary()[0]), float(rotation.GetImaginary()[1]), float(rotation.GetImaginary()[2]) },
                .globalTransform = {}
            };

            float* matVec = &transform.globalTransform.m00;
            for (int i = 0; i < 16; ++i)
            {
                matVec[i] = float(*globalXform[i]);
            }
            
            PushComponent(transform, blobAllocator, outComponents);
            return true;
        }

        void BuildChildOfComponent(std::string_view file, const pxr::UsdPrim& prim, uint64_t parentID, BumpAllocator& blobAllocator, Vector<scene::schema::Component>& outComponents)
        {
            scene::schema::ChildOf childOf = {
                .parent = scene::Entity(parentID)
            };

            PushComponent(childOf, blobAllocator, outComponents);
        }

        constexpr const uint64_t NO_PARENT = std::numeric_limits<uint64_t>::max();

        bool ProcessComponents(std::string_view file, const pxr::UsdPrim& prim, BumpAllocator& blobAllocator, Vector<scene::schema::Component>& outComponents, uint64_t& outArchetypeID)
        {
            for (const ComponentHandler& handler : ComponentHandlers())
            {
                if (handler.fnHasAPI(prim))
                {
                    outArchetypeID = outArchetypeID ^ handler.typeID;
                    handler.fnBuildComponent(file, prim, blobAllocator, outComponents);
                    break;
                }
            }

            return true;
        }

        bool ProcessEntity(std::string_view file, const pxr::UsdPrim& prim, uint64_t parentID, const pxr::GfMatrix4d& parentXform, BumpAllocator& blobAllocator, Vector<EntityData>& outEntities, Vector<scene::schema::Component>& outComponents)
        {
            std::string_view name = prim.GetPath().GetString();
            uint64_t entityID = HashString(name);

            pxr::GfMatrix4d xform = BuildGlobalTransform(file, prim, parentXform);

            pxr::UsdPrimSiblingRange children = prim.GetAllChildren();
            for (const pxr::UsdPrim& child : children)
            {
                if (child.IsA<pxr::UsdGeomXform>())
                {
                    // Child entity! Process it first
                    if (!ProcessEntity(file, child, entityID, xform, blobAllocator, outEntities, outComponents))
                    {
                        BuildError(file) << "Failed to process child prim: \"" << child.GetPath().GetString() << "\"." << std::endl;
                        return false;
                    }
                }
            }

            size_t startComponentIdx = outComponents.size();
            if (!BuildTransformComponent(file, prim, xform, blobAllocator, outComponents))
            {
                return false;
            }

            uint64_t archetypeID = 0;
            if (!ProcessComponents(file, prim, blobAllocator, outComponents, archetypeID))
            {
                return false;
            }

            if (parentID != NO_PARENT)
            {
                BuildChildOfComponent(file, prim, parentID, blobAllocator, outComponents);
            }

            outEntities.push_back({
                .name = prim.GetPath().GetString(),
                .archetypeID = archetypeID,
                .startComponentIdx = startComponentIdx,
                .componentCount = outComponents.size() - startComponentIdx
            });

            return true;
        }
    }

    void RegisterComponentHandler(const ComponentHandler& handler)
    {
        ComponentHandlers().push_back(handler);
    }

    int ProcessUsdScene(std::string_view file, const DataBuildOptions& options, const pxr::UsdPrim& prim, Vector<std::string>& outFiles)
    {
        BumpAllocator blobAllocator(MemoryCategory::Default, 64 * MEGA);

        Vector<EntityData> entityData = MakeVector<EntityData>(MemoryCategory::TestCategory);
        Vector<scene::schema::Component> components = MakeVector<scene::schema::Component>(MemoryCategory::TestCategory);

        pxr::GfMatrix4d rootXform;

        pxr::UsdPrimSiblingRange children = prim.GetAllChildren();
        for (const pxr::UsdPrim& child : children)
        {
            if (child.IsA<pxr::UsdGeomXform>())
            {
                // This is an entity
                if (!ProcessEntity(file, child, NO_PARENT, rootXform, blobAllocator, entityData, components))
                {
                    return 1;
                }
            }
            else
            {
                // Unknown prim type. Handle scene properties here in the future
            }
        }

        Vector<scene::schema::Entity> entities = MakeVector<scene::schema::Entity>(MemoryCategory::TestCategory);
        entities.reserve(entityData.size());
        for (const EntityData& e : entityData)
        {
            entities.push_back({
                .name = e.name,
                .archetypeID = e.archetypeID,
                .components = { components.data() + e.startComponentIdx, e.componentCount }
            });
        }

        scene::schema::Scene outScene = {
            .entities = entities
        };

        uint64_t outSize = scene::schema::Scene::SerializedSize(outScene);
        Span<uint8_t> assetData = { static_cast<uint8_t*>(ScopedAlloc(outSize, CACHE_LINE_TARGET_SIZE)), outSize };
        rn::Serialize(assetData, outScene);

        return WriteAssetToDisk(file, ".scene", options, assetData, {}, outFiles);
    }
}