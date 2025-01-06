#pragma warning(push, 1)
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/property.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usd/relationship.h>

#include <pxr/usd/usd/primRange.h>
#include <pxr/usd/usdGeom/metrics.h>
#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/usdGeom/xform.h>
#include <pxr/usd/usdGeom/scope.h>

#include <pxr/usd/usdShade/material.h>

#include "rn/texture.h"
#include "rn/materialShader.h"
#include "rn/rainMaterialAPI.h"
#include "rn/tokens.h"
#pragma warning(pop)

#include "usd.hpp"

#include "build.hpp"
#include "geometry.hpp"
#include "texture.hpp"
#include "material_shader.hpp"
#include "material.hpp"
#include "scene/scene.hpp"
namespace rn
{
    std::filesystem::path MakeAssetReferencePath(const DataBuildContext& ctxt, const pxr::SdfAssetPath& path, const std::filesystem::path& extension)
    {
        std::string assetPath = path.GetResolvedPath();
        std::filesystem::path outPath = MakeRelativeTo(ctxt.file, assetPath);
        outPath.replace_extension(extension);

        return outPath;
    }

    std::filesystem::path MakeAbsoluteAssetReferencePath(const DataBuildContext& ctxt, const pxr::SdfAssetPath& path, const std::filesystem::path& extension)
    {
        std::filesystem::path outPath = path.GetResolvedPath();
        outPath.replace_extension(extension);

        return outPath;
    }

    bool IsNonEmptyAssetPath(const DataBuildContext& ctxt, const pxr::UsdAttribute& prop)
    {
        auto str = Value<pxr::SdfAssetPath>(prop);
        return !str.GetAssetPath().empty();
    }

    bool IsNonEmptyString(const DataBuildContext& ctxt, const pxr::UsdAttribute& prop)
    {
        pxr::TfToken str = Value<pxr::TfToken>(prop);
        return !str.IsEmpty();
    }
    
    pxr::SdfPathVector ResolveRelationTargets(const pxr::UsdRelationship& rel)
    {
        pxr::SdfPathVector paths;
        rel.GetTargets(&paths);
        return paths;
    }

    bool IsRelationshipNotEmpty(const DataBuildContext& ctxt, const pxr::UsdProperty& prop)
    {
        const pxr::UsdRelationship* rel = static_cast<const pxr::UsdRelationship*>(&prop);

        pxr::SdfPathVector paths;
        rel->GetTargets(&paths);

        if (paths.empty())
        {
            BuildError(ctxt) << "Required relationship does not have any values" << std::endl;
            return false;
        }

        return true;
    }

    bool ValidatePrim(const DataBuildContext& ctxt, const pxr::UsdPrim& prim, const PrimSchema& schema)
    {
        for (const PrimProperty& key : schema.requiredProperties)
        {
            auto nameToken = pxr::TfToken(key.name);
            pxr::UsdProperty prop = prim.GetProperty(nameToken);
            if (!prop || !prop.IsAuthored())
            {
                BuildError(ctxt) << "Required property '" << key.name << "' not found in prim \"" << prim.GetPath() << "\"" << std::endl;
                return false;
            }

           
            if (pxr::UsdAttribute attr = prim.GetAttribute(nameToken); attr && key.fnIsValidPropertyValue)
            {
                if (!key.fnIsValidPropertyValue(ctxt, attr))
                {
                    BuildError(ctxt) << "Required attribute '" << key.name << "' has an unsupported value." << std::endl;
                    return false;
                }
            }

            if (pxr::UsdRelationship rel = prim.GetRelationship(nameToken); rel)
            {
                if (ResolveRelationTargets(rel).empty())
                {
                    BuildError(ctxt) << "Required relationship '" << key.name << "' does not list any elements." << std::endl;
                    return false;
                }

                if (key.fnIsValidRelationship && !key.fnIsValidRelationship(ctxt, *prim.GetStage(), rel))
                {
                    BuildError(ctxt) << "Required relationship '" << key.name << "' is invalid." << std::endl;
                    return false;
                }
            }
            
        }

        for (const PrimProperty& key : schema.optionalProperties)
        {
            auto nameToken = pxr::TfToken(key.name);
            if (pxr::UsdAttribute attr = prim.GetAttribute(nameToken); attr && key.fnIsValidPropertyValue)
            {
                if (!key.fnIsValidPropertyValue(ctxt, attr))
                {
                    BuildError(ctxt) << "Attribute '" << key.name << "' has an unsupported value." << std::endl;
                    return false;
                }
            }

            if (pxr::UsdRelationship rel = prim.GetRelationship(nameToken); rel)
            {
                if (key.fnIsValidRelationship && !key.fnIsValidRelationship(ctxt, *prim.GetStage(), rel))
                {
                    BuildError(ctxt) << "Relationship '" << key.name << "' is invalid." << std::endl;
                    return false;
                }
            }
        }

        return true;
    }

    int DoBuildUSD(const DataBuildContext& ctxt, Vector<std::string>& outFiles)
    {
        std::string filePath = {ctxt.file.data(), ctxt.file.length()};
        pxr::UsdStageRefPtr stage = pxr::UsdStage::Open(filePath);
        pxr::UsdPrimRange range = stage->Traverse();

        pxr::TfToken upAxis = pxr::UsdGeomGetStageUpAxis(stage);

        // Assume inputs are right-handed. We want left-handed, Y-up.
        FnPermuteVec3 fnPermute = [](const pxr::GfVec3f& v) { return pxr::GfVec3f(v[0], v[1], -v[2]); };
        if (upAxis == pxr::UsdGeomTokens->z)
        {
            fnPermute = [](const pxr::GfVec3f& v) { return pxr::GfVec3f(v[0], v[2], -v[1]); };
        }

        double unitScale = 1.0 / pxr::UsdGeomGetStageMetersPerUnit(stage);

        pxr::UsdPrim rootPrim = stage->GetDefaultPrim();
        if (!rootPrim.IsValid())
        {
            BuildError(ctxt) << "No valid default prim provided." << std::endl;
            return 1;
        }

        if (rootPrim.IsComponent())
        {
            // This is an asset USD file
            if (rootPrim.IsA<pxr::UsdGeomMesh>())
            {
                UsdGeometryBuildDesc desc = {
                    .mesh = pxr::UsdGeomMesh(rootPrim),
                    .fnPermute = fnPermute,
                    .unitScale = float(unitScale)
                };

                return ProcessUsdGeomMesh(ctxt, desc, outFiles);
            }
            else if (rootPrim.IsA<pxr::RnTexture>())
            {
                return ProcessUsdTexture(ctxt, rootPrim, outFiles);
            }
            else if (rootPrim.IsA<pxr::RnMaterialShader>())
            {
                return ProcessUsdMaterialShader(ctxt, rootPrim, outFiles);
            }
            else if (rootPrim.IsA<pxr::UsdShadeMaterial>() && rootPrim.HasAPI<pxr::RnRainMaterialAPI>())
            {
                return ProcessUsdMaterial(ctxt, rootPrim, outFiles);
            }
            
        }
        else if (rootPrim.IsA<pxr::UsdGeomScope>() || rootPrim.IsA<pxr::UsdGeomXform>())
        {
            // This is a "scene" USD file
            return ProcessUsdScene(ctxt, rootPrim, outFiles);
        }
        else
        {
            BuildError(ctxt) << "Root prim has an unsupported schema type. Stopping build." << std::endl;
            return 1;
        }
        
        return 0;
    }
}