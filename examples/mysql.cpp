#include "lua.hpp"
#include "LuaBinding.h"
#include "mysqlx/xdevapi.h"

extern "C" int luaopen_mysql(lua_State *L)
{
    auto State = LuaBinding::State(L);

    State.addClass<mysqlx::Session>("Session")
        .cfun("sql", [](mysqlx::Session* session, LuaBinding::State S) {
            try {
                auto query = S.front().as<const char*>();
                mysqlx::SqlResult r = session->sql(query).execute();
                if (r.count() == 0)
                    return 0;
                if (r.count() == 1)
                    S.alloc<mysqlx::Row>(r.fetchOne());
                else
                {
                    auto rowlist = r.fetchAll();
                    std::vector<mysqlx::Row> rows;
                    for (auto row : rowlist)
                        rows.push_back(row);
                    S.push(rows);
                }
                return (int)r.count();
            } catch (const std::exception& e) {
                S.error(e.what());
            }
            return 0;
        });

    State.addClass<mysqlx::Row>("Row")
        .fun("get", [](mysqlx::Row* row, int index) {
            return (*row)[index];
        });

    State.addClass<mysqlx::Value>("Value")
        .fun("string", [](mysqlx::Value* value) {
            return value->get<std::string>();
        })
        .fun("int", [](mysqlx::Value* value) {
            return value->get<int>();
        })
        .fun("double", [](mysqlx::Value* value) {
            return value->get<double>();
        })
        .fun("bool", [](mysqlx::Value* value) {
            return value->get<bool>();
        });
    
    State["Session"] = [](LuaBinding::State State) {
        try {
            auto args = State.args();
            if (args.size() == 1)
                State.alloc<mysqlx::Session>(args[0].as<const char*>());
            else if (args.size() == 4)
            {
                State.alloc<mysqlx::Session>(args[0].as<const char*>(), args[1].as<const char*>(), args[2].as<const char*>(), args[3].as<const char*>());
            }
            else
                throw std::runtime_error("Invalid number of arguments");
        } catch (const std::exception& e) {
            State.error(e.what());
        }
    };

    return 0;
}