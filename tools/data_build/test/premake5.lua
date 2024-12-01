

project "data_build_tests"

    kind "ConsoleApp"
    language "C++"
    cppdialect "C++20"
    flags { "FatalWarnings", "MultiProcessorCompile" }

    files {
        "src/**.hpp",
        "src/**.cpp", 
        "../../../libs/test/src/test_main.cpp",
    }

    -- Test assets
    files {
        "assets/**.usd",
        "assets/**.usdc",
        "assets/**.usda",
    }

    includedirs(RN_COMMON_INCLUDES)
    includedirs(RN_ASSET_INCLUDES)
    includedirs(RN_DATA_INCLUDES)
    includedirs(RN_RHI_INCLUDES)
    includedirs(RN_RHI_D3D12_INCLUDES)
    includedirs(RN_GTEST_INCLUDES)

    -- Symlink USD directories so we can run binaries relying on USD from the build folder. I'd prefer to move this somewhere else at some point
    prebuildcommands { 
        "IF not exist \"%{wks.location}%{cfg.buildcfg}\\usd\" (mklink /d \"%{wks.location}%{cfg.buildcfg}\\usd\" \"" .. OPENUSD_BUILD_PATH .. "/%{cfg.buildcfg}/lib/usd\")",
        "IF not exist \"%{wks.location}%{cfg.buildcfg}\\python\" (mklink /d \"%{wks.location}%{cfg.buildcfg}\\python\" \"" .. OPENUSD_BUILD_PATH .. "/%{cfg.buildcfg}/lib/python\")" 
    }

    filter "files:**.usda or **.usdc or **.usd"
        buildmessage ""
        buildcommands {
            "%{wks.location}%{cfg.buildcfg}\\data_build.exe -o " .. GENERATED_FILE_PATH .. "/data_build_tests -cache " .. GENERATED_FILE_PATH .. "/data_build_tests/cache -root ./../../tools/data_build/test/assets %{file.relpath}"
        }

        buildoutputs {
            "FORCE_ASSET_REBUILD_IGNORE_THIS_WARNING"
        }

    filter {}

    libdirs {
        "%{wks.location}/%{cfg.buildcfg}"
    }

    targetdir "%{wks.location}/%{cfg.buildcfg}/"

    dependson { "data_build" }
    links { "googletest", "rnCommon", "rnAsset", "rnData", "rnRHI", "rnRHID3D12", "dxgi", "WinPixEventRuntime" }