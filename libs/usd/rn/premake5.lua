project "usd_rn_schema"
    kind "Utility"

    files {
        "schema.usda"
    }

    filter "files:**.usda"
        buildmessage ""
        buildcommands {
            "echo Path is at: $(PATH)",
            "usdGenSchema %{file.relpath} " .. GENERATED_FILE_PATH .. "/usd_plugins/rn"
        }

        buildoutputs {
            GENERATED_FILE_PATH .. "/usd_plugins/rn/generatedSchema.usda"
        }

    dependson("usd")

project "usd_rn"
    kind "SharedLib"
    language "C++"
    cppdialect "C++17"

    flags {
        "MultiProcessorCompile"
    }

    disablewarnings {
        "4003",
        "4244",
        "4305",
        "4267",
        "4506",
        "4091",
        "4273",
        "4180",
        "4334"
    }

    defines { 
        "NOMINMAX",
        "RN_EXPORTS=1"
    }

    -- Workaround for an annoying USD include
    filter "configurations:Debug"
        defines { "TBB_USE_DEBUG=1" }
    
    filter "configurations:Release"
        defines { "TBB_USE_DEBUG=0" }
    filter{}

    files {
        GENERATED_FILE_PATH .. "/usd_plugins/rn/**.cpp",
        GENERATED_FILE_PATH .. "/usd_plugins/rn/**.h",
        "**.cpp",
    }

    includedirs {
        GENERATED_FILE_PATH .. "/usd_plugins/rn"
    }
    includedirs(OPENUSD_INCLUDE_DIRS)
    libdirs(OPENUSD_LIB_DIRS)
    links { "usd_ms" }
    dependson { "usd_rn_schema" }
    targetdir "%{wks.location}/%{cfg.buildcfg}/"