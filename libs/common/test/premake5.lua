project "commonTests"

    kind "ConsoleApp"
    language "C++"
    cppdialect "C++20"
    flags { "FatalWarnings", "MultiProcessorCompile" }

    files {
        "src/**.hpp",
        "src/**.cpp"
    }

    includedirs {
        "../../../libs/common/include",
        "../../../contrib/submodules/googletest/googletest/include",
        "../../../contrib/submodules/spdlog/include",
        "../../../contrib/submodules/unordered_dense/include",
        "../../../contrib/submodules/enkiTS/src",
    }

    libdirs {
        "%{wks.location}/%{cfg.buildcfg}"
    }

    targetdir "%{wks.location}/%{cfg.buildcfg}/"

    links { "googletest", "rnCommon"}