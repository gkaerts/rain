include "../../contrib/projects/basis"

project "rnData"
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
    
    RN_DATA_INCLUDES = {
        "%{wks.location}/../../libs/data/include",
        GENERATED_FILE_PATH
    }

    DATA_SCHEMA_PATH = "%{wks.location}/../../libs/data/schema"
    RN_DATA_SCHEMA_INCLUDES = {
        ASSET_SCHEMA_PATH,
        DATA_SCHEMA_PATH,
    }

    compile_schema_files(GENERATED_FILE_PATH .. "/data", RN_DATA_SCHEMA_INCLUDES)

    includedirs(RN_COMMON_INCLUDES)
    includedirs(RN_ASSET_INCLUDES)
    includedirs(RN_RHI_INCLUDES)
    includedirs(RN_DATA_INCLUDES)
    includedirs(RN_BASIS_INCLUDES)

    libdirs {
        "%{wks.location}/%{cfg.buildcfg}"
    }

    targetdir "%{wks.location}/%{cfg.buildcfg}/"
    
    dependson { "flatc", "basisu", "basisu_encoder" }
    links { "rnCommon", "rnAsset", "rnRHI" }

if BUILD_PROPERTIES.IncludeTestsInBuild then
    include "test"
end