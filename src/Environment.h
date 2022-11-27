#pragma once

namespace LuaBinding {
    class Environment : public ObjectRef {
    protected:
        static int lookup_global(lua_State* L)
        {
            lua_pushvalue(L, -2);
            lua_pushvalue(L, -2);
            lua_rawget(L, -2);
            if (!lua_isnoneornil(L, -1))
                return 1;
            lua_pop(L, 1);
#if LUA_VERSION_NUM < 502
            lua_rawget(L, LUA_GLOBALSINDEX);
#else
            lua_rawgeti(L, LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS);
#endif
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
#if LUA_VERSION_NUM < 502
            lua_getfenv(L, findex);
#else
            lua_getuservalue(L, findex);
#endif
            this->idx = luaL_ref(L, LUA_REGISTRYINDEX);
        }
        explicit Environment(lua_State* L, bool)
        {
            this->L = L;
#if LUA_VERSION_NUM < 502
            lua_getfenv(L, lua_upvalueindex(1));
#else
            lua_getuservalue(L, lua_upvalueindex(1));
#endif
            this->idx = luaL_ref(L, LUA_REGISTRYINDEX);
        }
        int pcall(int narg = 0, int nres = 0) {
            lua_rawgeti(L, LUA_REGISTRYINDEX, idx);
#if LUA_VERSION_NUM < 502
            lua_setfenv(L, lua_gettop(L) - narg - 1);
#else
            lua_setuservalue(L, lua_gettop(L) - narg - 1);
#endif
            int errindex = lua_gettop(L) - narg;
            lua_pushcclosure(L, tack_on_traceback, 0);
            lua_insert(L, errindex);
            auto status = lua_pcall(L, narg, nres, errindex);
            lua_remove(L, errindex);
            return status;
        }
    };
}