#pragma warning(push, 1)
#include <pxr/usd/usdGeom/mesh.h>
#include "rn/renderMeshComponentAPI.h"
#include "rn/renderMaterialListComponentAPI.h"
#pragma warning(pop)

#include "builders/build.hpp"
#include "builders/usd.hpp"
#include "builders/scene/scene.hpp"

#include "asset/asset.hpp"

#include "mesh_gen.hpp"

namespace rn
{
    const PrimSchema RENDER_MESH_PRIM_SCHEMA = {
        .requiredProperties = {
            { .name = "rendermesh:geometry", .fnIsValidRelationship = RelatedPrimIsA<pxr::UsdGeomMesh> },
        },
    };

    const PrimSchema RENDER_MATERIAL_LIST_PRIM_SCHEMA = {
        .requiredProperties = {
            { .name = "rendermateriallist:materials", .fnIsValidRelationship = RelatedPrimIsA<pxr::UsdGeomMesh> },
        },
    };

    bool BuildRenderMeshComponent(const DataBuildContext& ctxt, const pxr::RnRenderMeshComponentAPI& usdMesh, BumpAllocator& blobAllocator, Vector<std::string>& outReferences, render::schema::RenderMesh& outMesh)
    {
        pxr::UsdStageWeakPtr stage = usdMesh.GetPrim().GetStage();

        pxr::SdfPathVector meshPaths = ResolveRelationTargets(usdMesh.GetGeometryRel());
        pxr::UsdPrim meshPrim = stage->GetPrimAtPath(meshPaths[0]);
        pxr::VtValue assetId = meshPrim.GetAssetInfo()["identifier"];
        if (!assetId.IsHolding<pxr::SdfAssetPath>())
        {
            BuildError(ctxt) << "Mesh component not pointing to a valid asset. (Path: \"" << meshPaths[0] << "\")" << std::endl;
            return false;
        }

        std::filesystem::path geometryPath = MakeAbsoluteAssetReferencePath(ctxt,
            assetId.Get<pxr::SdfAssetPath>(), 
            "geometry");

        outReferences.emplace_back(geometryPath.string());
        outMesh.geometry = asset::MakeAssetIdentifier(MakeRootRelativeReferencePath(ctxt, outReferences.back()).string());
        return true;
    }

    bool BuildRenderMaterialListComponent(const DataBuildContext& ctxt, const pxr::RnRenderMaterialListComponentAPI& usdList, BumpAllocator& blobAllocator, Vector<std::string>& outReferences, render::schema::RenderMaterialList& outMesh)
    {
        pxr::UsdStageWeakPtr stage = usdList.GetPrim().GetStage();

        pxr::SdfPathVector materialPaths = ResolveRelationTargets(usdList.GetMaterialsRel());
        if (materialPaths.size() > outMesh.materials.size())
        {
            BuildWarning(ctxt) << "Specified material count exceeds the amount of supported materials. Truncating." << std::endl;
        }

        uint32_t matIdx = 0;
        for (const pxr::SdfPath& path : materialPaths)
        {
            pxr::UsdPrim materialPrim = stage->GetPrimAtPath(path);
            pxr::VtValue assetId = materialPrim.GetAssetInfo()["identifier"];
            if (!assetId.IsHolding<pxr::SdfAssetPath>())
            {
                BuildError(ctxt) << "Material list component not pointing to a valid asset. (Path: \"" << path << "\")" << std::endl;
                return false;
            }

            std::filesystem::path assetPath = MakeAbsoluteAssetReferencePath(ctxt,
                assetId.Get<pxr::SdfAssetPath>(), 
                "material");

            outReferences.emplace_back(assetPath.string());
            outMesh.materials[matIdx++] = asset::MakeAssetIdentifier(MakeRootRelativeReferencePath(ctxt, outReferences.back()).string());

            if (matIdx >= outMesh.materials.size())
            {
                break;
            }
        }

        return true;
    }

    struct RegisterRenderMeshComponents
    {
        RegisterRenderMeshComponents()
        {
            const ComponentHandler handlers[] = {
                MakeHasAPIHandler<BuildRenderMeshComponent>(),
                MakeHasAPIHandler<BuildRenderMaterialListComponent>(),
            };

            for (const ComponentHandler& handler : handlers)
            {
                RegisterComponentHandler(handler);
            }
        }
    };

    static RegisterRenderMeshComponents _RegisterRenderMeshComponents;
}