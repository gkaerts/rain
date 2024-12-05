include "../../contrib/projects/basis"

project "rnData"
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
    }

    files { 
        "src/**.h", 
        "src/**.hpp", 
        "src/**.c", 
        "src/**.cpp", 
        "include/**.h",
        "include/**.hpp", 
        PROJECT_ROOT .. "/build/codegen/rnData/**.hpp",
        PROJECT_ROOT .. "/build/codegen/rnData/**.cpp",
    }
    
    RN_DATA_INCLUDES = {
        "%{wks.location}/../../libs/data/include",
        PROJECT_ROOT .. "/build/codegen/rnData",
    }

    includedirs(RN_COMMON_INCLUDES)
    includedirs(RN_ASSET_INCLUDES)
    includedirs(RN_RHI_INCLUDES)
    includedirs(RN_DATA_INCLUDES)
    includedirs(RN_BASIS_INCLUDES)

    libdirs {
        "%{wks.location}/%{cfg.buildcfg}"
    }

    targetdir "%{wks.location}/%{cfg.buildcfg}/"
    
    dependson { "basisu", "basisu_encoder" }
    links { "rnCommon", "rnAsset", "rnRHI" }

if BUILD_PROPERTIES.IncludeTestsInBuild then
    include "test"
end