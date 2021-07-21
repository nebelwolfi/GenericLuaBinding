#pragma once

#include "lua.hpp"
#include <type_traits>

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
        //lua_getglobal(L, "debug");
        //lua_getfield(L, -1, "traceback");
        //lua_remove(L, -2);
        //int errindex = -narg - 2;
        //lua_insert(L, errindex);
        auto status = lua_pcall(L, narg, nres, 0);
        //luaL_traceback(L, L, NULL, 1);
        //lua_remove(L, errindex);
        return status;
    }
}

#if LUA_VERSION_NUM < 502
#define luaL_tolstring lua_tolstring
#define LUA_OPEQ 1
#define LUA_OPLT 2
#define LUA_OPLE 3
    inline void* lua_newuserdatauv(lua_State *L, size_t sz, int nuvalue)
    {
        return lua_newuserdata(L, sz);
    }
    inline int lua_absindex (lua_State* L, int idx)
    {
        if (idx > LUA_REGISTRYINDEX && idx < 0)
            return lua_gettop (L) + idx + 1;
        else
            return idx;
    }
    inline int lua_rawgetp (lua_State* L, int idx, void const* p)
    {
        idx = lua_absindex (L, idx);
        lua_pushlightuserdata (L, const_cast <void*> (p));
        lua_rawget (L,idx);
        return lua_type(L, -1);
    }
    inline void lua_rawsetp (lua_State* L, int idx, void const* p)
    {
        idx = lua_absindex (L, idx);
        lua_pushlightuserdata (L, const_cast <void*> (p));
        // put key behind value
        lua_insert (L, -2);
        lua_rawset (L, idx);
    }
    inline int luaL_gettable(lua_State *L, int idx) {
        lua_gettable(L, idx);
        return lua_type(L, -1);
    }
    inline int luaL_getfield(lua_State *L, int idx, const char* name) {
        lua_getfield(L, idx, name);
        return lua_type(L, -1);
    }
    inline int lua_compare(lua_State* L, int idx1, int idx2, int op)
    {
        switch (op)
        {
            case LUA_OPEQ:
                return lua_equal(L, idx1, idx2);
            case LUA_OPLT:
                return lua_lessthan(L, idx1, idx2);
            case LUA_OPLE:
                return lua_equal(L, idx1, idx2) || lua_lessthan(L, idx1, idx2);
            default:
                return 0;
        };
    }
    inline int get_len(lua_State* L, int idx)
    {
        return int(lua_objlen(L, idx));
    }
    inline void lua_geti(lua_State *L, int idx, lua_Integer n) {
        lua_pushinteger(L, n);
        lua_gettable(L, idx);
    }
    inline int lua_isinteger(lua_State *L, int idx) {
        return lua_isnumber(L, idx);
    }

    inline int luaL_typeerror( lua_State* L, int arg, const char* tname )
    {
        const char* msg;
        const char* typearg; /* name for the type of the actual argument */
        if ( luaL_getmetafield( L, arg, "__name" ) == LUA_TSTRING )
            typearg = lua_tostring( L, -1 ); /* use the given type name */
        else if ( lua_type( L, arg ) == LUA_TLIGHTUSERDATA )
            typearg = "light userdata"; /* special name for messages */
        else
            typearg = luaL_typename( L, arg ); /* standard name */
        msg = lua_pushfstring( L, "%s expected, got %s", tname, typearg );
        return luaL_argerror( L, arg, msg );
    }
    inline int luaL_rawget(lua_State* L, int idx)
    {
        lua_rawget(L, idx);
        return lua_type(L, -1);
    }
    inline int luaL_getglobal(lua_State*L, const char* name)
    {
        lua_getglobal(L, name);
        return lua_type(L, -1);
    }
#else
    inline int get_len(lua_State* L, int idx)
    {
        lua_len(L, idx);
        auto len = (int)lua_tointeger(L, -1);
        lua_pop(L, 1);
        return len;
    }
    inline int luaL_gettable(lua_State *L, int idx) {
        return lua_gettable(L, idx);
    }
    inline int luaL_getfield(lua_State *L, int idx, const char* name) {
        return lua_getfield(L, idx, name);
    }
    inline int luaL_rawget(lua_State* L, int idx)
    {
        return lua_rawget(L, idx);
    }
    inline int luaL_getglobal(lua_State*L, const char* name)
    {
        return lua_getglobal(L, name);
    }
#endif

#include "Object.h"
#include "Class.h"
#include "State.h"
#include "Traits.h"
#include "Stack.h"
#include "MemoryTypes.h"

namespace LuaBinding {
    static Object Nil = Object();
    static ObjectRef NilRef = ObjectRef();
}
