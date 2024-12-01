#pragma once

#include "common/common.hpp"
#include "common/memory/span.hpp"
#include "common/memory/vector.hpp"

#include <iostream>
#include <string_view>
#include <filesystem>

using namespace std::literals;
namespace rn
{
    struct DataBuildOptions
    {
        std::string_view exeName;
        std::string_view outputDirectory;
        std::string_view cacheDirectory;
        std::string_view assetRootDirectory;
        bool force;
    };

    std::ostream& BuildMessage(std::string_view file);
    std::ostream& BuildError(std::string_view file);
    std::ostream& BuildWarning(std::string_view file);
    

    std::filesystem::path MakeRelativeTo(std::string_view buildFile, std::string_view otherFile);
    std::filesystem::path MakeRelativeTo(std::string_view buildFile, std::wstring_view otherFile);

    void WriteDependenciesFile(std::string_view file, const DataBuildOptions& options, Span<const std::string> dependencies);
    int WriteAssetToDisk(std::string_view file, std::string_view extension, const DataBuildOptions& options, Span<const uint8_t> assetdata, Span<std::string_view> references, Vector<std::string>& outFiles);
    int WriteDataToDisk(std::string_view file, std::string_view extension, const DataBuildOptions& options, Span<const uint8_t> data, bool writeText, Vector<std::string>& outFiles);
    int DoBuild(std::string_view file, const DataBuildOptions& options);
}