#pragma once

#include "common/common.hpp"
#include "common/memory/span.hpp"
#include "common/memory/vector.hpp"

#include <iostream>
#include <string_view>

#include "toml++/toml.hpp"

using namespace std::literals;
namespace rn
{
    struct DataBuildOptions
    {
        std::string_view exeName;
        std::string_view outputDirectory;
        std::string_view cacheDirectory;
    };

    using FnBuildAsset = int(*)(std::string_view file, toml::parse_result& root, const DataBuildOptions& options, Vector<std::string>& outFiles);
    using FnIsValidValue = bool(*)(std::string_view file, toml::node& node);

    std::ostream& BuildMessage(std::string_view file);
    std::ostream& BuildError(std::string_view file);
    std::ostream& BuildError(std::string_view file, const toml::source_region& source);

    std::ostream& BuildWarning(std::string_view file);
    std::ostream& BuildWarning(std::string_view file, const toml::source_region& source);

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

    bool ValidateTable(std::string_view file, toml::node& node, const TableSchema& schema);
    bool ValidateArray(std::string_view file, std::string_view name, toml::array* arr, toml::node_type elementType);

    void WriteDependenciesFile(std::string_view file, const DataBuildOptions& options, Span<const std::string> dependencies);
    int WriteAssetToDisk(std::string_view file, std::string_view extension, const DataBuildOptions& options, Span<const uint8_t> assetdata, Span<std::string_view> references, Vector<std::string>& outFiles);
    int WriteDataToDisk(std::string_view file, std::string_view extension, const DataBuildOptions& options, Span<const uint8_t> data, bool writeText, Vector<std::string>& outFiles);
    int DoBuild(std::string_view file, const DataBuildOptions& options);

}