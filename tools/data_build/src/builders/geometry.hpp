#pragma once

#include "build.hpp"

#pragma warning(push, 1)
#include <pxr/usd/usdGeom/mesh.h>
#pragma warning(pop)

namespace rn
{
    using FnPermuteVec3 = pxr::GfVec3f(*)(const pxr::GfVec3f&);

    struct UsdGeometryBuildDesc
    {
        pxr::UsdGeomMesh mesh;
        FnPermuteVec3 fnPermute;
        float unitScale;
    };

    bool ProcessUsdGeomMesh(std::string_view file, const DataBuildOptions& options, const UsdGeometryBuildDesc& desc, Vector<std::string>& outFiles);
}