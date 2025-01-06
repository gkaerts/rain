#pragma once

#pragma warning(push, 1)
#include <pxr/usd/usd/prim.h>
#pragma warning(pop)

#include "toml++/toml.hpp"
#include <string_view>

namespace rn
{
    struct DataBuildOptions;
    int ProcessUsdTexture(const DataBuildContext& ctxt, const pxr::UsdPrim& prim, Vector<std::string>& outFiles);
}