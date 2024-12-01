local p = premake
p.modules.download = {}

local api = p.api
api.register {
    name = "externaldownloadurl",
    scope = "project",
    kind = "string",
}

api.register {
    name = "externaldownloadname",
    scope = "project",
    kind = "string",
}

api.register {
    name = "externaldownloadtype",
    scope = "project",
    kind = "string",
    allowed = {
        "None",
        "Zip"
    }
}

local downloadProgress = function (name, total, current)
    local ratio = current / total;
    ratio = math.min(math.max(ratio, 0), 1);
    local percent = math.floor(ratio * 100);
    print("Downloading " .. name .. " (" .. percent .. "%/100%)")
end

local m = p.modules.download
newaction {
    trigger = "download_dependencies",
    description = "Downloads any external dependencies",

    onStart = function()
    end,

    onWorkspace = function(wks)
    end,

    onProject = function(prj)
        if prj.externaldownloadurl and prj.externaldownloadname and prj.externaldownloadtype then
            local filename = path.join(prj.location, prj.externaldownloadname)
            if not os.isfile(filename) then
                http.download(prj.externaldownloadurl, filename, {
                    progress = function(total, current) downloadProgress(prj.externaldownloadname, total, current) end
                })
            end

            if prj.externaldownloadtype == "Zip" then
                local destdir = path.join(path.getdirectory(filename), prj.name)
                if not os.isdir(destdir) then
                    zip.extract(filename, destdir)
                end
            end
        end
    end
}

PYTHON_VENV_PATH = PROJECT_ROOT .. "/build/python/venv"
DOWNLOADS_PATH = PROJECT_ROOT .. "/downloads"

require("usd_paths")

newaction {
    trigger = "first_time_setup",
    description = "Handles initial setup of the project. Installs a python virtual environment and downloads and builds OpenUSD.",

    onStart = function()
        if not os.isdir(PYTHON_VENV_PATH) then
            print("Generating python venv in build/python/venv")
            os.execute("python -m venv " .. PYTHON_VENV_PATH)
        else
            print("Existing python venv found")
        end

        print ("Installing required python packages in venv")
        os.execute[["powershell scripts/InstallPythonVenvPackages.ps1"]]

        if not os.isfile(OPENUSD_DOWNLOAD_FULL_PATH) then
            print("Downloading OpenUSD")
            http.download(OPENUSD_DOWNLOAD_URL, OPENUSD_DOWNLOAD_FULL_PATH, {
                progress = function(total, current) downloadProgress(OPENUSD_DOWNLOAD_NAME, total, current) end
            })
        else
            print("Existing OpenUSD download found")
        end

        if not os.isdir(OPENUSD_SOURCE_PATH) then
            print("Extracting " .. OPENUSD_DOWNLOAD_NAME)
            zip.extract(OPENUSD_DOWNLOAD_FULL_PATH, OPENUSD_SOURCE_PATH)
        else
            print("OpenUSD directory exists. Skipping unzip.")
        end

        print("Building OpenUSD")
        os.execute[["powershell scripts/BuildOpenUSD.ps1"]]

    end
}