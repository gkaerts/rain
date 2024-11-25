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
