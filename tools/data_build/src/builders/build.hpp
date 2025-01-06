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

    struct DataBuildContext
    {
        std::string_view file;
        DataBuildOptions options;
    };

    std::filesystem::path MakeRelativeTo(std::string_view buildFile, std::string_view otherFile);
    std::filesystem::path MakeRelativeTo(std::string_view buildFile, std::wstring_view otherFile);
    std::filesystem::path MakeRootRelativeReferencePath(const DataBuildContext& ctxt, std::string_view refPath);

    std::ostream& BuildMessage(const DataBuildContext& ctxt);
    std::ostream& BuildError(const DataBuildContext& ctxt);
    std::ostream& BuildWarning(const DataBuildContext& ctxt);
    
    void    WriteDependenciesFile(const DataBuildContext& ctxt, Span<const std::string> dependencies);
    int     WriteAssetToDisk(const DataBuildContext& ctxt, std::string_view extension, Span<uint8_t> assetdata, Span<std::string_view> references, Vector<std::string>& outFiles);
    int     WriteDataToDisk(const DataBuildContext& ctxt, std::string_view extension, Span<uint8_t> data, bool writeText, Vector<std::string>& outFiles);
    int     DoBuild(const DataBuildContext& ctxt);
}