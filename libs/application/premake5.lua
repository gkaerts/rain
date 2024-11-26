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

    RN_APPLICATION_INCLUDES = {
        "%{wks.location}/../../libs/application/include",
    }

    includedirs(RN_COMMON_INCLUDES)
    includedirs(RN_RHI_INCLUDES)
    includedirs(RN_RHI_D3D12_INCLUDES)
    includedirs(RN_APPLICATION_INCLUDES)

    libdirs {
        "%{wks.location}/%{cfg.buildcfg}"
    }

    if BUILD_PROPERTIES.RequiresSDL then
        includedirs(RN_SDL_INCLUDES)

        dependson { "SDL" }
        links { "SDL2", "SDL2main" }

    end
    
    targetdir "%{wks.location}/%{cfg.buildcfg}/"

    links { "rnCommon", "rnRHI", "rnRHID3D12" }

if BUILD_PROPERTIES.IncludeTestsInBuild then
    include "test"
end