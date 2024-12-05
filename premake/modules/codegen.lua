local p = premake
p.modules.codegen = {}

local project = p.project
local tree = p.tree

local api = p.api
api.register {
    name = "codegenfiles",
    scope = "config",
    kind = "list:file",
    tokens = true
}

api.register {
    name = "codegentargets",
    scope = "config",
    kind = "list:string",
}

api.register {
    name = "codegenimportdirs",
    scope = "config",
    kind = "list:directory",
    token = true
}

local schema = require("tools/luagen/lua/schema")
local luagen = require("tools/luagen/lua/generate")
local m = p.modules.codegen
newaction {
    trigger = "codegen",
    description = "Does a code generation pass on all projects using luagen",

    onStart = function()
    end,

    onWorkspace = function(wks)
    end,

    onProject = function(prj)
        local inputArgs = {
            generateCpp = false,
            outputPath = "./",
            importDirs = "./;",
            schemaName = ""
        }

        local cfg = project.getfirstconfig(prj)
        for _, v in ipairs(cfg.codegentargets) do
            if v == "cpp" then
                inputArgs.generateCpp = true
            end
        end

        for _, v in ipairs(cfg.codegenimportdirs) do
            inputArgs.importDirs = v .. ";" .. inputArgs.importDirs
        end

        for _, v in ipairs(cfg.codegenfiles) do
            inputArgs.schemaName = v
            inputArgs.outputPath = prj.location .. "/" .. prj.name
            
            luagen.doRun(inputArgs)
        end
    end
}