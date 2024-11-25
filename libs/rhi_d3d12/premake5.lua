if BUILD_PROPERTIES.SupportsD3D12 then

    include "../../contrib/projects/d3d12agility"
    include "../../contrib/projects/pix"

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

        RN_RHI_D3D12_INCLUDES = {
            "%{wks.location}/../../libs/rhi_d3d12/include",
        }
    
        includedirs(RN_COMMON_INCLUDES)
        includedirs(RN_RHI_INCLUDES)
        includedirs(RN_D3D12_AGILITY_INCLUDES)
        includedirs(RN_PIX_INCLUDES)
        includedirs(RN_RHI_D3D12_INCLUDES)

        libdirs {
            "%{wks.location}/%{cfg.buildcfg}"
        }

        targetdir "%{wks.location}/%{cfg.buildcfg}/"
        
        dependson { "d3d12agility", "pix" }
        links { "rnCommon", "rnRHI", "d3d12", "dxguid" }

    if BUILD_PROPERTIES.IncludeTestsInBuild then
        include "test"
    end
end