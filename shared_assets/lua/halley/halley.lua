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

function functional.any(f, vs)
    for i,v in ipairs(vs) do
        if (f(v)) then
            return true
        end
    end
    return false
end

return {}