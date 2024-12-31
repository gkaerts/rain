#pragma once

#pragma warning(push, 1)
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usd/property.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usd/relationship.h>
#include <pxr/usd/sdf/path.h>
#pragma warning(pop)

#include "build.hpp"
#include <filesystem>

namespace rn
{
    using FnIsValidPropertyValue = bool(*)(std::string_view file, const pxr::UsdAttribute& prop);
    using FnIsValidRelationship = bool(*)(std::string_view file, const pxr::UsdStage& stage, const pxr::UsdRelationship& rel);

    struct PrimProperty
    {
        std::string name;
        FnIsValidPropertyValue fnIsValidPropertyValue = nullptr;
        FnIsValidRelationship fnIsValidRelationship = nullptr;
    };

    struct PrimSchema
    {
        std::initializer_list<PrimProperty> requiredProperties;
        std::initializer_list<PrimProperty> optionalProperties;
    };

    template <typename T> T Value(const pxr::UsdAttribute& attr)
    {
        T outValue;
        attr.Get(&outValue);
        return outValue;
    }

    std::filesystem::path MakeAssetReferencePath(std::string_view file, const pxr::SdfAssetPath& path, const std::filesystem::path& extension);
    bool IsNonEmptyAssetPath(std::string_view file, const pxr::UsdAttribute& prop);
    bool IsNonEmptyString(std::string_view file, const pxr::UsdAttribute& prop);
    pxr::SdfPathVector ResolveRelationTargets(const pxr::UsdRelationship& rel);
    bool IsRelationshipNotEmpty(std::string_view file, const pxr::UsdProperty& prop);
    bool ValidatePrim(std::string_view file, const pxr::UsdPrim& prim, const PrimSchema& schema);

    template <typename... T>
    bool RelatedPrimIsA(std::string_view file, const pxr::UsdStage& stage, const pxr::UsdRelationship& rel)
    {
        pxr::SdfPathVector paths = ResolveRelationTargets(rel);
        for (const pxr::SdfPath& path : paths)
        {
            pxr::UsdPrim prim = stage.GetPrimAtPath(path);
            bool hasAPI = (... || prim.IsA<T>());
            if (!hasAPI)
            {
                return false;
            }
        }

        return true;
    }

    template <typename... T>
    bool RelatedPrimHasAPI(std::string_view file, const pxr::UsdStage& stage, const pxr::UsdRelationship& rel)
    {
        pxr::SdfPathVector paths = ResolveRelationTargets(rel);
        for (const pxr::SdfPath& path : paths)
        {
            pxr::UsdPrim prim = stage.GetPrimAtPath(path);
            bool hasAPI = (... || prim.HasAPI<T>());
            if (!hasAPI)
            {
                return false;
            }
        }

        return true;
    }

    int DoBuildUSD(std::string_view file, const DataBuildOptions& options, Vector<std::string>& outFiles);
}