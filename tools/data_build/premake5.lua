include "../../contrib/projects/dxc"
project "data_build"

    kind "ConsoleApp"
    language "C++"
    cppdialect "C++20"
    flags { "FatalWarnings", "MultiProcessorCompile" }

    files {
        "src/**.hpp",
        "src/**.cpp"
    }

    includedirs(RN_COMMON_INCLUDES)
    includedirs(RN_ASSET_INCLUDES)
    includedirs(RN_DATA_INCLUDES)
    includedirs(RN_BASIS_INCLUDES)
    includedirs(RN_DXC_INCLUDES)

    includedirs{
        "%{wks.location}/../../contrib/submodules/tomlplusplus/include"
    }

    libdirs {
        "%{wks.location}/%{cfg.buildcfg}"
    }

    targetdir "%{wks.location}/%{cfg.buildcfg}/"

    dependson { "dxc" }
    links { "rnCommon", "rnAsset", "rnData", "dxcompiler" }