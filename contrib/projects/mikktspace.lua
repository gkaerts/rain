project "mikktspace"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"

    flags {
        "MultiProcessorCompile"
    }

    local project_dir = "../submodules/MikkTSpace"
    local source_dir = project_dir
    local include_dir = project_dir

    RN_MIKKTSPACE_INCLUDES = {
        "%{wks.location}/../../contrib/submodules/MikkTSpace"
    }

    files {
        source_dir .. "/*.c",
        include_dir .. "/**.h"
    }
    
    includedirs {
        include_dir,
    }

    libdirs {
        "%{wks.location}/%{cfg.buildcfg}"
    }


    targetdir "%{wks.location}/%{cfg.buildcfg}/"