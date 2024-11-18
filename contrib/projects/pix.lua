project "pix"
    kind "Utility"

    local target_dir = path.translate("%{wks.location}/%{cfg.buildcfg}/", '\\')
    local lib_dir = path.translate(path.getabsolute("../third_party/pix/bin/%{cfg.platform}/"), '\\')

    postbuildcommands {
        "xcopy " .. lib_dir .. "\\*.lib " .. target_dir .. "\\* /Y",
        "xcopy " .. lib_dir .. "\\*.dll " .. target_dir .. "\\* /Y"
    }