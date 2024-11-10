project "rnRHI"
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
    }

    includedirs { 
        "include",
        "../common/include",
        "../../contrib/submodules/spdlog/include",
        "../../contrib/submodules/unordered_dense/include",
        "../../contrib/submodules/enkiTS/src",
        "../../contrib/submodules/hlslpp/include",
    }

    libdirs {
        "%{wks.location}/%{cfg.buildcfg}"
    }

    targetdir "%{wks.location}/%{cfg.buildcfg}/"
    
    links { "rnCommon" }

if BUILD_PROPERTIES.IncludeTestsInBuild then
    include "test"
end