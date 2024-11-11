project "d3d12agility"
    kind "Utility"
    language "C++"
    cppdialect "C++20"

    files {
        "../third_party/d3d12agility/include/**.h"
    }
    
    local target_dir = path.translate("%{wks.location}/%{cfg.buildcfg}/", '\\')
    local bin_dir = path.translate(path.getabsolute("../third_party/d3d12agility/bin/x64"), '\\')

    postbuildcommands {
        "xcopy " .. bin_dir .. "\\*.dll " .. target_dir .. "\\* /Y"
    }