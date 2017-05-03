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

-- Replace module loader
local function halleyPackageLoader(moduleName)
    return function()
        return api.packageLoader(moduleName)
    end
end

package.searchers = {package.searchers[1], halleyPackageLoader}

-- Disable unsafe tables
io = nil
os = nil
debug = nil


return {}