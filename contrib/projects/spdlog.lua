project "spdlog"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"

    flags {
        "MultiProcessorCompile"
    }

    local project_dir = "../submodules/spdlog"
    local source_dir = project_dir .. "/src"
    local include_dir = project_dir .. "/include"
    files {
        source_dir .. "/**.cpp",
        source_dir .. "/**.h",
        include_dir .. "/**.h"
    }
    
    includedirs {
        include_dir
    }

    defines { "SPDLOG_COMPILED_LIB" }
    
    targetdir "%{wks.location}/%{cfg.buildcfg}/"