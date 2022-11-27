local main = "src/LuaBinding.h"
local dir = main:match("(.*)[/\\]") .. "/"
main = main:match("[^/\\]*$")

print("Generating single header file...")

local files = {}
local function add_file(file)
    local f = io.open(dir .. file, "r")
    if not f then error("File not found: " .. file) end
    local content = f:read("*all")
    f:close()
    files[file] = content
end
add_file(main)

local function resolve_deps(file)
    local content = files[file]:gsub("#pragma once", "")
    return (string.gsub(content, "#include \"([^\"]+)\"",
        function(dep)
            if not files[dep] then
                print("+" .. dep)
                add_file(dep)
                return resolve_deps(dep)
            else
                print("~" .. dep)
                return ""
            end
        end
    ))
end

local f = io.open(main, "w")
if not f then error("Could not open file for writing: " .. main) end
f:write("#pragma once\n" ..  resolve_deps(main))
f:close()