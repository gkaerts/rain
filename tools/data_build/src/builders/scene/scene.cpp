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

        pxr::GfMatrix4d BuildGlobalTransform(const DataBuildContext& ctxt, const pxr::UsdPrim& prim, const pxr::GfMatrix4d& parentXform)
        {
            pxr::UsdGeomXform xform(prim);
            pxr::GfMatrix4d xformMatrix;

            {
                bool resetStack = false;
                if (!xform.GetLocalTransformation(&xformMatrix, &resetStack))
                {
                    BuildError(ctxt) << "Failed to extract local transform for entity prim." << std::endl;
                }
            }

            return parentXform * xformMatrix;
        }

        bool BuildTransformComponent(const DataBuildContext& ctxt, const pxr::UsdPrim& prim, const pxr::GfMatrix4d& globalXform, BumpAllocator& blobAllocator, Vector<scene::schema::Component>& outComponents)
        {
            pxr::UsdGeomXform xform(prim);
            pxr::GfMatrix4d xformMatrix;

            {
                bool resetStack = false;
                if (!xform.GetLocalTransformation(&xformMatrix, &resetStack))
                {
                    BuildError(ctxt) << "Failed to extract local transform for entity prim." << std::endl;
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

        void BuildChildOfComponent(const DataBuildContext& ctxt, const pxr::UsdPrim& prim, uint64_t parentID, BumpAllocator& blobAllocator, Vector<scene::schema::Component>& outComponents)
        {
            scene::schema::ChildOf childOf = {
                .parent = scene::Entity(parentID)
            };

            PushComponent(childOf, blobAllocator, outComponents);
        }

        constexpr const uint64_t NO_PARENT = std::numeric_limits<uint64_t>::max();

        bool ProcessComponents(const DataBuildContext& ctxt, const pxr::UsdPrim& prim, BumpAllocator& blobAllocator, Vector<std::string>& outReferences, Vector<scene::schema::Component>& outComponents, uint64_t& outArchetypeID)
        {
            for (const ComponentHandler& handler : ComponentHandlers())
            {
                if (handler.fnHasAPI(prim))
                {
                    outArchetypeID = outArchetypeID ^ handler.typeID;
                    handler.fnBuildComponent(ctxt, prim, blobAllocator, outReferences, outComponents);
                }
            }

            return true;
        }

        bool ProcessEntity(const DataBuildContext& ctxt, const pxr::UsdPrim& prim, uint64_t parentID, const pxr::GfMatrix4d& parentXform, BumpAllocator& blobAllocator, Vector<std::string>& outReferences, Vector<EntityData>& outEntities, Vector<scene::schema::Component>& outComponents)
        {
            std::string_view name = prim.GetPath().GetString();
            uint64_t entityID = HashString(name);

            pxr::GfMatrix4d xform = BuildGlobalTransform(ctxt, prim, parentXform);

            pxr::UsdPrimSiblingRange children = prim.GetAllChildren();
            for (const pxr::UsdPrim& child : children)
            {
                if (child.IsA<pxr::UsdGeomXform>())
                {
                    // Child entity! Process it first
                    if (!ProcessEntity(ctxt, child, entityID, xform, blobAllocator, outReferences, outEntities, outComponents))
                    {
                        BuildError(ctxt) << "Failed to process child prim: \"" << child.GetPath().GetString() << "\"." << std::endl;
                        return false;
                    }
                }
            }

            size_t startComponentIdx = outComponents.size();
            if (!BuildTransformComponent(ctxt, prim, xform, blobAllocator, outComponents))
            {
                return false;
            }

            uint64_t archetypeID = 0;
            if (!ProcessComponents(ctxt, prim, blobAllocator, outReferences, outComponents, archetypeID))
            {
                return false;
            }

            if (parentID != NO_PARENT)
            {
                BuildChildOfComponent(ctxt, prim, parentID, blobAllocator, outComponents);
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

    int ProcessUsdScene(const DataBuildContext& ctxt, const pxr::UsdPrim& prim, Vector<std::string>& outFiles)
    {
        BumpAllocator blobAllocator(MemoryCategory::Default, 64 * MEGA);

        Vector<std::string> references;
        Vector<EntityData> entityData;
        Vector<scene::schema::Component> components;

        pxr::GfMatrix4d rootXform;

        pxr::UsdPrimSiblingRange children = prim.GetAllChildren();
        for (const pxr::UsdPrim& child : children)
        {
            if (child.IsA<pxr::UsdGeomXform>())
            {
                // This is an entity
                if (!ProcessEntity(ctxt, child, NO_PARENT, rootXform, blobAllocator, references, entityData, components))
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

        ScopedVector<std::string_view> refViews;
        refViews.reserve(references.size());

        for (const std::string& ref : references)
        {
            refViews.push_back(ref);
        }

        return WriteAssetToDisk(ctxt, ".scene", assetData, refViews, outFiles);
    }
}