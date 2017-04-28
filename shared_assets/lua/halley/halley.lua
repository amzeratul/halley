-- Disable unsafe tables
io = nil
os = nil
debug = nil

-- Capture halleyAPI
local api = halleyAPI
halleyAPI = nil

-- Replace global print() function
print = function(...)
    local result = ""
    local args = {...}
    for i,v in ipairs(args) do
        if i > 1 then
            result = result .. "\t"
        end
        result = result .. tostring(v)
    end
    api.print(result)
end

-- Some useful functions
functional = {}

function functional.any(vs)
    for i,v in ipairs(vs) do
        if v then
            return true
        end
    end
    return false
end

function functional.all(vs)
    for i,v in ipairs(vs) do
        if not v then
            return false
        end
    end
    return true
end

function functional.anyWith(f, vs)
    for i,v in ipairs(vs) do
        if f(v) then
            return true
        end
    end
    return false
end

function functional.map(f, vs)
    local result = {}
    for i,v in ipairs(vs) do
        table.insert(result, f(v))
    end
    return result
end

function functional.filter(f, vs)
    local result = {}
    for i,v in ipairs(vs) do
        if f(v) then
            table.insert(result, v)
        end
    end
    return result
end

return {}