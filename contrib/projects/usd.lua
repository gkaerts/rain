project "usd"
    kind "Utility"

    files {
        OPENUSD_BUILD_PATH .. "/%{cfg.buildcfg}/lib/**.dll",
        OPENUSD_BUILD_PATH .. "/%{cfg.buildcfg}/bin/**.dll"
    }

    local target_dir = path.translate("%{wks.location}/%{cfg.buildcfg}/", '\\')
    filter 'files:**.dll'
        buildmessage 'Copying %{file.abspath}'
        buildcommands {
            "copy %{file.abspath} " .. target_dir .. "\\%{file.name}"
        }

        buildoutputs {
            target_dir .. '/%{file.name}'
        }