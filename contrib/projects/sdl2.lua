project "SDL"
    kind "Utility"

    externaldownloadurl "https://github.com/libsdl-org/SDL/releases/download/release-2.30.9/SDL2-devel-2.30.9-VC.zip"
    externaldownloadname "SDL2-devel-2.30.9-VC.zip"
    externaldownloadtype "Zip"

    RN_SDL_INCLUDES = {
        PROJECT_ROOT .. "/downloads/SDL/SDL2-2.30.9/include"
    }
    
    local target_dir = path.translate("%{wks.location}/%{cfg.buildcfg}/", '\\')

    files {
        PROJECT_ROOT .. "/downloads/SDL/SDL2-2.30.9/lib/x64/**.dll",
        PROJECT_ROOT .. "/downloads/SDL/SDL2-2.30.9/lib/x64/**.lib"
    }

    filter 'files:**.dll or **.lib'
        buildmessage 'Copying %{file.abspath}'
        buildcommands {
            "copy %{file.abspath} " .. target_dir .. "\\%{file.name}"
        }

        buildoutputs {
            target_dir .. '/%{file.name}'
        }