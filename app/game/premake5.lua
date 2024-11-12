project "game"

    kind "ConsoleApp"
    language "C++"
    cppdialect "C++20"
    flags { "FatalWarnings", "MultiProcessorCompile" }

    files {
        "src/**.hpp",
        "src/**.cpp"
    }

    includedirs {
        "include",
        "../../libs/common/include",
        "../../libs/rhi/include",
        "../../libs/rhi_d3d12/include",
        "../../libs/application/include",
        "../../contrib/submodules/googletest/googletest/include",
        "../../contrib/submodules/spdlog/include",
        "../../contrib/submodules/unordered_dense/include",
        "../../contrib/submodules/enkiTS/src",
        "../../contrib/submodules/hlslpp/include",
    }

    libdirs {
        "%{wks.location}/%{cfg.buildcfg}"
    }

    targetdir "%{wks.location}/%{cfg.buildcfg}/"

    links { "rnCommon", "rnRHI", "rnRHID3D12", "rnApplication", "dxgi" }