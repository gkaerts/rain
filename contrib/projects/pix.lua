project "pix"
    kind "Utility"

    externaldownloadurl "https://www.nuget.org/api/v2/package/WinPixEventRuntime/1.0.240308001"
    externaldownloadname "winpixeventruntime.1.0.240308001.zip"
    externaldownloadtype "Zip"

    RN_PIX_INCLUDES = {
        "%{wks.location}/../../downloads/pix/Include"
    }

    local target_dir = path.translate("%{wks.location}/%{cfg.buildcfg}/", '\\')

    files {
        "%{wks.location}/../../downloads/pix/bin/x64/**.dll",
        "%{wks.location}/../../downloads/pix/bin/x64/**.lib"
    }

    filter 'files:**.dll or **.lib'
        buildmessage 'Copying %{file.abspath}'
        buildcommands {
            "copy %{file.abspath} " .. target_dir .. "\\%{file.name}"
        }

        buildoutputs {
            target_dir .. '/%{file.name}'
        }