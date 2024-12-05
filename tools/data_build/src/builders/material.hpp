#pragma once

#pragma warning(push, 1)
#include <pxr/usd/usd/prim.h>
#pragma warning(pop)

#include "common/memory/vector.hpp"
#include "toml++/toml.hpp"
#include <string_view>

namespace rn
{
    struct DataBuildOptions;
    int ProcessUsdMaterial(std::string_view file, const DataBuildOptions& options, const pxr::UsdPrim& prim, Vector<std::string>& outFiles);
}