local Schema = {}

Schema.TypeLayout = {
    Primitive = 0,
    Struct =    1,
    Enum =      2,
    Span =      3,
    String =    4,
}

Schema.SpanByteSize = 16
Schema.StringByteSize = 16
Schema.EnumByteSize = 4

Schema.PrimitiveType = {
    Uint8 =     1,
    Uint16 =    2,
    Uint32 =    3,
    Uint64 =    4,
    Int8 =      5,
    Int16 =     6,
    Int32 =     7,
    Int64 =     8,
    Float =     9,
    Double =    10,
}

Schema.PrimitiveSizes = {
    1, -- Uint8
    2, -- Uint16
    4, -- Uint32
    8, -- Uint6
    1, -- Int8
    2, -- Int16
    4, -- Int32
    8, -- Int64
    4, -- Float
    8, -- Double
}

Schema.typesToSerialize = function(env)

    local outTypes = {}
    for i, t in ipairs(env._orderedTypes) do
        for k, v in pairs(env) do
            if v and v == t then
                table.insert(outTypes, {
                    name = k,
                    type = v
                })
            end
        end
    end
    
    return outTypes
end

local makeprimitive = function(type, env)
    local ret = {
        layout = Schema.TypeLayout.Primitive,
        sizeInBytes = Schema.PrimitiveSizes[type],
        primitive = type
    }

    table.insert(env._orderedTypes, ret)
    return ret
end

local makestring = function(env)
    local ret = {
        layout = Schema.TypeLayout.String,
        sizeInBytes = Schema.StringByteSize,
    }

    table.insert(env._orderedTypes, ret)
    return ret
end

local validateValueAgainstSchema = function(schemaType, value, recursion)

    if schemaType.layout == Schema.TypeLayout.Primitive then
        assert(type(value) == "number", "Expected value type of 'number', but got '" .. type(value) .. "'")

    elseif schemaType.layout == Schema.TypeLayout.String then
        assert(type(value) == "string", "Expected value type of 'string', but got '" .. type(value) .. "'")

    elseif schemaType.layout == Schema.TypeLayout.Enum then
        assert(type(value) == "string", "Expected value type of 'string', but got '" .. type(value) .. "'")

        local valueFound = false
        for _, v in ipairs(schemaType.elements) do
            if v.name == value then
                break
            end
        end
        assert(valueFound, "Value '" .. value .. "' is not a valid enum value for the provided type")

    elseif schemaType.layout == Schema.TypeLayout.Struct then
        assert(type(value) == "table", "Expected value type of 'table', but got '" .. type(value) .. "'")

        for _, f in ipairs(schemaType.elements) do
            local fieldValue = value[f.name]
            assert(fieldValue, "Expected field with name '" .. f.name .. ", but none found")
            recursion(f.type, fieldValue)
        end

    elseif schemaType.layout == Schema.TypeLayout.Span then
        assert(type(value) == "table", "Expected value type of 'table', but got '" .. type(value) .. "'")

        for _, v in ipairs(value) do
            recursion(schemaType.spannedType, v)
        end

    end
end

Schema.makeEnv = function(oldEnv)
    local env = {

        -- Sandbox
        assert = assert,
        pairs = pairs,
        type = type,
        print = print,
        math = math,
        string = string,
        table = table,
        Schema = Schema,

        -- Metadata
        _currentNamespace = "",
        _orderedTypes = {},
        _oldENV = oldEnv,
        _importDirs = {},
        _importedFiles = {},
    }
    
    

     -- Builtin types
    env.uint8 =     makeprimitive(Schema.PrimitiveType.Uint8, env)
    env.uint16 =    makeprimitive(Schema.PrimitiveType.Uint16, env)
    env.uint32 =    makeprimitive(Schema.PrimitiveType.Uint32, env)
    env.uint64 =    makeprimitive(Schema.PrimitiveType.Uint64, env)
    env.int8 =      makeprimitive(Schema.PrimitiveType.Int8, env)
    env.int16 =     makeprimitive(Schema.PrimitiveType.Int16, env)
    env.int32 =     makeprimitive(Schema.PrimitiveType.Int32, env)
    env.int64 =     makeprimitive(Schema.PrimitiveType.Int64, env)
    env.float =     makeprimitive(Schema.PrimitiveType.Float, env)
    env.double =    makeprimitive(Schema.PrimitiveType.Double, env)
    env.String =    makestring(env)

    return env
end


Schema.makeEnvAPI = function(env)
    local _import = function(_env, str)
        assert(str and string.len(str) > 0, "Invalid import file provided")
        local _old_struct =  _env.struct
        local _old_enum =  _env.enum

        -- Override default struct and enum functions to force them to be forward declares
        _env.struct =  _env.fwd_struct
        _env.enum =  _env.fwd_enum
        
        local p = os.pathsearch(str,  _env._importDirs)
        assert(p, "Could not resolve import file " .. str)

        p = p .. "/" .. str

        -- Work around the fact that premake has a custom loadfile implementation which ignores the env parameter
        -- See: https://github.com/premake/premake-core/issues/1392 
        -- local f = assert(loadfile(p, "t", _env))
        local importFileContents = io.input(p):read("*a")
        local f = assert(load(importFileContents, p, "t", _env))
        f()

        -- And restore them back
        _env.struct = _old_struct
        _env.enum = _old_enum

        table.insert(_env._importedFiles, str)
    end

    local _namespace = function(_env, str)
        assert(str and string.len(str) > 0, "Invalid namespace provided")

        _env._currentNamespace = str
    end
    
    local _span = function(_env, spannedType)
        assert(spannedType, "Invalid type provided to span")
        assert( spannedType.layout == Schema.TypeLayout.Struct or
                spannedType.layout == Schema.TypeLayout.Enum or
                spannedType.layout == Schema.TypeLayout.Primitive or
                spannedType.layout == Schema.TypeLayout.String,
                "Spanned type needs to be a struct, enum, string or primitive");
        return {
            layout = Schema.TypeLayout.Span,
            sizeInBytes = Schema.SpanByteSize,
            spannedType = spannedType
        }
    end

    local _value = function(_env, name, val)
        assert(name and string.len(name) > 0, "Invalid name provided for enum value")
        assert(not val or type(val) == "number", "Enum value needs to be a number")
        return {
            name = name,
            value = val
        }
    end

    local _enum = function(_env, values)
        assert(values, "Invalid value table provided to enum")
        local ret = {
            layout = Schema.TypeLayout.Enum,
            sizeInBytes = Schema.EnumByteSize,
            elements = values,
            namespace = env._currentNamespace
        }

        table.insert(_env._orderedTypes, ret)
        return ret
    end

    local _fwd_enum = function(_env)
        local ret = {
            layout = Schema.TypeLayout.Enum,
            sizeInBytes = Schema.EnumByteSize,
            namespace = env._currentNamespace,
            external = true
        }

        table.insert(_env._orderedTypes, ret)
        return ret
    end

    local _field = function(_env, fieldType, name)
        assert(string.len(name) > 0, "Invalid name provided for enum value")
        assert(fieldType, "Invalid type provided to struct field")
        return {
            type = fieldType,
            name = name
        }
    end

    local _struct = function(_env, fields)
        assert(fields, "No fields provided to struct")
        local sizeInBytes = 0
        for i, f in pairs(fields) do
            sizeInBytes = sizeInBytes + f.type.sizeInBytes

        end

        local ret = {
            layout = Schema.TypeLayout.Struct,
            sizeInBytes = math.max(4, sizeInBytes),
            elements = fields,
            namespace = _env._currentNamespace,
        }

        table.insert(_env._orderedTypes, ret)
        return ret
    end
    
    local _fwd_struct = function(_env, fields)
        assert(fields, "No fields provided to struct")
        local sizeInBytes = 0
        for i, f in pairs(fields) do
            sizeInBytes = sizeInBytes + f.type.sizeInBytes

        end
        local ret = {
            layout = Schema.TypeLayout.Struct,
            sizeInBytes = math.max(4, sizeInBytes),
            elements = fields,
            namespace = _env._currentNamespace,
            external = true
        }

        table.insert(_env._orderedTypes, ret)
        return ret
    end

    local _decorate = function(_env, type, decorations)
        assert(type, "Invalid type provided to decorate")
        assert(decorations, "No decorations provided to decorate")
        if not type.decorations then
            type.decorations = {}
        end

        for _, d in pairs(decorations) do
            table.insert(type.decorations, d)
        end
    end

    local _decoration = function(_env, type, name, value)
        assert(type, "Invalid type provided to decoration")
        assert(string.len(name) > 0, "Invalid decoration name provided")
        assert(value, "Invalid value provided to decoration")

        validateValueAgainstSchema(type, value, validateValueAgainstSchema)
        return {
            type = type,
            name = name,
            value = value
        }
    end
    

    env.import =        function(str)                   return _import(env, str) end
    env.namespace =     function(str)                   return _namespace(env, str) end
    env.value =         function(name, val)             return _value(env, name, val) end
    env.span =          function(spannedType)           return _span(env, spannedType) end
    env.enum =          function(values)                return _enum(env, values) end
    env.fwd_enum =      function()                      return _fwd_enum(env) end
    env.field =         function(fieldType, name)       return _field(env, fieldType, name) end
    env.struct =        function(fields)                return _struct(env, fields) end
    env.fwd_struct =    function(fields)                return _fwd_struct(env, fields) end
    env.decorate =      function(type, decorations)     return _decorate(env, type, decorations) end
    env.decoration =    function(type, name, value)     return _decoration(env, type, name, value) end
end

return Schema