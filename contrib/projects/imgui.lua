project "imgui"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"

    flags {
        "MultiProcessorCompile"
    }

    RN_IMGUI_INCLUDES = PROJECT_ROOT .. "/contrib/submodules/imgui"

    local project_dir = "../submodules/imgui"
    local source_dir = project_dir
    files {
        source_dir .. "/*.cpp",
        source_dir .. "/*.h"
    }
    
    includedirs {
        source_dir
    }
    
    targetdir "%{wks.location}/%{cfg.buildcfg}/"