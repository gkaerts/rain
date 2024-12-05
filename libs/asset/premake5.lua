project "rnAsset"
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
    }

    files { 
        "src/**.h", 
        "src/**.hpp", 
        "src/**.c", 
        "src/**.cpp", 
        "include/**.h",
        "include/**.hpp",
        PROJECT_ROOT .. "/build/codegen/rnAsset/**.hpp",
        PROJECT_ROOT .. "/build/codegen/rnAsset/**.cpp",
    }
    
    RN_ASSET_INCLUDES = {
        "%{wks.location}/../../contrib/submodules/mio/single_include",
        "%{wks.location}/../../libs/asset/include",
        PROJECT_ROOT .. "/build/codegen/rnAsset",
        PROJECT_ROOT .. "/tools/luagen/include"
    }

    includedirs(RN_COMMON_INCLUDES)
    includedirs(RN_ASSET_INCLUDES)

    libdirs {
        "%{wks.location}/%{cfg.buildcfg}"
    }

    targetdir "%{wks.location}/%{cfg.buildcfg}/"
    
    links { "rnCommon" }

if BUILD_PROPERTIES.IncludeTestsInBuild then
    include "test"
end