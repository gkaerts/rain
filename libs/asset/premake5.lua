include "../../contrib/projects/flatbuffers"


compile_schema_files = function(generated_file_path, include_dirs)
    local FLATC_CPP_ARGS = "--scoped-enums --cpp-std=C++17 --bfbs-gen-embed --no-includes"

    local COMMAND_CPP = "%{wks.location}/%{cfg.buildcfg}/flatc.exe --cpp " .. FLATC_CPP_ARGS
    COMMAND_CPP = COMMAND_CPP .. " -o " .. generated_file_path .. "/schema"
    for _, dir in ipairs(include_dirs) do
        COMMAND_CPP = COMMAND_CPP .. " -I " .. dir
    end
    COMMAND_CPP = COMMAND_CPP .. " %{file.relpath}"

    filter "files:**.fbs"
        buildmessage "Compiling schema: %{file.relpath}"
        buildcommands {
            COMMAND_CPP
        }

        buildoutputs {
            generated_file_path .. "/schema/%{file.basename}_generated.h"
        }
    filter{}
end

project "rnAsset"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"
    flags { "FatalWarnings", "MultiProcessorCompile" }

    files { 
        "src/**.h", 
        "src/**.hpp", 
        "src/**.c", 
        "src/**.cpp", 
        "include/**.h",
        "include/**.hpp", 
        "schema/**.fbs",
    }
    
    RN_ASSET_INCLUDES = {
        "%{wks.location}/../../contrib/submodules/mio/single_include",
        "%{wks.location}/../../libs/asset/include",
        RN_FLATBUFFERS_INCLUDES,
        GENERATED_FILE_PATH
    }

    RN_ASSET_SCHEMA_INCLUDES = {
        "%{wks.location}/../../libs/"
    }

    compile_schema_files(GENERATED_FILE_PATH .. "/asset", RN_ASSET_SCHEMA_INCLUDES)

    includedirs(RN_COMMON_INCLUDES)
    includedirs(RN_ASSET_INCLUDES)

    libdirs {
        "%{wks.location}/%{cfg.buildcfg}"
    }

    targetdir "%{wks.location}/%{cfg.buildcfg}/"
    
    dependson { "flatc" }
    links { "rnCommon", "flatbuffers" }

if BUILD_PROPERTIES.IncludeTestsInBuild then
    include "test"
end