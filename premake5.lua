require("premake/modules")

-- Options
newoption {
    trigger = "platform",
    description = "Platform to generate project and solution files for",
    allowed = {
        { "win64",              "Windows x86-64" },
    },
    default = "win64"
}

PLATFORM_BUILD_PROPERTIES = {
    win64 = {
        IncludeTestsInBuild = true,
        SupportsD3D12 = true,
        SupportsVulkan = true,
        RequiresExternalVulkanLib = true,
        RequiresSDL = true
    },
}

BUILD_PROPERTIES = PLATFORM_BUILD_PROPERTIES[_OPTIONS["platform"]]

workspace "rain"

    -- Windows x64 options
    platforms { _OPTIONS["platform"] }
    filter { "options:platform=win64" }
        system "windows"
        architecture "x86_64"
        buildoptions {"/utf-8"}

    -- Workspace build configurations
    configurations { "Debug", "Release" }

    filter "configurations:Debug"
        symbols "On"
        runtime "Debug"
        defines { "DEBUG" }
        optimize "Off"
        inlining "Disabled"

    filter "configurations:Release"
        symbols "On"
        runtime "Release"
        defines { "NDEBUG" }
        optimize "Speed"
        inlining "Auto"

    filter {}

    -- Build location
    location("build/" .. _OPTIONS["platform"])

    -- Platform specific libs
    if BUILD_PROPERTIES.SupportVulkan and BUILD_PROPERTIES.RequiresExternalVulkanLib then
        include "contrib/projects/vulkan"
    end

    if BUILD_PROPERTIES.IncludeTestsInBuild then
        include "contrib/projects/googletest"
    end

    -- Projects
    include "contrib/projects/spdlog"
    include "contrib/projects/enkiTS"
    include "libs"
    include "tools"
    include "app"
