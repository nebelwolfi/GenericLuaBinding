require("mysql")

local session = Session("127.0.0.1", "root", "", "TestDB")
local res = session:sql("SELECT * FROM testtable WHERE testkey = 1")
if not res then
	print("No results")
	return
end

print(res:get(1):string(), res:get(2):int(), res:get(3):double(), res:get(4):bool())