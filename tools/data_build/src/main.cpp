#include "common/common.hpp"
#include "common/memory/memory.hpp"
#include "tool.hpp"
#include "toml++/toml.hpp"

#include "common/memory/vector.hpp"

#include "builders/build.hpp"

namespace rn
{
    constexpr const Tool DATA_BUILD_TOOL = {
        .name = "data_build"sv,
        .additionalUsageText = 
            "FILE must be an asset build file in TOML format.\n"
            "Example: data_build -o .\\Content .\\textures\\my_texture.texture.toml\n"sv
    };

    void OnHelpOption(DataBuildOptions&, std::string_view);
    constexpr const ToolOption<DataBuildOptions> OPTIONS[] = {
        { 
            .option = "o"sv, 
            .parameter = "PATH"sv, 
            .description = "Path to write build output files to. Relative directory structures will be maintained compared to the current working directory."sv,
            .onOptionFound = [](DataBuildOptions& args, std::string_view arg){ args.outputDirectory = arg; }
        },
        {
            .option = "cache"sv,
            .parameter = "CACHE_PATH"sv,
            .description = "Path to write the dependency cache to."sv,
            .onOptionFound = [](DataBuildOptions& args, std::string_view arg){ args.cacheDirectory = arg; }
        },
        { 
            .option = "h"sv,
            .description = "Print this usage text"sv,
            .onOptionFound = OnHelpOption
        },
    };

    void OnHelpOption(DataBuildOptions&, std::string_view)
    {
        PrintUsage<DataBuildOptions>(DATA_BUILD_TOOL, OPTIONS);
    }
}

int main(int argc, char* argv[])
{
    rn::InitializeScopedAllocationForThread(16 * rn::MEGA);

    std::string_view file = ""sv;
    rn::DataBuildOptions options = 
    {
        .exeName = argv[0],
        .outputDirectory = ""sv,
        .cacheDirectory = "build/data_cache"sv
    };

    int ret = 1;
    if (ParseArgs<rn::DataBuildOptions>(rn::DATA_BUILD_TOOL, options, file, rn::OPTIONS, argc, argv))
    {
        ret = rn::DoBuild(file, options);
    }

    rn::TeardownScopedAllocationForThread();
    return ret;
}