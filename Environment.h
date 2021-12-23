#pragma once

namespace LuaBinding {
    class Environment : public ObjectRef {
    protected:
        static int lookup_global(lua_State* L)
        {
            printf("lookup");
            lua_pushvalue(L, -2);
            lua_pushvalue(L, -2);
            lua_rawget(L, -2);
            if (!lua_isnoneornil(L, -1))
                return 1;
            lua_pop(L, 1);
            lua_rawget(L, LUA_GLOBALSINDEX);
            return !lua_isnoneornil(L, -1);
        }
    public:
        Environment() = default;
        explicit Environment(lua_State* L)
        {
            this->L = L;
            lua_newtable(L);

            lua_newtable(L);
            lua_getglobal(L, "_G");
            lua_setfield(L, -2, "__index");
            lua_setmetatable(L, -2);

            this->idx = luaL_ref(L, LUA_REGISTRYINDEX);
        }
        explicit Environment(lua_State* L, int findex)
        {
            this->L = L;
            lua_getfenv(L, findex);
            this->idx = luaL_ref(L, LUA_REGISTRYINDEX);
        }
        explicit Environment(lua_State* L, bool)
        {
            this->L = L;
            lua_getfenv(L, lua_upvalueindex(1));
            this->idx = luaL_ref(L, LUA_REGISTRYINDEX);
        }
        int pcall(int narg = 0, int nres = 0) {
            lua_rawgeti(L, LUA_REGISTRYINDEX, idx);
            lua_setfenv(L, lua_gettop(L) - narg - 1);
            int errindex = lua_gettop(L) - narg;
            lua_pushcclosure(L, tack_on_traceback, 0);
            lua_insert(L, errindex);
            auto status = lua_pcall(L, narg, nres, errindex);
            lua_remove(L, errindex);
            return status;
        }
    };
}