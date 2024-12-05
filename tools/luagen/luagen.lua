local PATH = arg[0]
print(PATH)
local generate = require "lua/generate"
generate.fromCmdLine()