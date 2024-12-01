project "meshoptimizer"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"

    flags {
        "MultiProcessorCompile"
    }

    local project_dir = "../submodules/meshoptimizer"
    local source_dir = project_dir .. "/src"
    local include_dir = project_dir .. "/src"

    RN_MESHOPTIMIZER_INCLUDES = {
        "%{wks.location}/../../contrib/submodules/meshoptimizer/src"
    }

    files {
        source_dir .. "/*.cpp",
        include_dir .. "/**.h"
    }
    
    includedirs {
        include_dir,
    }

    libdirs {
        "%{wks.location}/%{cfg.buildcfg}"
    }


    targetdir "%{wks.location}/%{cfg.buildcfg}/"