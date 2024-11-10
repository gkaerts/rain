project "enkiTS"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"

    flags {
        "MultiProcessorCompile"
    }

    local project_dir = "../submodules/enkiTS"
    local source_dir = project_dir .. "/src"
    files {
        source_dir .. "/**.cpp",
        source_dir .. "/**.h"
    }
    
    includedirs {
        source_dir
    }
    
    targetdir "%{wks.location}/%{cfg.buildcfg}/"