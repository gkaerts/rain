project "enkiTS"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"

    flags {
        "MultiProcessorCompile"
    }
    
    files {
        "src/**.cpp",
        "src/**.h"
    }
    
    includedirs {
        "src"
    }
    
    targetdir "%{wks.location}/%{cfg.buildcfg}/"