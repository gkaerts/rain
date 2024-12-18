project "game"

    kind "ConsoleApp"
    language "C++"
    cppdialect "C++20"
    flags { "FatalWarnings", "MultiProcessorCompile" }

    files {
        "src/**.hpp",
        "src/**.cpp"
    }

    includedirs(RN_COMMON_INCLUDES)
    includedirs(RN_RHI_INCLUDES)
    includedirs(RN_RHI_D3D12_INCLUDES)
    includedirs(RN_RENDER_GRAPH_INCLUDES)
    includedirs(RN_APPLICATION_INCLUDES)
    includedirs(RN_ASSET_INCLUDES)
    includedirs(RN_DATA_INCLUDES)
    includedirs(RN_SCENE_INCLUDES)
    includedirs(RN_RENDER_INCLUDES)
    includedirs(RN_IMGUI_INCLUDES)

    libdirs {
        "%{wks.location}/%{cfg.buildcfg}"
    }

    targetdir "%{wks.location}/%{cfg.buildcfg}/"

    links { "rnCommon", "rnRHI", "rnRHID3D12", "rnApplication", "rnRenderGraph", "rnAsset", "rnData", "rnRender", "imgui", "dxgi", "WinPixEventRuntime" }