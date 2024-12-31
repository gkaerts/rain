include "../../contrib/projects/dxc"
include "../../contrib/projects/mikktspace"
include "../../contrib/projects/meshoptimizer"
include "../../contrib/projects/usd"

project "data_build"

    kind "ConsoleApp"
    language "C++"
    cppdialect "C++20"
    flags { "FatalWarnings", "MultiProcessorCompile" }
    defines { 
        "NOMINMAX",
    }

    -- Workaround for an annoying USD include
    filter "configurations:Debug"
        defines { "TBB_USE_DEBUG=1" }
    
    filter "configurations:Release"
        defines { "TBB_USE_DEBUG=0" }
    filter{}

    files {
        "src/**.hpp",
        "src/**.cpp",
        PROJECT_ROOT .. "/build/codegen/rnRender/**.hpp",
        PROJECT_ROOT .. "/build/codegen/rnRender/**.cpp",
    }

    includedirs(RN_COMMON_INCLUDES)
    includedirs(RN_RHI_INCLUDES)
    includedirs(RN_ASSET_INCLUDES)
    includedirs(RN_DATA_INCLUDES)
    includedirs(RN_RENDER_INCLUDES)
    includedirs(RN_SCENE_INCLUDES)
    includedirs(RN_BASIS_INCLUDES)
    includedirs(RN_DXC_INCLUDES)
    includedirs(RN_MESHOPTIMIZER_INCLUDES)
    includedirs(RN_MIKKTSPACE_INCLUDES)


    includedirs(OPENUSD_INCLUDE_DIRS)
    libdirs(OPENUSD_LIB_DIRS)
    links { "usd_ms" }

    includedirs{
        "src",
        "%{wks.location}/../../contrib/submodules/tomlplusplus/include",
        GENERATED_FILE_PATH .. "/usd_plugins/"
    }

    libdirs {
        "%{wks.location}/%{cfg.buildcfg}"
    }

    targetdir "%{wks.location}/%{cfg.buildcfg}/"

    dependson { "dxc", "usd" }
    links { "rnCommon", "rnAsset", "rnData", "rnScene", "dxcompiler", "meshoptimizer", "mikktspace", "usd_rn" }

if BUILD_PROPERTIES.IncludeTestsInBuild then
    include "test"
end