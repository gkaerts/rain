include "../../contrib/projects/imgui"

project "rnRender"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"
    flags { "FatalWarnings", "MultiProcessorCompile" }

    codegenfiles {
        "schema/**.lua"
    }

    codegentargets {
        "cpp"
    }

    codegenimportdirs {
        "schema",
        "../asset/schema",
        "../data/schema"
    }

    files { 
        "src/**.h", 
        "src/**.hpp", 
        "src/**.c", 
        "src/**.cpp", 
        "include/**.h",
        "include/**.hpp", 
        "shaders/**.toml",
        PROJECT_ROOT .. "/build/codegen/rnRender/**.hpp",
        PROJECT_ROOT .. "/build/codegen/rnRender/**.cpp",
    }

    filter "files:**.toml"
        buildmessage ""
        buildcommands {
            "%{wks.location}%{cfg.buildcfg}\\data_build.exe -o " .. GENERATED_FILE_PATH .. "/rnRender -cache " .. GENERATED_FILE_PATH .. "/rnRender/cache -root ./../../libs/render/shaders %{file.relpath}"
        }

        buildoutputs {
            "FORCE_ASSET_REBUILD_IGNORE_THIS_WARNING"
        }
    filter {}

    RN_RENDER_INCLUDES = {
        "%{wks.location}/../../libs/render/include",
        PROJECT_ROOT .. "/build/codegen/rnRender",
        GENERATED_FILE_PATH .. "/rnRender"
    }

    includedirs(RN_COMMON_INCLUDES)
    includedirs(RN_RHI_INCLUDES)
    includedirs(RN_ASSET_INCLUDES)
    includedirs(RN_DATA_INCLUDES)
    includedirs(RN_RENDER_GRAPH_INCLUDES)
    includedirs(RN_RENDER_INCLUDES)
    includedirs(RN_IMGUI_INCLUDES)
    includedirs(RN_SDL_INCLUDES)
    if BUILD_PROPERTIES.RequiresSDL then
        

        dependson { "SDL" }
    end

    includedirs {
        GENERATED_FILE_PATH .. "/rnRender"
    }
    
    libdirs {
        "%{wks.location}/%{cfg.buildcfg}"
    }

    targetdir "%{wks.location}/%{cfg.buildcfg}/"
    
    dependson { "data_build" }
    links { "rnCommon", "rnRHI", "rnAsset", "rnData", "rnRenderGraph", "imgui" }

if BUILD_PROPERTIES.IncludeTestsInBuild then
    include "test"
end