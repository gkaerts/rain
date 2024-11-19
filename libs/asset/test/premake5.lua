project "assetTests"

    kind "ConsoleApp"
    language "C++"
    cppdialect "C++20"
    flags { "FatalWarnings", "MultiProcessorCompile" }

    files {
        "src/**.hpp",
        "src/**.cpp",
        "../../../libs/test/src/test_main.cpp",
    }

    includedirs(RN_COMMON_INCLUDES)
    includedirs(RN_ASSET_INCLUDES)
    includedirs(RN_GTEST_INCLUDES)

    libdirs {
        "%{wks.location}/%{cfg.buildcfg}"
    }

    targetdir "%{wks.location}/%{cfg.buildcfg}/"

    links { "googletest", "rnCommon", "rnAsset"}