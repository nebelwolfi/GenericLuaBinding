#pragma once

#include "lua.hpp"

namespace LuaBinding {

    namespace detail {
        namespace _resolve {
            template <typename Sig, typename C>
            inline Sig C::* resolve_v(std::false_type, Sig C::* mem_func_ptr) {
                return mem_func_ptr;
            }

            template <typename Sig, typename C>
            inline Sig C::* resolve_v(std::true_type, Sig C::* mem_variable_ptr) {
                return mem_variable_ptr;
            }
        } // namespace detail

        template <typename... Args, typename R>
        inline auto resolve(R fun_ptr(Args...))->R(*)(Args...) {
            return fun_ptr;
        }

        template <typename Sig>
        inline Sig* resolve(Sig* fun_ptr) {
            return fun_ptr;
        }

        template <typename... Args, typename R, typename C>
        inline auto resolve(R(C::* mem_ptr)(Args...))->R(C::*)(Args...) {
            return mem_ptr;
        }

        template <typename Sig, typename C>
        inline Sig C::* resolve(Sig C::* mem_ptr) {
            return _resolve::resolve_v(std::is_member_object_pointer<Sig C::*>(), mem_ptr);
        }
    }
    static int lua_openlib_mt(lua_State* L, const char* name, luaL_Reg* funcs, lua_CFunction indexer)
    {
        lua_newtable(L);                            // +1 | 0 | 1
        if (indexer) {
            lua_newtable(L);                        // +1 | 1 | 2
            lua_pushcfunction(L, indexer);          // +1 | 2 | 3
            lua_setfield(L, -2, "__index");   // -1 | 3 | 2
            lua_setmetatable(L, -2);        // -1 | 2 | 1
        }
        if (funcs) {
            luaL_setfuncs(L, funcs, 0);         //  0 | 1 | 1
        }
        lua_setglobal(L, name);                     // -1 | 1 | 0

        return 0;
    }
    static int pcall(lua_State *L, int narg = 0, int nres = 0) {
        lua_getglobal(L, "debug");
        lua_getfield(L, -1, "traceback");
        lua_remove(L, -2);
        int errindex = -narg - 2;
        lua_insert(L, errindex);
        auto status = lua_pcall(L, narg, nres, errindex);
        lua_remove(L, errindex);
        return status;
    }
#if LUA_VERSION_NUM < 502
    static void* lua_newuserdatauv(lua_State *L, size_t sz, int nuvalue)
    {
        return lua_newuserdata(L, sz);
    }
    static int lua_getiuservalue(lua_State *L, int idx, int nuvalue)
    {
        return lua_setuservalue(L, idx);
    }
    static int lua_newuserdatauv(lua_State *L, int sz, int nuvalue)
    {
        return lua_getuservalue(L, idx);
    }
    inline int lua_absindex (lua_State* L, int idx)
    {
        if (idx > LUA_REGISTRYINDEX && idx < 0)
            return lua_gettop (L) + idx + 1;
        else
            return idx;
    }

    inline void lua_rawgetp (lua_State* L, int idx, void const* p)
    {
        idx = lua_absindex (L, idx);
        lua_pushlightuserdata (L, const_cast <void*> (p));
        lua_rawget (L,idx);
    }

    inline void lua_rawsetp (lua_State* L, int idx, void const* p)
    {
        idx = lua_absindex (L, idx);
        lua_pushlightuserdata (L, const_cast <void*> (p));
        // put key behind value
        lua_insert (L, -2);
        lua_rawset (L, idx);
    }

    static int luaL_gettable(lua_State *L, int idx) {
        lua_gettable(L, idx);
        return lua_type(L, -1);
    }
    static int luaL_getfield(lua_State *L, int idx, const char* name) {
        lua_getfield(L, idx, name);
        return lua_type(L, -1);
    }
#else
    static int luaL_gettable(lua_State *L, int idx) {
        return lua_gettable(L, idx);
    }
    static int luaL_getfield(lua_State *L, int idx, const char* name) {
        return lua_getfield(L, idx, name);
    }
#endif
}

#include "LuaBinding/Object.h"
#include "LuaBinding/Class.h"
#include "LuaBinding/State.h"
#include "LuaBinding/Traits.h"
#include "LuaBinding/Stack.h"
