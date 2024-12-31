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
        "include/**.hpp",
        PROJECT_ROOT .. "/build/codegen/rnScene/**.hpp",
        PROJECT_ROOT .. "/build/codegen/rnScene/**.cpp",
    }

    RN_SCENE_INCLUDES = {
        PROJECT_ROOT .. "/libs/scene/include",
        PROJECT_ROOT .. "/build/codegen/rnScene",
    }

    includedirs(RN_COMMON_INCLUDES)
    includedirs(RN_ASSET_INCLUDES)
    includedirs(RN_DATA_INCLUDES)
    includedirs(RN_SCENE_INCLUDES)

    
    libdirs {
        "%{wks.location}/%{cfg.buildcfg}"
    }

    targetdir "%{wks.location}/%{cfg.buildcfg}/"
    
    links { "rnCommon", "rnAsset", "rnData" }

if BUILD_PROPERTIES.IncludeTestsInBuild then
    include "test"
end