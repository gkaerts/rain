#pragma warning(push, 1)
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/property.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usd/relationship.h>

#include <pxr/usd/usd/primRange.h>
#include <pxr/usd/usdGeom/metrics.h>
#include <pxr/usd/usdGeom/mesh.h>
#include "rn/texture.h"
#include "rn/materialShader.h"
#pragma warning(pop)

#include "usd.hpp"

#include "build.hpp"
#include "geometry.hpp"
#include "texture.hpp"
#include "material_shader.hpp"

namespace rn
{
    bool IsNonEmptyAssetPath(std::string_view file, const pxr::UsdAttribute& prop)
    {
        auto str = Value<pxr::SdfAssetPath>(prop);
        return !str.GetAssetPath().empty();
    }

    bool IsNonEmptyString(std::string_view file, const pxr::UsdAttribute& prop)
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

    bool IsRelationshipNotEmpty(std::string_view file, const pxr::UsdProperty& prop)
    {
        const pxr::UsdRelationship* rel = static_cast<const pxr::UsdRelationship*>(&prop);

        pxr::SdfPathVector paths;
        rel->GetTargets(&paths);

        if (paths.empty())
        {
            BuildError(file) << "Required relationship does not have any values" << std::endl;
            return false;
        }

        return true;
    }

    bool ValidatePrim(std::string_view file, const pxr::UsdPrim& prim, const PrimSchema& schema)
    {
        for (const PrimProperty& key : schema.requiredProperties)
        {
            auto nameToken = pxr::TfToken(key.name);
            pxr::UsdProperty prop = prim.GetProperty(nameToken);
            if (!prop || !prop.IsAuthored())
            {
                BuildError(file) << "Required property '" << key.name << "' not found in prim \"" << prim.GetPath() << "\"" << std::endl;
                return false;
            }

           
            if (pxr::UsdAttribute attr = prim.GetAttribute(nameToken); attr && key.fnIsValidPropertyValue)
            {
                if (!key.fnIsValidPropertyValue(file, attr))
                {
                    BuildError(file) << "Required attribute '" << key.name << "' has an unsupported value." << std::endl;
                    return false;
                }
            }

            if (pxr::UsdRelationship rel = prim.GetRelationship(nameToken); rel)
            {
                if (ResolveRelationTargets(rel).empty())
                {
                    BuildError(file) << "Required relationship '" << key.name << "' does not list any elements." << std::endl;
                    return false;
                }
            }
            
        }

        for (const PrimProperty& key : schema.optionalProperties)
        {
            auto nameToken = pxr::TfToken(key.name);
            if (pxr::UsdAttribute attr = prim.GetAttribute(nameToken); attr && key.fnIsValidPropertyValue)
            {
                if (!key.fnIsValidPropertyValue(file, attr))
                {
                    BuildError(file) << "Attribute '" << key.name << "' has an unsupported value." << std::endl;
                    return false;
                }
            }
        }

        return true;
    }

    int DoBuildUSD(std::string_view file, const DataBuildOptions& options, Vector<std::string>& outFiles)
    {
        std::string filePath = {file.data(), file.length()};
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

        for (const pxr::UsdPrim& prim : range)
        {
            if (prim.IsA<pxr::UsdGeomMesh>())
            {
                UsdGeometryBuildDesc desc = {
                    .mesh = pxr::UsdGeomMesh(prim),
                    .fnPermute = fnPermute,
                    .unitScale = float(unitScale)
                };
                
                if (!ProcessUsdGeomMesh(file, options, desc, outFiles))
                {
                    return 1;
                }
            }
            else if (prim.IsA<pxr::RnTexture>())
            {
                return ProcessUsdTexture(file, options, prim, outFiles);
            }
            else if (prim.IsA<pxr::RnMaterialShader>())
            {
                return ProcessUsdMaterialShader(file, options, prim, outFiles);
            }
        }
        return 0;
    }
}