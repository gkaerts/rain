Schema = require "schema"
cpp = require "cpp"

local Generate = {}
Generate.printUsageText = function()
    print("Usage: generate [OPTION]... SCHEMA_NAME")

    for _, v in ipairs(Generate.Options) do
        local str = string.format("-%s %s", v.option, v.parameter)
        print(string.format("%s%s%s", str, string.rep(" ", math.max(0, 30 - string.len(str))), v.description))
    end

    print ("Example: generate -cpp -o ./output -I ./import my_schema_file")
end

Generate.Options = {
    {
        option = "cpp",
        parameter = "",
        description = "Generates a C++ representation of the defined schema types in a hpp and cpp file",
        onOptionFound = function(args, value) args.generateCpp = true end
    },
    {
        option = "o",
        parameter = "PATH",
        description = "Path to write the generated output files to. Relative directory structures will be maintained compared to the current working directory.",
        onOptionFound = function(args, value) args.outputPath = value end
    },
    {
        option = "I",
        parameter = "IMPORT_DIR",
        description = "Specifies an import directory to use for importing schema files",
        onOptionFound = function(args, value) args.importDirs = value .. ";" .. args.importDirs end
    },
    {
        option = "h",
        parameter = "",
        description = "Prints this usage text",
        onOptionFound = function(args, value) Generate.printUsageText() end
    }
}


Generate.parseArgs = function()
    local inputArgs = {
        generateCpp = false,
        outputPath = "./",
        importDirs = "./;",
        schemaName = ""
    }

    for i=1, #arg do
        
        if arg[i]:sub(1, 1) == "-" then
            local option = arg[i]:sub(2, arg[i]:len())
            for _, o in ipairs(Generate.Options) do
                if option == o.option then
                    local value = ""
                    if o.parameter:len() > 0 then
                        value = arg[i + 1]
                    end
                    o.onOptionFound(inputArgs, value)
                end
            end
        else
            inputArgs.schemaName = arg[i]
        end
    end

    return inputArgs
end

Generate.doRun = function(inputArgs)
    
    local env = Schema.makeEnv(_ENV)
    Schema.makeEnvAPI(env)
    env._importDirs = inputArgs.importDirs

    if inputArgs.schemaName:len() > 0 then
        
        -- Work around the fact that premake has a custom loadfile implementation which ignores the env parameter
        -- See: https://github.com/premake/premake-core/issues/1392 
        -- local f = assert(loadfile(inputArgs.schemaName .. ".lua", "t", env))
        local schemaFileName = inputArgs.schemaName
        local schemaFileContents = io.input(schemaFileName):read("*a")
        local f = assert(load(schemaFileContents, schemaFileName, "t", env))
        f()

        if inputArgs.generateCpp then
            local outputBaseName = path.getbasename(schemaFileName)
            local generator = cpp:new(inputArgs.outputPath .. "/" .. outputBaseName .. "_gen", Schema.typesToSerialize(env), env._namespace, env._importedFiles)
            generator:writeHeader()
            generator:writeCPP()
        end
    else
        Generate.printUsageText()
    end
end

Generate.fromCmdLine = function()
    local inputArgs = Generate.parseArgs()
    Generate.doRun(inputArgs)
end
return Generate