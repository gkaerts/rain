if BUILD_PROPERTIES.RequiresSDL then
    include "../../contrib/projects/sdl2"
end

project "rnApplication"
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
    
    includedirs {
        "include",
        "../../libs/common/include",
        "../../libs/rhi/include",
        "../../libs/rhi_d3d12/include",
        "../../contrib/submodules/googletest/googletest/include",
        "../../contrib/submodules/spdlog/include",
        "../../contrib/submodules/unordered_dense/include",
        "../../contrib/submodules/enkiTS/src",
        "../../contrib/submodules/hlslpp/include",
    }

    libdirs {
        "%{wks.location}/%{cfg.buildcfg}"
    }

    if BUILD_PROPERTIES.RequiresSDL then
        includedirs {
            "../../contrib/third_party/sdl-2.30.9/include/SDL2",
        }

        dependson { "SDL2" }

        filter "configurations:Debug"
            links { "SDL2d", "SDL2maind"}

        filter "configurations:Release"
            links { "SDL2", "SDL2main" }
    end
    
    targetdir "%{wks.location}/%{cfg.buildcfg}/"

    links { "rnCommon", "rnRHI", "rnRHID3D12" }

if BUILD_PROPERTIES.IncludeTestsInBuild then
    include "test"
end