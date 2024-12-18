project "renderTests"

    kind "ConsoleApp"
    language "C++"
    cppdialect "C++20"
    flags { "FatalWarnings", "MultiProcessorCompile" }

    files {
        "src/**.hpp",
        "src/**.cpp",
        "../../../libs/test/src/test_main.cpp",
        "shaders/**.toml"
    }

    filter "files:**.toml"
        buildmessage ""
        buildcommands {
            "%{wks.location}%{cfg.buildcfg}\\data_build.exe -o " .. GENERATED_FILE_PATH .. "/renderTests -cache " .. GENERATED_FILE_PATH .. "/renderTests/cache -root ./../../libs/render/test/shaders %{file.relpath}"
        }

        buildoutputs {
            "FORCE_ASSET_REBUILD_IGNORE_THIS_WARNING"
        }
    filter {}

    includedirs(RN_COMMON_INCLUDES)
    includedirs(RN_RHI_INCLUDES)
    includedirs(RN_RHI_D3D12_INCLUDES)
    includedirs(RN_ASSET_INCLUDES)
    includedirs(RN_DATA_INCLUDES)
    includedirs(RN_RENDER_GRAPH_INCLUDES)
    includedirs(RN_RENDER_INCLUDES)
    includedirs(RN_GTEST_INCLUDES)

    includedirs {
        "shaders",
        GENERATED_FILE_PATH .. "/renderTests"
    }

    libdirs {
        "%{wks.location}/%{cfg.buildcfg}"
    }

    targetdir "%{wks.location}/%{cfg.buildcfg}/"

    links { "googletest", "rnCommon", "rnRHI", "rnRHID3D12", "rnAsset", "rnData", "rnRender", "rnRenderGraph", "dxgi", "WinPixEventRuntime"}