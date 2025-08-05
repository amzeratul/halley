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

printError = function(...)
    local result = ""
    local args = {...}
    for i,v in ipairs(args) do
        if i > 1 then
            result = result .. "\t"
        end
        result = result .. tostring(v)
    end
    api.printError(result)
end

printWarning = function(...)
    local result = ""
    local args = {...}
    for i,v in ipairs(args) do
        if i > 1 then
            result = result .. "\t"
        end
        result = result .. tostring(v)
    end
    api.printWarning(result)
end

printDev = function(...)
    local result = ""
    local args = {...}
    for i,v in ipairs(args) do
        if i > 1 then
            result = result .. "\t"
        end
        result = result .. tostring(v)
    end
    api.printDev(result)
end

-- Replace module loader
local function halleyPackageLoader(moduleName)
    return function()
        return api.packageLoader(moduleName)
    end
end

-- Replace coroutine resume
local origTraceback = debug.traceback
local origResume = coroutine.resume
coroutine.resume = function(co, ...)
    local results = table.pack(origResume(co, ...))
    if results[1] then
        return table.unpack(results)
    else
        return false, "Coroutine Error:\n\t-- START OF COROUTINE STACK --\n" .. api.handleCoroutineError(co, results[2]) .. "\n\t-- END OF COROUTINE STACK ---\n"
    end
end

package.searchers = {package.searchers[1], halleyPackageLoader}



-- Load strict
local getinfo, error, rawset, rawget = debug.getinfo, error, rawset, rawget

local mt = getmetatable(_G)
if mt == nil then
    mt = {}
    setmetatable(_G, mt)
end

mt.__declared = {}

local function what ()
    local d = getinfo(3, "S")
    return d and d.what or "C"
end

mt.__newindex = function (t, n, v)
    if not mt.__declared[n] then
        local w = what()
        if w ~= "main" and w ~= "C" then
            printError("Lua error: assign to undeclared variable '"..n.."'")
        end
        mt.__declared[n] = true
    end
    rawset(t, n, v)
end
  
mt.__index = function (t, n)
    if not mt.__declared[n] and what() ~= "C" then
        printError("Lua error: variable '"..n.."' is not declared")
        rawset(t, n, nil)
        mt.__declared[n] = true
    end
    return rawget(t, n)
end



-- Disable unsafe tables
io = nil
os = nil
debug = nil


return {}