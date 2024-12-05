local FileWriter = {}
function FileWriter:new(filename)
    o = o or {}
    setmetatable(o, self)
    self.__index = self
    self.file = io.open(filename, "w")
    self.indentation = 0

    return self
end

function FileWriter:lineBreak()
    self:write("\n")
end

function FileWriter:write(fmt, ...)
    self.file:write(string.format(fmt, ...))
end

function FileWriter:writeLn(fmt, ...)
    for i=0, self.indentation-1 , 1 do
        self:write("\t")
    end

    self:write(fmt, ...)
    self:lineBreak()
end

function FileWriter:indent()
    self.indentation = self.indentation + 1
end

function FileWriter:unindent()
    if (self.indentation >= 1) then
        self.indentation = self.indentation - 1
    end
end

return FileWriter