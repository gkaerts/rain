PYTHON_VENV_PATH = PROJECT_ROOT .. "/build/python/venv"
DOWNLOADS_PATH = PROJECT_ROOT .. "/downloads"

include "usd_paths"

local downloadProgress = function (name, total, current)
    local ratio = current / total;
    ratio = math.min(math.max(ratio, 0), 1);
    local percent = math.floor(ratio * 100);
    print("Downloading " .. name .. " (" .. percent .. "%/100%)")
end

newaction {
    trigger = "setup",
    description = "Handles initial setup of the project. Installs a python virtual environment and downloads and builds OpenUSD.",

    onStart = function()

        print("Setting up submodules")
        os.execute("git submodule update --init --recursive")
        
        print("Downloading LFS objects")
        os.execute("git lfs pull")

        if not os.isdir(PYTHON_VENV_PATH) then
            print("Generating python venv in build/python/venv")
            os.execute("python -m venv " .. PYTHON_VENV_PATH)
        else
            print("Existing python venv found")
        end

        print ("Installing required python packages in venv")
        os.execute[["powershell scripts/InstallPythonVenvPackages.ps1"]]

        if not os.isdir(DOWNLOADS_PATH) then
            os.mkdir(DOWNLOADS_PATH)
        end

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