project "rnRenderGraph"
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

    RN_RENDER_GRAPH_INCLUDES = {
        "%{wks.location}/../../libs/render_graph/include"
    }

    includedirs(RN_COMMON_INCLUDES)
    includedirs(RN_RHI_INCLUDES)
    includedirs(RN_RENDER_GRAPH_INCLUDES)

    libdirs {
        "%{wks.location}/%{cfg.buildcfg}"
    }

    targetdir "%{wks.location}/%{cfg.buildcfg}/"
    
    links { "rnCommon", "rnRHI" }

if BUILD_PROPERTIES.IncludeTestsInBuild then
    include "test"
end