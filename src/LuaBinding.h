#pragma once

#include <type_traits>
#include <stdexcept>

#if LUA_VERSION_NUM < 502
#define LUA_OPEQ 1
#define LUA_OPLT 2
#define LUA_OPLE 3
    inline int lua_absindex (lua_State* L, int idx)
    {
        if (idx > LUA_REGISTRYINDEX && idx < 0)
            return lua_gettop (L) + idx + 1;
        else
            return idx;
    }
    inline const char* luaL_tolstring (lua_State* L, int idx, size_t* len) {
        if (lua_isstring(L, idx))
            return lua_tolstring(L, idx, len);
        idx = lua_absindex(L, idx);
        lua_getglobal(L, "tostring");
        lua_pushvalue(L, idx);
        lua_call(L, 1, 1);
        auto result = lua_tolstring(L, -1, len);
        lua_pop(L, 1);
        return result;
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
    inline int lua_getlen(lua_State* L, int idx)
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
    inline unsigned int lua_touinteger(lua_State* L, int idx)
    {
        return (unsigned int)lua_tonumber(L, idx);
    }
#else
    inline int lua_getlen(lua_State* L, int idx)
    {
        return (int)lua_rawlen(L, idx);
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

namespace LuaBinding {
    namespace ResolveDetail {
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
    }

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
        return ResolveDetail::_resolve::resolve_v(std::is_member_object_pointer<Sig C::*>(), mem_ptr);
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
    static int tack_on_traceback(lua_State* L) {
        const char * msg = lua_tostring(L, -1);
        luaL_traceback(L, L, msg, 2);
        lua_remove(L, -2);
        return 1;
    }
    static int pcall(lua_State *L, int narg = 0, int nres = 0) {
        int errindex = lua_gettop(L) - narg;
        lua_pushcclosure(L, tack_on_traceback, 0);
        lua_insert(L, errindex);
        auto status = lua_pcall(L, narg, nres, errindex);
        lua_remove(L, errindex);
        return status;
    }
    static void TableDump(lua_State *L, int idx, decltype(printf) printfun, const char* tabs = "")
    {
        idx = lua_absindex(L, idx);
        lua_pushnil(L);
        while (lua_next(L, idx) != 0)
        {
            int t = lua_type(L, -1);
            switch (t) {
                case LUA_TTABLE:
                    if (lua_type(L, -2) == LUA_TNUMBER)
                        printfun("%s[%d] = {\n", tabs, lua_tointeger(L, -2));
                    else
                        printfun("%s[%s] = {\n", tabs, lua_tostring(L, -2));
                    if (strlen(tabs) < 4)
                        TableDump(L, -1, printfun, (std::string(tabs) + "\t").c_str());
                    printfun("%s}\n", tabs);
                    break;
                case LUA_TSTRING:  /* strings */
                    if (lua_type(L, -2) == LUA_TNUMBER)
                        printfun("%s[%d] = '%s'\n", tabs, lua_tointeger(L, -2), lua_tostring(L, -1));
                    else
                        printfun("%s[%s] = '%s'\n", tabs, lua_tostring(L, -2), lua_tostring(L, -1));
                    break;
                case LUA_TBOOLEAN:  /* booleans */
                    if (lua_type(L, -2) == LUA_TNUMBER)
                        printfun("%s[%d] = %s\n", tabs, lua_tointeger(L, -2), lua_toboolean(L, -1) ? "true" : "false");
                    else
                        printfun("%s[%s] = %s\n", tabs, lua_tostring(L, -2), lua_toboolean(L, -1) ? "true" : "false");
                    break;
                case LUA_TNUMBER:  /* numbers */
                    if (lua_type(L, -2) == LUA_TNUMBER)
                        printfun("%s[%d] = %g\n", tabs, lua_tointeger(L, -2), lua_tonumber(L, -1));
                    else
                        printfun("%s[%s] = %g\n", tabs, lua_tostring(L, -2), lua_tonumber(L, -1));
                    break;
                default:  /* other values */
                    if (lua_type(L, -2) == LUA_TNUMBER)
                        printfun("%s[%d] = %s\n", tabs, lua_tointeger(L, -2), lua_typename(L, lua_type(L, -1)));
                    else
                        printfun("%s[%s] = %s\n", tabs, lua_tostring(L, -2), lua_typename(L, lua_type(L, -1)));
                    break;
            }
            lua_pop(L, 1);
        }
    }
    static void StackDump(lua_State *L, decltype(printf) printfun)
    {
        int i;
        int top = lua_gettop(L);
        for (i = 1; i <= top; i++) {  /* repeat for each level */
            int t = lua_type(L, i);
            switch (t) {
                case LUA_TTABLE:  /* table */
                    printfun("[%d] = {\n", i);
                    TableDump(L, i, printfun, "\t");
                    printfun("}\n");
                    break;
                case LUA_TSTRING:  /* strings */
                    printfun("[%d] = '%s'\n", i, lua_tostring(L, i));
                    break;
                case LUA_TBOOLEAN:  /* booleans */
                    printfun("[%d] = %s\n", i, lua_toboolean(L, i) ? "true" : "false");
                    break;
                case LUA_TNUMBER:  /* numbers */
                    if (lua_tonumber(L, i) > 0x100000 && lua_tonumber(L, i) < 0xffffffff)
                        printfun("[%d] = %X\n", i, (uint32_t)lua_tonumber(L, i));
                    else
                        printfun("[%d] = %g\n", i, lua_tonumber(L, i));
                    break;
                case LUA_TUSERDATA:  /* numbers */
                    printfun("[%d] = %s %llX\n", i, lua_typename(L, t), (uintptr_t)lua_touserdata(L, i));
                    break;
                default:  /* other values */
                    printfun("[%d] = %s\n", i, lua_typename(L, t));
                    break;
            }
        }
    }
    static void StackTrace(lua_State* L, decltype(printf) printfun)
    {
        luaL_traceback(L, L, NULL, 1);
        const char* lerr = lua_tostring(L, -1); // -0
        if (strlen(lerr) > 0) {
            printfun(lerr);
        }
        lua_pop(L, 1);
    }
}

#include "ClassPreDefs.h"
#include "Object.h"
#include "Environment.h"
#include "State.h"
#include "Traits.h"
#include "Stack.h"
#include "Class.h"
#include "MemoryTypes.h"
#include "Function.h"

namespace LuaBinding {
    static Object Nil = Object();
    static ObjectRef NilRef = ObjectRef();
}
