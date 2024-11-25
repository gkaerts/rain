project "d3d12agility"
    kind "Utility"

    externaldownloadurl "https://www.nuget.org/api/v2/package/Microsoft.Direct3D.D3D12/1.614.1"
    externaldownloadname "d3dagility.1.614.1.zip"
    externaldownloadtype "Zip"

    RN_D3D12_AGILITY_INCLUDES = {
        "%{wks.location}/../../downloads/d3d12agility/build/native/include"
    }

    local target_dir = path.translate("%{wks.location}/%{cfg.buildcfg}/", '\\')

    files {
        "%{wks.location}/../../downloads/d3d12agility/build/native/bin/x64/**.dll"
    }

    filter 'files:**.dll'
        buildmessage 'Copying %{file.abspath}'
        buildcommands {
            "copy %{file.abspath} " .. target_dir .. "\\%{file.name}"
        }

        buildoutputs {
            target_dir .. '/%{file.name}'
        }
