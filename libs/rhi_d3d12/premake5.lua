if BUILD_PROPERTIES.SupportsD3D12 then

    include "../../contrib/projects/d3d12agility"

    project "rnRHID3D12"
        kind "StaticLib"
        language "C++"
        cppdialect "C++20"
        flags { "FatalWarnings", "MultiProcessorCompile" }

        files { 
            "src/**.h", 
            "src/**.hpp", 
            "src/**.c", 
            "src/**.cpp", 
            "include/**.h",
            "include/**.hpp", 
        }

        includedirs { 
            "include",
            "../common/include",
            "../rhi/include",
            "../../contrib/submodules/spdlog/include",
            "../../contrib/submodules/unordered_dense/include",
            "../../contrib/submodules/enkiTS/src",
            "../../contrib/submodules/hlslpp/include",
            "../../contrib/third_party/d3d12agility/include"
        }

        libdirs {
            "%{wks.location}/%{cfg.buildcfg}"
        }

        targetdir "%{wks.location}/%{cfg.buildcfg}/"
        
        dependson { "d3d12agility" }
        links { "rnCommon", "rnRHI", "d3d12", "dxguid" }

    if BUILD_PROPERTIES.IncludeTestsInBuild then
        include "test"
    end
end