

project "rhiD3D12Tests"

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

    includedirs(RN_COMMON_INCLUDES)
    includedirs(RN_RHI_INCLUDES)
    includedirs(RN_RHI_D3D12_INCLUDES)
    includedirs(RN_GTEST_INCLUDES)

    includedirs {
        GENERATED_FILE_PATH .. "/rhiD3D12Tests"
    }

    local SHADER_DEST_PATH =  GENERATED_FILE_PATH .. "/rhiD3D12Tests/shaders"
    filter "files:**.hlsl"
        buildmessage "Compiling %{file.relpath}"
        buildcommands {
            "%{wks.location}%{cfg.buildcfg}\\dxc.exe -E vs_main -T vs_6_6 -Fh " .. SHADER_DEST_PATH .. "/%{file.basename}_vs.hpp -Vn %{file.basename}_vs %{file.relpath}",
            "%{wks.location}%{cfg.buildcfg}\\dxc.exe -E ps_main -T ps_6_6 -Fh " .. SHADER_DEST_PATH .. "/%{file.basename}_ps.hpp -Vn %{file.basename}_ps %{file.relpath}",
            "%{wks.location}%{cfg.buildcfg}\\dxc.exe -E cs_main -T cs_6_6 -Fh " .. SHADER_DEST_PATH .. "/%{file.basename}_cs.hpp -Vn %{file.basename}_cs %{file.relpath}"
        }

        buildoutputs {
            SHADER_DEST_PATH .. "/%{file.basename}_vs.hpp",
            SHADER_DEST_PATH .. "/%{file.basename}_ps.hpp",
            SHADER_DEST_PATH .. "/%{file.basename}_cs.hpp"
        }

    filter {}

    libdirs {
        "%{wks.location}/%{cfg.buildcfg}"
    }

    targetdir "%{wks.location}/%{cfg.buildcfg}/"

    links { "googletest", "rnCommon", "rnRHI", "rnRHID3D12", "dxgi", "WinPixEventRuntime" }