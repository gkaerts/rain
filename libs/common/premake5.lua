project "rnCommon"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"

    flags {
        "FatalWarnings",
        "MultiProcessorCompile"
    }

    files {
        "src/**.h",
        "src/**.hpp",
        "src/**.c",
        "src/**.cpp",
        "include/**.h",
        "include/**.hpp"
    }

    RN_COMMON_INCLUDES = {
        "%{wks.location}/../../libs/common/include",
        "%{wks.location}/../../contrib/submodules/spdlog/include",
        "%{wks.location}/../../contrib/submodules/unordered_dense/include",
        "%{wks.location}/../../contrib/submodules/enkiTS/src",
        "%{wks.location}/../../contrib/submodules/hlslpp/include",
    }
    
    includedirs(RN_COMMON_INCLUDES)

    libdirs {
        "%{wks.location}/%{cfg.buildcfg}"
    }
    
    targetdir "%{wks.location}/%{cfg.buildcfg}/"

    links { "spdlog", "enkiTS" }

if BUILD_PROPERTIES.IncludeTestsInBuild then
    include "test"
end