#pragma once

#include "common/memory/vector.hpp"

#include "build.hpp"
#include "toml++/toml.hpp"
#include <string_view>

namespace rn
{
    using FnBuildAsset = int(*)(std::string_view file, toml::parse_result& root, const DataBuildOptions& options, Vector<std::string>& outFiles);
    using FnIsValidValue = bool(*)(std::string_view file, toml::node& node);

    struct Field
    {
        std::string_view name;
        toml::node_type type;
        FnIsValidValue fnIsValidValue = nullptr;
    };

    struct TableSchema
    {
        std::string_view name;
        std::initializer_list<Field> requiredFields;
        std::initializer_list<Field> optionalFields;
    };

    std::ostream& BuildError(std::string_view file, const toml::source_region& source);
    std::ostream& BuildWarning(std::string_view file, const toml::source_region& source);

    bool ValidateTable(std::string_view file, toml::node& node, const TableSchema& schema);
    bool ValidateArray(std::string_view file, std::string_view name, toml::array* arr, toml::node_type elementType);
    int DoBuildTOML(std::string_view file, const DataBuildOptions& options, Vector<std::string>& outFiles);
}