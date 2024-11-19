include "../../contrib/projects/flatbuffers"

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
   
    local GENERATED_FILE_PATH = "%{wks.location}/../../generated/%{cfg.buildcfg}/rnAsset"    
    RN_ASSET_INCLUDES = {
        "%{wks.location}/../../contrib/submodules/mio/single_include",
        "%{wks.location}/../../libs/asset/include",
        RN_FLATBUFFERS_INCLUDES,
        GENERATED_FILE_PATH
    }

    local ASSET_SCHEMA_PATH = "%{wks.location}/../../libs/asset/schema"
    RN_ASSET_SCHEMA_INCLUDES = {
        ASSET_SCHEMA_PATH,
    }

    filter "files:**.fbs"
        buildmessage "Compiling schema: %{file.relpath}"
        buildcommands {
            "%{wks.location}/%{cfg.buildcfg}/flatc.exe --cpp -o " .. GENERATED_FILE_PATH .. "/schema -I " .. ASSET_SCHEMA_PATH .. " %{file.relpath}"
        }

        buildoutputs {
            GENERATED_FILE_PATH .. "/schema/%{file.basename}_generated.h"
        }
    filter{}


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