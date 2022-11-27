local main = "src/LuaBinding.h"
local dir = main:match("(.*)[/\\]") .. "/"
main = main:match("[^/\\]*$")

print("Generating single header file...")

local files = {}
local function add_file(file)
    local f = io.open(dir .. file, "r")
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
f:write("#pragma once\n" ..  resolve_deps(main))
f:close()