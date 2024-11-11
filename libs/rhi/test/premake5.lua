project "rhiTests"

    kind "ConsoleApp"
    language "C++"
    cppdialect "C++20"
    flags { "FatalWarnings", "MultiProcessorCompile" }

    files {
        "src/**.hpp",
        "src/**.cpp", 
        "src/shaders/**.hlsl",
        "../../../libs/test/src/test_main.cpp",
    }

    includedirs {
        "../../../libs/common/include",
        "../../../libs/rhi/include",
        "../../../contrib/submodules/googletest/googletest/include",
        "../../../contrib/submodules/spdlog/include",
        "../../../contrib/submodules/unordered_dense/include",
        "../../../contrib/submodules/enkiTS/src",
        "../../../contrib/submodules/hlslpp/include",

    }

    libdirs {
        "%{wks.location}/%{cfg.buildcfg}"
    }

    targetdir "%{wks.location}/%{cfg.buildcfg}/"

    links { "googletest", "rnCommon", "rnRHI", "rnRHID3D12", "dxgi" }