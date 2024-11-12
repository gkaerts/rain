project "SDL2"
    kind "Utility"
    language "C++"
    cppdialect "C++20"
    
    local target_dir = path.translate("%{wks.location}/%{cfg.buildcfg}/", '\\')
    local lib_dir = path.translate(path.getabsolute("../third_party/sdl-2.30.9/lib/%{cfg.buildcfg}/"), '\\')

    postbuildcommands {
        "xcopy " .. lib_dir .. "\\*.lib " .. target_dir .. "\\* /Y",
        "xcopy " .. lib_dir .. "\\*.dll " .. target_dir .. "\\* /Y"
    }