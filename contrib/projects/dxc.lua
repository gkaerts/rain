project "dxc"
    kind "Utility"

    externaldownloadurl "https://github.com/microsoft/DirectXShaderCompiler/releases/download/v1.8.2407/dxc_2024_07_31.zip"
    externaldownloadname "dxc_2024_07_31.zip"
    externaldownloadtype "Zip"
    
    local lib_dir = path.translate(path.getabsolute("../third_party/dxc/lib/x64/"), '\\')
    local bin_dir = path.translate(path.getabsolute("../third_party/dxc/bin/x64/"), '\\')
    
    RN_DXC_INCLUDES = {
        "%{wks.location}/../../downloads/dxc/inc"
    }
    
    
    local target_dir = path.translate("%{wks.location}/%{cfg.buildcfg}/", '\\')
    files {
        "%{wks.location}/../../downloads/dxc/bin/x64/**.dll",
        "%{wks.location}/../../downloads/dxc/lib/x64/**.lib"
    }

    filter 'files:**.dll or **.lib'
        buildmessage 'Copying %{file.abspath}'
        buildcommands {
            "copy %{file.abspath} " .. target_dir .. "\\%{file.name}"
        }

        buildoutputs {
            target_dir .. '/%{file.name}'
        }
    