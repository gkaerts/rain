#pragma once

#include "common/memory/vector.hpp"

#include "build.hpp"
#include "toml++/toml.hpp"
#include <string_view>

namespace rn
{
    using FnBuildAsset = int(*)(const DataBuildContext& ctxt, toml::parse_result& root, Vector<std::string>& outFiles);
    using FnIsValidValue = bool(*)(const DataBuildContext& ctxt, toml::node& node);

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

    std::ostream& BuildError(const DataBuildContext& ctxt, const toml::source_region& source);
    std::ostream& BuildWarning(const DataBuildContext& ctxt, const toml::source_region& source);

    bool    ValidateTable(const DataBuildContext& ctxt, toml::node& node, const TableSchema& schema);
    bool    ValidateArray(const DataBuildContext& ctxt, std::string_view name, toml::array* arr, toml::node_type elementType);
    int     DoBuildTOML(const DataBuildContext& ctxt, Vector<std::string>& outFiles);
}