project "vulkan"
    kind "Utility"
    language "C++"
    cppdialect "C++20"

    files {
        "../third_party/vulkan/include/**.h"
    }

    includedirs { "include/vulkan" }
    
    local target_dir = path.translate("%{wks.location}/%{cfg.buildcfg}/", '\\')
    local lib_dir = path.translate(path.getabsolute("../third_party/vulkan/lib/"), '\\')
    local bin_dir = path.translate(path.getabsolute("../third_party/vulkan/bin/"), '\\')

    postbuildcommands {
        "xcopy " .. lib_dir .. "\\*.lib " .. target_dir .. "\\* /Y",
        "xcopy " .. bin_dir .. "\\*.dll " .. target_dir .. "\\* /Y"
    }