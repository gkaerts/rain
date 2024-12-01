#pragma once

#pragma warning(push, 1)
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/property.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usd/relationship.h>
#include <pxr/usd/sdf/path.h>
#pragma warning(pop)

#include "build.hpp"

namespace rn
{
    using FnIsValidPropertyValue = bool(*)(std::string_view file, const pxr::UsdAttribute& prop);

    struct PrimProperty
    {
        std::string name;
        FnIsValidPropertyValue fnIsValidPropertyValue = nullptr;
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

    bool IsNonEmptyAssetPath(std::string_view file, const pxr::UsdAttribute& prop);
    bool IsNonEmptyString(std::string_view file, const pxr::UsdAttribute& prop);
    pxr::SdfPathVector ResolveRelationTargets(const pxr::UsdRelationship& rel);
    bool IsRelationshipNotEmpty(std::string_view file, const pxr::UsdProperty& prop);
    bool ValidatePrim(std::string_view file, const pxr::UsdPrim& prim, const PrimSchema& schema);

    int DoBuildUSD(std::string_view file, const DataBuildOptions& options, Vector<std::string>& outFiles);
}