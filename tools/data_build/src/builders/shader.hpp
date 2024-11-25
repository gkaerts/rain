#pragma once

#include "toml++/toml.hpp"
#include <string_view>

namespace rn
{
    struct DataBuildOptions;
    int BuildShader(std::string_view file, toml::parse_result& root, const DataBuildOptions& options, Vector<std::string>& outFiles);
}