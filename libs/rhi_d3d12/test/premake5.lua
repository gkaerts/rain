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

    local GENERATED_FILE_PATH = "%{prj.location}/generated/%{cfg.buildcfg}/%{prj.name}"
    includedirs {
        "../../../libs/common/include",
        "../../../libs/rhi/include",
        "../../../libs/rhi_d3d12/include",
        "../../../contrib/submodules/googletest/googletest/include",
        "../../../contrib/submodules/spdlog/include",
        "../../../contrib/submodules/unordered_dense/include",
        "../../../contrib/submodules/enkiTS/src",
        "../../../contrib/submodules/hlslpp/include",
        GENERATED_FILE_PATH
    }

    local SHADER_DEST_PATH =  GENERATED_FILE_PATH .. "/shaders"
    filter "files:**.hlsl"
        buildmessage "Compiling %{file.relpath}"
        buildcommands {
            "..\\..\\contrib\\bin\\dxc\\dxc.exe -E vs_main -T vs_6_6 -Fh " .. SHADER_DEST_PATH .. "/%{file.basename}_vs.hpp -Vn %{file.basename}_vs %{file.relpath}",
            "..\\..\\contrib\\bin\\dxc\\dxc.exe -E ps_main -T ps_6_6 -Fh " .. SHADER_DEST_PATH .. "/%{file.basename}_ps.hpp -Vn %{file.basename}_ps %{file.relpath}",
            "..\\..\\contrib\\bin\\dxc\\dxc.exe -E cs_main -T cs_6_6 -Fh " .. SHADER_DEST_PATH .. "/%{file.basename}_cs.hpp -Vn %{file.basename}_cs %{file.relpath}"
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

    links { "googletest", "rnCommon", "rnRHI", "rnRHID3D12", "dxgi" }