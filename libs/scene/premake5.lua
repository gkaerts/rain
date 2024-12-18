project "rnScene"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"
    flags { "FatalWarnings", "MultiProcessorCompile" }

    codegenfiles {
        "schema/**.lua"
    }

    codegentargets {
        "cpp"
    }

    codegenimportdirs {
        "schema",
        "../asset/schema",
        "../data/schema"
    }

    files { 
        "src/**.h", 
        "src/**.hpp", 
        "src/**.c", 
        "src/**.cpp", 
        "include/**.h",
        "include/**.hpp"
    }

    RN_SCENE_INCLUDES = {
        "%{wks.location}/../../libs/scene/include",
    }

    includedirs(RN_COMMON_INCLUDES)
    includedirs(RN_ASSET_INCLUDES)
    includedirs(RN_DATA_INCLUDES)
    includedirs(RN_SCENE_INCLUDES)

    
    libdirs {
        "%{wks.location}/%{cfg.buildcfg}"
    }

    targetdir "%{wks.location}/%{cfg.buildcfg}/"
    
    dependson { "data_build" }
    links { "rnCommon", "rnAsset", "rnData" }

if BUILD_PROPERTIES.IncludeTestsInBuild then
    include "test"
end