#pragma once


// Check windows
#if _WIN32 || _WIN64
#if _WIN64
#define ENV64
#else
#define ENV32
#endif
#endif

// Check GCC
#if __GNUC__
#if __x86_64__ || __ppc64__
#define ENV64
#else
#define ENV32
#endif
#endif

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



namespace LuaBinding {
    template<typename C, typename ...Functions>
    class OverloadedFunction;
    template<typename T>
    class Class;
    class DynClass;
    class Object;
    class ObjectRef;
    class Environment;
    class State;
    class IndexProxy;
    template<typename T>
    class Stack;
    template<typename T>
    class StackClass;
    template<typename T>
    class helper;



    template <class R, class... Params>
    class Traits;
    template <class T, class... Params>
    class TraitsCtor;
    template <class R, class... Params>
    class TraitsSTDFunction;
    class TraitsCFunc;
    template <class R, class T, class... Params>
    class TraitsClass;
    template <class R, class T, class... Params>
    class TraitsFunClass;
    template <class R, class T, class... Params>
    class TraitsNClass;
    template <class T>
    class TraitsClassNCFunc;
    template <class T>
    class TraitsClassFunNCFunc;
    template <class T>
    class TraitsClassCFunc;
    template <class T>
    class TraitsClassFunCFunc;
    template <class T>
    class TraitsClassLCFunc;
    template <class T>
    class TraitsClassFunLCFunc;
    template <class T>
    class TraitsClassFunNLCFunc;
    template <class R, class T>
    class TraitsClassProperty;
    template <class R, class T>
    class TraitsClassPropertyNFn;
    template <class R, class T>
    class TraitsClassPropertyFn;
    template <class R, class T>
    class TraitsClassPropertyNFunFn;
    template <class R, class T>
    class TraitsClassPropertyFunFn;
    template <class R, class T>
    class TraitsClassPropertyNFunCFn;
    template <class R, class T>
    class TraitsClassPropertyNFunLCFn;
    template <class R, class T>
    class TraitsClassPropertyFunLCFn;
    template <class R, class T>
    class TraitsClassPropertyNFunC;
    template <class R, class T>
    class TraitsClassPropertyFunC;
    template <class R, class T>
    class TraitsClassPropertyNCFun;
    template <class R, class T>
    class TraitsClassPropertyCFun;
}



#include <map>
#include <optional>
#include <vector>
#include <set>
#include <list>
#include <map>
#include <queue>
#include <stack>
#include <deque>
#include <unordered_set>
#include <unordered_map>

namespace LuaBinding {
    namespace detail {
        template<typename T>
        concept is_pushable = requires(T a) {
            { LuaBinding::Stack<std::decay_t<T>>::is(nullptr, 0) };
        };

        template<class T> requires is_pushable<T>
        T get(lua_State* L, int index)
        {
            return Stack<T>::get(L, index);
        }

        template<class T> requires (!is_pushable<T>)
        T get(lua_State* L, int index)
        {
            return StackClass<T>::get(L, index);
        }

        template<class T> requires is_pushable<T>
        T extract(lua_State* L, int index)
        {
            auto r = Stack<T>::get(L, index);
            lua_remove(L, index);
            return r;
        }

        template<class T> requires (!is_pushable<T>)
        T extract(lua_State* L, int index)
        {
            auto r = StackClass<T>::get(L, index);
            lua_remove(L, index);
            return r;
        }

        template<class T> requires is_pushable<T>
        int push(lua_State* L, T&& t)
        {
            return Stack<T>::push(L, t);
        }

        template<class T> requires is_pushable<T>
        int push(lua_State* L, T& t)
        {
            return Stack<T>::push(L, t);
        }

        template<class T> requires (!is_pushable<T> && !std::is_same_v<T, nullptr_t>)
        int push(lua_State*L, T& t)
        {
            return StackClass<T>::push(L, t);
        }

        template<class T> requires (!is_pushable<T> && !std::is_same_v<T, nullptr_t>)
        int push(lua_State*L, T&& t)
        {
            return StackClass<T>::push(L, t);
        }

        template<class T> requires (!is_pushable<T> && std::is_same_v<T, nullptr_t>)
        int push(lua_State* L, T& t)
        {
            lua_pushnil(L);
            return 1;
        }

        template<class T> requires (!is_pushable<T> && std::is_same_v<T, nullptr_t>)
        int push(lua_State* L, T&& t)
        {
            lua_pushnil(L);
            return 1;
        }

        template<class T> requires is_pushable<T>
        bool is(lua_State* L, int index)
        {
            return Stack<T>::is(L, index);
        }

        template<class T> requires (!is_pushable<T>)
        bool is(lua_State* L, int index)
        {
            return StackClass<T>::is(L, index);
        }

        template<class T> requires is_pushable<T>
        const char* basic_type_name(lua_State* L)
        {
            return Stack<T>::basic_type_name(L);
        }

        template<class T> requires (!is_pushable<T>)
        const char* basic_type_name(lua_State* L)
        {
            return "userdata";
        }

        template<class T> requires is_pushable<T>
        int basic_type(lua_State* L)
        {
            return Stack<T>::basic_type(L);
        }

        template<class T> requires (!is_pushable<T>)
        int basic_type(lua_State* L)
        {
            return LUA_TUSERDATA;
        }
    }

    template<typename T>
    class Stack {
    public:
        static int push(lua_State* L, T t) {
            static_assert(sizeof(T) == 0, "Unspecialized use of Stack<T>::push");
        }
        static bool is(lua_State* L, int index) = delete;
        static T get(lua_State* L, int index) {
            static_assert(sizeof(T) == 0, "Unspecialized use of Stack<T>::get");
        }
        static const char* type_name(lua_State* L) {
            return "";
        }
        static const char* basic_type_name(lua_State* L) {
            return "";
        }
        static int basic_type(lua_State* L) {
            return LUA_TNONE;
        }
    };

    template<typename T>
    class StackClass {
    public:
        static int push(lua_State* L, T& t) requires (!std::is_convertible_v<T, void*>)
        {
            auto _t = new (malloc(sizeof(T))) T(t);
            auto u = (void**)lua_newuserdata(L, sizeof(void*) * 2);
            *u = _t; *(u + 1) = (void*)0xC0FFEE;
            helper<std::remove_pointer_t<std::decay_t<T>>>::push_metatable(L);
            lua_setmetatable(L, -2);
            return 1;
        }
        static int push(lua_State* L, T&& t) requires (!std::is_convertible_v<T, void*>)
        {
            auto _t = new (malloc(sizeof(T))) T(t);
            auto u = (void**)lua_newuserdata(L, sizeof(void*) * 2);
            *u = _t; *(u + 1) = (void*)0xC0FFEE;
            helper<std::remove_pointer_t<std::decay_t<T>>>::push_metatable(L);
            lua_setmetatable(L, -2);
            return 1;
        }
        static int push(lua_State* L, T& t) requires std::is_convertible_v<T, void*>
        {
            if (t == nullptr)
            {
                lua_pushnil(L);
                return 1;
            }
            auto u = (void**)lua_newuserdata(L, sizeof(void*) * 2);
            *u = (void*)t; *(u + 1) = 0;

            helper<std::remove_pointer_t<std::decay_t<T>>>::push_metatable(L);
            lua_setmetatable(L, -2);
            return 1;
        }
        static int push(lua_State* L, T&& t) requires std::is_convertible_v<T, void*>
        {
            if (t == nullptr)
            {
                lua_pushnil(L);
                return 1;
            }
            auto u = (void**)lua_newuserdata(L, sizeof(void*) * 2);
            *u = (void*)t; *(u + 1) = 0;
            helper<std::remove_pointer_t<std::decay_t<T>>>::push_metatable(L);
            lua_setmetatable(L, -2);
            return 1;
        }
        static bool is(lua_State* L, int index) requires std::is_same_v<T, void*> {
#ifdef FLUENT_USERDATA_CHECKS
            if (!lua_isuserdata(L, index) && !lua_isnumber(L, index)) return false;
#else
            if (!lua_isuserdata(L, index)) return false;
#endif
            return true;
        }
        static bool is(lua_State* L, int index) requires (!std::is_same_v<T, void*>) {
            if (!lua_isuserdata(L, index)) {
#ifdef FLUENT_USERDATA_CHECKS
                if constexpr (std::is_convertible_v<T, uintptr_t>)
                    if (lua_isnumber(L, index))
                        return true;
#endif
                return false;
            }
            lua_getmetatable(L, index);
            helper<std::remove_pointer_t<std::decay_t<T>>>::push_metatable(L);
            auto res = lua_compare(L, -1, -2, LUA_OPEQ);
            lua_pop(L, 2);
            return res == 1;
        }
        static const char* type_name(lua_State* L) {
            helper<std::remove_pointer_t<std::decay_t<T>>>::push_metatable(L);
            if (luaL_getfield(L, -1, "__name"))
            {
                auto name = lua_tostring(L, -1);
                lua_pop(L, 2);
                return name;
            }
            lua_pop(L, 2);
            return typeid(T).name();
        }
        static const char* basic_type_name(lua_State* L) {
            return "userdata";
        }
        static int basic_type(lua_State* L) {
            return LUA_TUSERDATA;
        }
        static T get(lua_State* L, int index) requires (std::is_convertible_v<T, void*>)
        {
            if (!is(L, index))
            {
                if constexpr (std::is_convertible_v<T, void*>)
                    if (lua_isnoneornil(L, index))
                        return nullptr;
                luaL_typeerror(L, index, type_name(L));
            }
#ifdef FLUENT_USERDATA_CHECKS
            if (lua_isnumber(L, index))
                return (T)(uintptr_t)lua_tonumber(L, index);
#endif
            return *(T*)lua_touserdata(L, index);
        }
        static T get(lua_State* L, int index) requires (!std::is_convertible_v<T, void*>)
        {
            if (!is(L, index))
            {
                luaL_typeerror(L, index, type_name(L));
            }
#ifdef FLUENT_USERDATA_CHECKS
            if constexpr (std::is_convertible_v<T, uintptr_t>)
                if (lua_isnumber(L, index))
                    return (T)(uintptr_t)lua_tonumber(L, index);
#endif
            return **(T**)lua_touserdata(L, index);
        }
    };

    template<>
    class Stack<int> {
    public:
        static int push(lua_State* L, int t)
        {
            lua_pushnumber(L, (double)t);
            return 1;
        }
        static bool is(lua_State* L, int index) {
            return lua_isnumber(L, index);
        }
        static int get(lua_State* L, int index)
        {
            return (int32_t)lua_tonumber(L, index);
        }
        static const char* type_name(lua_State* L) {
            return "integer";
        }
        static const char* basic_type_name(lua_State* L) {
            return "number";
        }
        static int basic_type(lua_State* L) {
            return LUA_TNUMBER;
        }
    };

    template<>
    class Stack<unsigned int> {
    public:
        static int push(lua_State* L, unsigned int t)
        {
            lua_pushnumber(L, (double)t);
            return 1;
        }
        static bool is(lua_State* L, int index) {
            return lua_isnumber(L, index);
        }
        static unsigned int get(lua_State* L, int index)
        {
            return lua_touinteger(L, index);
        }
        static const char* type_name(lua_State* L) {
            return "unsigned integer";
        }
        static const char* basic_type_name(lua_State* L) {
            return "number";
        }
        static int basic_type(lua_State* L) {
            return LUA_TNUMBER;
        }
    };

    template<>
    class Stack<void> {
    public:
        static int push(lua_State* L)
        {
            lua_pushnil(L);
            return 1;
        }
        static bool is(lua_State* L, int index) {
            return lua_isnoneornil(L, index);
        }
        static const char* type_name(lua_State* L) {
            return "nil";
        }
        static const char* basic_type_name(lua_State* L) {
            return "nil";
        }
        static int basic_type(lua_State* L) {
            return LUA_TNIL;
        }
    };

    template<class T> requires std::is_floating_point_v<T>
    class Stack<T> {
    public:
        static int push(lua_State* L, T t)
        {
            lua_pushnumber(L, t);
            return 1;
        }
        static bool is(lua_State* L, int index) {
            return lua_isnumber(L, index);
        }
        static T get(lua_State* L, int index)
        {
            return (T)lua_tonumber(L, index);
        }
        static const char* type_name(lua_State* L) {
            return "number";
        }
        static const char* basic_type_name(lua_State* L) {
            return "number";
        }
        static int basic_type(lua_State* L) {
            return LUA_TNUMBER;
        }
    };

    template<class T> requires std::is_integral_v<T>
    class Stack<T> {
    public:
        static int push(lua_State* L, T t)
        {
            lua_pushnumber(L, (double)t);
            return 1;
        }
        static bool is(lua_State* L, int index) {
            return lua_isnumber(L, index);
        }
        static T get(lua_State* L, int index)
        {
            return (T)lua_tonumber(L, index);
        }
        static const char* type_name(lua_State* L) {
            return "integer";
        }
        static const char* basic_type_name(lua_State* L) {
            return "number";
        }
        static int basic_type(lua_State* L) {
            return LUA_TNUMBER;
        }
    };

    template<class T> requires std::is_same_v<T, char*>
    class Stack<T> {
    public:
        static int push(lua_State* L, T t)
        {
            lua_pushstring(L, t);
            return 1;
        }
        static bool is(lua_State* L, int index) {
            return lua_isstring(L, index);
        }
        static T get(lua_State* L, int index)
        {
            return (T)luaL_tolstring(L, index, nullptr);
        }
        static const char* type_name(lua_State* L) {
            return "string";
        }
        static const char* basic_type_name(lua_State* L) {
            return "string";
        }
        static int basic_type(lua_State* L) {
            return LUA_TSTRING;
        }
    };

    template<>
    class Stack<bool> {
    public:
        static int push(lua_State* L, bool t)
        {
            lua_pushboolean(L, t);
            return 1;
        }
        static bool is(lua_State* L, int) {
            return true;
        }
        static bool get(lua_State* L, int index)
        {
            return lua_toboolean(L, index);
        }
        static const char* type_name(lua_State* L) {
            return "boolean";
        }
        static const char* basic_type_name(lua_State* L) {
            return "boolean";
        }
        static int basic_type(lua_State* L) {
            return LUA_TBOOLEAN;
        }
    };

    template<>
    class Stack<char*> {
    public:
        static int push(lua_State* L, char* t)
        {
            lua_pushstring(L, t);
            return 1;
        }
        static bool is(lua_State* L, int index) {
            return lua_isstring(L, index);
        }
        static char* get(lua_State* L, int index)
        {
            return (char*)luaL_tolstring(L, index, nullptr);
        }
        static const char* type_name(lua_State* L) {
            return "string";
        }
        static const char* basic_type_name(lua_State* L) {
            return "string";
        }
        static int basic_type(lua_State* L) {
            return LUA_TSTRING;
        }
    };

    template<>
    class Stack<const char*> {
    public:
        static int push(lua_State* L, const char* t)
        {
            lua_pushstring(L, t);
            return 1;
        }
        static bool is(lua_State* L, int index) {
            return lua_isstring(L, index);
        }
        static const char* get(lua_State* L, int index)
        {
            return luaL_tolstring(L, index, nullptr);
        }
        static const char* type_name(lua_State* L) {
            return "string";
        }
        static const char* basic_type_name(lua_State* L) {
            return "string";
        }
        static int basic_type(lua_State* L) {
            return LUA_TSTRING;
        }
    };

    template<>
    class Stack<std::string> {
    public:
        static int push(lua_State* L, std::string t)
        {
            lua_pushstring(L, t.c_str());
            return 1;
        }
        static bool is(lua_State* L, int index) {
            return lua_isstring(L, index);
        }
        static std::string get(lua_State* L, int index)
        {
            size_t len;
            return { luaL_tolstring(L, index, &len), len };
        }
        static const char* type_name(lua_State* L) {
            return "string";
        }
        static const char* basic_type_name(lua_State* L) {
            return "string";
        }
        static int basic_type(lua_State* L) {
            return LUA_TSTRING;
        }
    };

    template<typename T>
    class Stack<std::optional<T>> {
    public:
        static int push(lua_State* L, std::optional<T> t)
        {
            if (t.has_value())
                detail::push<T>(L, t.value());
            else
                lua_pushnil(L);
            return 1;
        }
        static bool is(lua_State* L, int index) {
            if (lua_isnil(L, index))
                return true;
            if constexpr (detail::is_pushable<T>)
            {
                return Stack<T>::is(L, index);
            }
            return StackClass<T>::is(L, index);
        }
        static std::optional<T> get(lua_State* L, int index)
        {
            if (lua_isnoneornil(L, index))
                return std::nullopt;
            return detail::get<T>(L, index);
        }
        static const char* type_name(lua_State* L) {
            if constexpr (detail::is_pushable<T>)
            {
                return Stack<T>::type_name(L);
            }
            return StackClass<T>::type_name(L);
        }
        static const char* basic_type_name(lua_State* L) {
            if constexpr (detail::is_pushable<T>)
            {
                return Stack<T>::basic_type_name(L);
            }
            return StackClass<T>::basic_type_name(L);
        }
        static int basic_type(lua_State* L) {
            if constexpr (detail::is_pushable<T>)
            {
                return Stack<T>::basic_type(L);
            }
            return StackClass<T>::basic_type(L);
        }
    };

    template<typename Param>
    int snp(lua_State *L, char* buff) {
        if constexpr (detail::is_pushable<Param>)
            snprintf(buff, 1000, strlen(buff) > 6 ? "%s, %s" : "%s%s", buff, Stack<Param>::type_name(L));
        else
            snprintf(buff, 1000, strlen(buff) > 6 ? "%s, %s" : "%s%s", buff, StackClass<Param>::type_name(L));
        return 0;
    };

    template<typename ...Params>
    class Stack<std::tuple<Params...>> {
    public:
        static int push(lua_State* L, std::tuple<Params...> t)
        {
            lua_createtable(L, sizeof...(Params), 0);
            auto tbl = lua_gettop(L);
            std::apply([L, tbl](auto &&... args) {
                (void)std::initializer_list<int>{ detail::push(L, args)... };
                for (auto i = sizeof...(Params); i > 0; i--)
                    lua_rawseti(L, tbl, i);
            }, t);
            return 1;
        }
        static bool is(lua_State* L, int index) {
            return lua_istable(L, index);
        }
        static std::tuple<Params...> get(lua_State* L, int index)
        {
            auto tbl = lua_absindex(L, index);
            auto top = lua_gettop(L);
            for (auto i = 1; i <= sizeof...(Params); i++)
            {
                lua_geti(L, tbl, i);
            }
            using ParamList = std::tuple<std::decay_t<Params>...>;
            ParamList params;
            assign_tup(L, params, top);
            lua_pop(L, sizeof...(Params));
            return params;
        }
        static const char* type_name(lua_State* L) {
            static char buff[1000] = { '\0' };
            if (buff[0]) return buff;
            snprintf(buff, 1000, "table{");
            (void)std::initializer_list<int> { snp<Params>(L, buff)... };
            snprintf(buff, 1000, "%s}", buff);
            return buff;
        }
        static const char* basic_type_name(lua_State* L) {
            return "table";
        }
        static int basic_type(lua_State* L) {
            return LUA_TTABLE;
        }
    };

    template<typename K, typename V>
    class Stack<std::pair<K, V>> {
    public:
        static int push(lua_State* L, std::pair<K, V> t)
        {
            lua_createtable(L, 2, 0);
            detail::push<K>(L, t.first);
            lua_rawseti(L, -2, 1);
            detail::push<K>(L, t.second);
            lua_rawseti(L, -2, 2);
            return 1;
        }
        static bool is(lua_State* L, int index) {
            return lua_istable(L, index);
        }
        static std::pair<K, V> get(lua_State* L, int index)
        {
            index = lua_absindex(L, index);
            lua_geti(L, index, 1);
            lua_geti(L, index, 2);
            std::pair<K, V> t = { detail::get<K>(L, -2), detail::get<V>(L, -1) };
            lua_pop(L, 2);
            return t;
        }
        static const char* type_name(lua_State* L) {
            static char buff[100] = { '\0' };
            if (buff[0]) return buff;
            if constexpr (detail::is_pushable<K>)
            {
                snprintf(buff, 100, "table{%s, ", Stack<K>::type_name(L));
            } else
                snprintf(buff, 100, "table{%s, ", StackClass<K>::type_name(L));
            if constexpr (detail::is_pushable<V>)
            {
                snprintf(buff, 100, "%s%s}", buff, Stack<V>::type_name(L));
            } else
                snprintf(buff, 100, "%s%s}", buff, StackClass<V>::type_name(L));
            return buff;
        }
        static const char* basic_type_name(lua_State* L) {
            return "table";
        }
        static int basic_type(lua_State* L) {
            return LUA_TTABLE;
        }
    };

    template<typename T>
    class Stack<std::vector<T>> {
    public:
        static int push(lua_State* L, std::vector<T> t)
        {
            lua_createtable(L, t.size(), 0);
            for (auto i = 0; i < t.size(); i++)
            {
                detail::push<T>(L, t[i]);
                lua_rawseti(L, -2, i + 1);
            }
            return 1;
        }
        static bool is(lua_State* L, int index) {
            return lua_istable(L, index);
        }
        static std::vector<T> get(lua_State* L, int index)
        {
            index = lua_absindex(L, index);
            size_t len = lua_getlen(L, index);
            std::vector<T> result;
            result.reserve(len);
            for (auto i = 1; i <= len; i++)
            {
                lua_geti(L, index, i);
                result.push_back(detail::get<T>(L, -1));
                lua_pop(L, 1);
            }
            return result;
        }
        static const char* type_name(lua_State* L) {
            static char buff[100] = { '\0' };
            if (buff[0]) return buff;
            if constexpr (detail::is_pushable<T>)
            {
                snprintf(buff, 100, "table{%s}", Stack<T>::type_name(L));
            } else
                snprintf(buff, 100, "table{%s}", StackClass<T>::type_name(L));
            return buff;
        }
        static const char* basic_type_name(lua_State* L) {
            return "table";
        }
        static int basic_type(lua_State* L) {
            return LUA_TTABLE;
        }
    };

    template<typename T>
    class Stack<std::vector<T>*> {
    public:
        static int push(lua_State* L, std::vector<T>* t)
        {
            lua_createtable(L, t->size(), 0);
            for (auto i = 0; i < t->size(); i++)
            {
                detail::push<T>(L, t->at(i));
                lua_rawseti(L, -2, i + 1);
            }
            return 1;
        }
        static bool is(lua_State* L, int index) {
            return lua_istable(L, index);
        }
        static std::vector<T>* get(lua_State* L, int index)
        {
            index = lua_absindex(L, index);
            size_t len = lua_getlen(L, index);
            auto result = new std::vector<T>();
            result->reserve(len);
            for (auto i = 1; i <= len; i++)
            {
                lua_geti(L, index, i);
                result->push_back(detail::get<T>(L, -1));
                lua_pop(L, 1);
            }
            return result;
        }
        static const char* type_name(lua_State* L) {
            static char buff[100] = { '\0' };
            if (buff[0]) return buff;
            if constexpr (detail::is_pushable<T>)
            {
                snprintf(buff, 100, "table{%s}", Stack<T>::type_name(L));
            } else
                snprintf(buff, 100, "table{%s}", StackClass<T>::type_name(L));
            return buff;
        }
        static const char* basic_type_name(lua_State* L) {
            return "table";
        }
        static int basic_type(lua_State* L) {
            return LUA_TTABLE;
        }
    };

    template<typename T>
    class Stack<std::list<T>> {
    public:
        static int push(lua_State* L, std::list<T> t)
        {
            lua_createtable(L, t.size(), 0);
            auto i = 1;
            for (auto& el : t)
            {
                detail::push<T>(L, el);
                lua_rawseti(L, -2, i++);
            }
            return 1;
        }
        static bool is(lua_State* L, int index) {
            return lua_istable(L, index);
        }
        static std::list<T> get(lua_State* L, int index)
        {
            index = lua_absindex(L, index);
            size_t len = lua_getlen(L, index);
            std::list<T> result;
            result.reserve(len);
            for (auto i = 1; i <= len; i++)
            {
                lua_geti(L, index, i);
                result.push_back(detail::get<T>(L, -1));
                lua_pop(L, 1);
            }
            return result;
        }
        static const char* type_name(lua_State* L) {
            static char buff[100] = { '\0' };
            if (buff[0]) return buff;
            if constexpr (detail::is_pushable<T>)
            {
                snprintf(buff, 100, "table{%s}", Stack<T>::type_name(L));
            } else
                snprintf(buff, 100, "table{%s}", StackClass<T>::type_name(L));
            return buff;
        }
        static const char* basic_type_name(lua_State* L) {
            return "table";
        }
        static int basic_type(lua_State* L) {
            return LUA_TTABLE;
        }
    };

    template<typename T>
    class Stack<std::stack<T>> {
    public:
        static int push(lua_State* L, std::stack<T> t)
        {
            lua_createtable(L, t.size(), 0);
            auto i = 1;
            for (auto& el : t)
            {
                detail::push<T>(L, el);
                lua_rawseti(L, -2, i++);
            }
            return 1;
        }
        static bool is(lua_State* L, int index) {
            return lua_istable(L, index);
        }
        static std::stack<T> get(lua_State* L, int index)
        {
            index = lua_absindex(L, index);
            size_t len = lua_getlen(L, index);
            std::stack<T> result;
            result.reserve(len);
            for (auto i = 1; i <= len; i++)
            {
                lua_geti(L, index, i);
                result.push_back(detail::get<T>(L, -1));
                lua_pop(L, 1);
            }
            return result;
        }
        static const char* type_name(lua_State* L) {
            static char buff[100] = { '\0' };
            if (buff[0]) return buff;
            if constexpr (detail::is_pushable<T>)
            {
                snprintf(buff, 100, "table{%s}", Stack<T>::type_name(L));
            } else
                snprintf(buff, 100, "table{%s}", StackClass<T>::type_name(L));
            return buff;
        }
        static const char* basic_type_name(lua_State* L) {
            return "table";
        }
        static int basic_type(lua_State* L) {
            return LUA_TTABLE;
        }
    };

    template<typename T>
    class Stack<std::queue<T>> {
    public:
        static int push(lua_State* L, std::queue<T> t)
        {
            lua_createtable(L, t.size(), 0);
            auto i = 1;
            for (auto& el : t)
            {
                detail::push<T>(L, el);
                lua_rawseti(L, -2, i++);
            }
            return 1;
        }
        static bool is(lua_State* L, int index) {
            return lua_istable(L, index);
        }
        static std::queue<T> get(lua_State* L, int index)
        {
            index = lua_absindex(L, index);
            size_t len = lua_getlen(L, index);
            std::queue<T> result;
            result.reserve(len);
            for (auto i = 1; i <= len; i++)
            {
                lua_geti(L, index, i);
                result.push_back(detail::get<T>(L, -1));
                lua_pop(L, 1);
            }
            return result;
        }
        static const char* type_name(lua_State* L) {
            static char buff[100] = { '\0' };
            if (buff[0]) return buff;
            if constexpr (detail::is_pushable<T>)
            {
                snprintf(buff, 100, "table{%s}", Stack<T>::type_name(L));
            } else
                snprintf(buff, 100, "table{%s}", StackClass<T>::type_name(L));
            return buff;
        }
        static const char* basic_type_name(lua_State* L) {
            return "table";
        }
        static int basic_type(lua_State* L) {
            return LUA_TTABLE;
        }
    };

    template<typename T>
    class Stack<std::deque<T>> {
    public:
        static int push(lua_State* L, std::deque<T> t)
        {
            lua_createtable(L, t.size(), 0);
            auto i = 1;
            for (auto& el : t)
            {
                detail::push<T>(L, el);
                lua_rawseti(L, -2, i++);
            }
            return 1;
        }
        static bool is(lua_State* L, int index) {
            return lua_istable(L, index);
        }
        static std::deque<T> get(lua_State* L, int index)
        {
            index = lua_absindex(L, index);
            size_t len = lua_getlen(L, index);
            std::deque<T> result;
            result.reserve(len);
            for (auto i = 1; i <= len; i++)
            {
                lua_geti(L, index, i);
                result.push_back(detail::get<T>(L, -1));
                lua_pop(L, 1);
            }
            return result;
        }
        static const char* type_name(lua_State* L) {
            static char buff[100] = { '\0' };
            if (buff[0]) return buff;
            if constexpr (detail::is_pushable<T>)
            {
                snprintf(buff, 100, "table{%s}", Stack<T>::type_name(L));
            } else
                snprintf(buff, 100, "table{%s}", StackClass<T>::type_name(L));
            return buff;
        }
        static const char* basic_type_name(lua_State* L) {
            return "table";
        }
        static int basic_type(lua_State* L) {
            return LUA_TTABLE;
        }
    };

    template<typename T>
    class Stack<std::set<T>> {
    public:
        static int push(lua_State* L, std::set<T> t)
        {
            lua_createtable(L, t.size(), 0);
            auto i = 1;
            for (auto& el : t)
            {
                detail::push<T>(L, el);
                lua_rawseti(L, -2, i++);
            }
            return 1;
        }
        static bool is(lua_State* L, int index) {
            return lua_istable(L, index);
        }
        static std::set<T> get(lua_State* L, int index)
        {
            index = lua_absindex(L, index);
            size_t len = lua_getlen(L, index);
            std::set<T> result;
            result.reserve(len);
            for (auto i = 1; i <= len; i++)
            {
                lua_geti(L, index, i);
                result.push_back(detail::get<T>(L, -1));
                lua_pop(L, 1);
            }
            return result;
        }
        static const char* type_name(lua_State* L) {
            static char buff[100] = { '\0' };
            if (buff[0]) return buff;
            if constexpr (detail::is_pushable<T>)
            {
                snprintf(buff, 100, "table{%s}", Stack<T>::type_name(L));
            } else
                snprintf(buff, 100, "table{%s}", StackClass<T>::type_name(L));
            return buff;
        }
        static const char* basic_type_name(lua_State* L) {
            return "table";
        }
        static int basic_type(lua_State* L) {
            return LUA_TTABLE;
        }
    };

    template<typename T>
    class Stack<std::unordered_set<T>> {
    public:
        static int push(lua_State* L, std::unordered_set<T> t)
        {
            lua_createtable(L, t.size(), 0);
            auto i = 1;
            for (auto el : t)
            {
                detail::push<std::decay_t<T>>(L, el);
                lua_rawseti(L, -2, i++);
            }
            return 1;
        }
        static bool is(lua_State* L, int index) {
            return lua_istable(L, index);
        }
        static std::unordered_set<T> get(lua_State* L, int index)
        {
            index = lua_absindex(L, index);
            size_t len = lua_getlen(L, index);
            std::unordered_set<T> result;
            result.reserve(len);
            for (auto i = 1; i <= len; i++)
            {
                lua_geti(L, index, i);
                result.insert(detail::get<T>(L, -1));
                lua_pop(L, 1);
            }
            return result;
        }
        static const char* type_name(lua_State* L) {
            static char buff[100] = { '\0' };
            if (buff[0]) return buff;
            if constexpr (detail::is_pushable<T>)
            {
                snprintf(buff, 100, "table{%s}", Stack<T>::type_name(L));
            } else
                snprintf(buff, 100, "table{%s}", StackClass<T>::type_name(L));
            return buff;
        }
        static const char* basic_type_name(lua_State* L) {
            return "table";
        }
        static int basic_type(lua_State* L) {
            return LUA_TTABLE;
        }
    };

    template <class T, size_t size>
    class Stack<std::array <T, size>> {
    public:
        static int push(lua_State* L, std::array <T, size> t)
        {
            lua_createtable(L, size, 0);
            for (auto i = 0; i < size; i++)
            {
                detail::push<T>(L, t[i]);
                lua_rawseti(L, -2, i);
            }
            return 1;
        }
        static bool is(lua_State* L, int index) {
            return lua_istable(L, index);
        }
        static std::array <T, size> get(lua_State* L, int index)
        {
            index = lua_absindex(L, index);
            std::array <T, size> result;
            for (auto i = 0; i < size; i++)
            {
                lua_geti(L, index, i);
                result[i] = detail::get<T>(L, -1);
                lua_pop(L, 1);
            }
            return result;
        }
        static const char* type_name(lua_State* L) {
            static char buff[100] = { '\0' };
            if (buff[0]) return buff;
            if constexpr (detail::is_pushable<T>)
            {
                snprintf(buff, 100, "table{%s}", Stack<T>::type_name(L));
            } else
                snprintf(buff, 100, "table{%s}", StackClass<T>::type_name(L));
            return buff;
        }
        static const char* basic_type_name(lua_State* L) {
            return "table";
        }
        static int basic_type(lua_State* L) {
            return LUA_TTABLE;
        }
    };

    template<typename K, typename V>
    class Stack<std::map<K, V>> {
    public:
        static int push(lua_State* L, std::map<K, V> t)
        {
            lua_createtable(L, 0, t.size());
            for (auto& el : t)
            {
                detail::push<std::decay_t<K>>(L, (std::decay_t<K>)el.first);
                detail::push<V>(L, el.second);
                lua_settable(L, -3);
            }
            return 1;
        }
        static bool is(lua_State* L, int index) {
            return lua_istable(L, index);
        }
        static std::map<K, V> get(lua_State* L, int index)
        {
            index = lua_absindex(L, index);
            std::map<K, V> result;
            lua_pushnil(L);
            while (lua_next(L, index))
            {
                result[detail::get<std::decay_t<K>>(L, -2)] = detail::get<std::decay_t<V>>(L, -1);
                lua_pop(L, 1);
            }
            return result;
        }
        static const char* type_name(lua_State* L) {
            static char buff[100] = { '\0' };
            if (buff[0]) return buff;
            if constexpr (detail::is_pushable<K>)
            {
                snprintf(buff, 100, "table{[%s] = ", Stack<K>::type_name(L));
            } else
                snprintf(buff, 100, "table{[%s] = ", StackClass<K>::type_name(L));
            if constexpr (detail::is_pushable<V>)
            {
                snprintf(buff, 100, "%s%s}", buff, Stack<V>::type_name(L));
            } else
                snprintf(buff, 100, "%s%s}", buff, StackClass<V>::type_name(L));
            return buff;
        }
        static const char* basic_type_name(lua_State* L) {
            return "table";
        }
        static int basic_type(lua_State* L) {
            return LUA_TTABLE;
        }
    };

    template<typename K, typename V>
    class Stack<std::unordered_map<K, V>> {
    public:
        static int push(lua_State* L, std::unordered_map<K, V> t)
        {
            lua_createtable(L, 0, t.size());
            for (auto& el : t)
            {
                detail::push<std::decay_t<K>>(L, (std::decay_t<K>)el.first);
                detail::push<V>(L, el.second);
                lua_settable(L, -3);
            }
            return 1;
        }
        static bool is(lua_State* L, int index) {
            return lua_istable(L, index);
        }
        static std::unordered_map<K, V> get(lua_State* L, int index)
        {
            index = lua_absindex(L, index);
            std::unordered_map<K, V> result;
            lua_pushnil(L);
            while (lua_next(L, index))
            {
                result[detail::get<std::decay_t<K>>(L, -2)] = detail::get<std::decay_t<V>>(L, -1);
                lua_pop(L, 1);
            }
            return result;
        }
        static const char* type_name(lua_State* L) {
            static char buff[100] = { '\0' };
            if (buff[0]) return buff;
            if constexpr (detail::is_pushable<K>)
            {
                snprintf(buff, 100, "table{[%s] = ", Stack<K>::type_name(L));
            } else
                snprintf(buff, 100, "table{[%s] = ", StackClass<K>::type_name(L));
            if constexpr (detail::is_pushable<V>)
            {
                snprintf(buff, 100, "%s%s}", buff, Stack<V>::type_name(L));
            } else
                snprintf(buff, 100, "%s%s}", buff, StackClass<V>::type_name(L));
            return buff;
        }
        static const char* basic_type_name(lua_State* L) {
            return "table";
        }
        static int basic_type(lua_State* L) {
            return LUA_TTABLE;
        }
    };
}



#include <functional>
#include <tuple>

namespace LuaBinding {
    namespace FunctionDetail
    {
        template <typename F>
        struct function_traits : public function_traits<decltype(&F::operator())> {};

        template <typename R, typename C, typename... Args>
        struct function_traits<R (C::*)(Args...) const>
        {
            using function_type = std::function<R (Args...)>;
        };

        template <typename R, typename C, typename... Args>
        struct function_traits<R (C::*)(Args...)>
        {
            using function_type = std::function<R (Args...)>;
        };
    }

    template <typename F>
    using function_type_t = typename FunctionDetail::function_traits<F>::function_type;

    namespace Function {
        template <class T, class R, class... Params>
        void fun(lua_State *L, R(* func)(Params...))
        {
            using FnType = decltype (func);
            new (lua_newuserdata(L, sizeof(func))) FnType(func);
            lua_pushcclosure(L, TraitsNClass<R, T, Params...>::f, 1);
        }

        template <class T, class R, class... Params>
        void fun(lua_State *L, R(T::* func)(Params...))
        {
            using FnType = decltype (func);
            new (lua_newuserdata(L, sizeof(func))) FnType(func);
            lua_pushcclosure(L, TraitsClass<R, T, Params...>::f, 1);
        }

        template <class T, class R, class... Params>
        void fun(lua_State *L, R(T::* func)(Params...) const)
        {
            using FnType = std::decay_t<decltype (func)>;
            new (lua_newuserdata(L, sizeof(func))) FnType(func);
            lua_pushcclosure(L, TraitsClass<R, T, Params...>::f, 1);
        }

        template <class T, class R, class... Params>
        void fun(lua_State *L, std::function<R(Params...)> func)
        {
            using FnType = decltype (func);
            new (lua_newuserdata(L, sizeof(func))) FnType(func);
            lua_pushcclosure(L, TraitsFunClass<R, T, Params...>::f, 1);
        }

        template <class T, class R, class... Params>
        void fun(lua_State *L, std::function<R(Params...) const> func)
        {
            using FnType = std::decay_t<decltype (func)>;
            new (lua_newuserdata(L, sizeof(func))) FnType(func);
            lua_pushcclosure(L, TraitsFunClass<R, T, Params...>::f, 1);
        }

        template <class T, class R> requires std::is_integral_v<R>
        void cfun(lua_State *L, R(T::*func)(lua_State*))
        {
            using func_t = decltype (func);
            new (lua_newuserdata(L, sizeof(func))) func_t(func);
            lua_pushcclosure(L, TraitsClassLCFunc<T>::f, 1);
        }

        template <class T, class R> requires std::is_integral_v<R>
        void cfun(lua_State *L, std::function<R(T*, lua_State*)> func)
        {
            using FnType = decltype (func);
            new (lua_newuserdata(L, sizeof(func))) FnType(func);
            lua_pushcclosure(L, TraitsClassFunLCFunc<T>::f, 1);
        }

        template <class T, class R> requires std::is_integral_v<R>
        void cfun(lua_State *L, R(*func)(lua_State*))
        {
            lua_pushcclosure(L, func, 0);
        }

        template <class T, class R> requires std::is_integral_v<R>
        void cfun(lua_State *L, std::function<R(lua_State*)> func)
        {
            using FnType = decltype (func);
            new (lua_newuserdata(L, sizeof(func))) FnType(func);
            lua_pushcclosure(L, TraitsClassFunNLCFunc<T>::f, 1);
        }

        template <class T, class R, class U> requires std::is_integral_v<R>
        void cfun(lua_State *L, R(T::*func)(U))
        {
            using func_t = decltype (func);
            new (lua_newuserdata(L, sizeof(func))) func_t(func);
            lua_pushcclosure(L, TraitsClassCFunc<T>::f, 1);
        }

        template <class T, class R, class U> requires std::is_integral_v<R>
        void cfun(lua_State *L, R(*func)(U))
        {
            lua_pushlightuserdata(L, reinterpret_cast<void*>(func));
            lua_pushcclosure(L, TraitsClassNCFunc<T>::f, 1);
        }

        template <class T, class R, class U> requires std::is_integral_v<R>
        void cfun(lua_State *L, std::function<R(U)> func)
        {
            using FnType = decltype (func);
            new (lua_newuserdata(L, sizeof(func))) FnType(func);
            lua_pushcclosure(L, TraitsClassFunNCFunc<T>::f, 1);
        }

        template <class T, class R, class U> requires std::is_integral_v<R>
        void cfun(lua_State *L, std::function<R(T*, U)> func)
        {
            using FnType = decltype (func);
            new (lua_newuserdata(L, sizeof(func))) FnType(func);
            lua_pushcclosure(L, TraitsClassFunCFunc<T>::f, 1);
        }

        template <class T, class R> requires std::is_integral_v<R>
        void cfun(lua_State *L, R(*func)(State))
        {
            using FnType = decltype (func);
            new (lua_newuserdata(L, sizeof(func))) FnType(func);
            lua_pushcclosure(L, TraitsCFunc::f, 1);
        }
        
        template <class T, class F>
        void fun(lua_State *L, F& func)
        {
            fun<T>(L, static_cast<function_type_t<std::decay_t<F>>>(func));
        }

        template <class T, class F>
        void cfun(lua_State *L, F& func)
        {
            cfun<T>(L, static_cast<function_type_t<std::decay_t<F>>>(func));
        }
        
        template <class T, class F>
        void fun(lua_State *L, F&& func)
        {
            fun<T>(L, static_cast<function_type_t<std::decay_t<F>>>(func));
        }

        template <class T, class F>
        void cfun(lua_State *L, F&& func)
        {
            cfun<T>(L, static_cast<function_type_t<std::decay_t<F>>>(func));
        }
    }

    template<typename T>
    struct disect_function;

    template<typename R, typename ...Args>
    struct disect_function<R(Args...)>
    {
        static constexpr size_t nargs = sizeof...(Args);
        static constexpr bool isClass = false;

        template <size_t i>
        struct arg
        {
            typedef typename std::tuple_element<i, std::tuple<Args...>>::type type;
        };
    };

    template<typename R, typename ...Args>
    struct disect_function<R(Args...) const>
    {
        static constexpr size_t nargs = sizeof...(Args);
        static constexpr bool isClass = false;

        template <size_t i>
        struct arg
        {
            typedef typename std::tuple_element<i, std::tuple<Args...>>::type type;
        };
    };

    template<typename R, typename ...Args>
    struct disect_function<R(*)(Args...)>
    {
        static constexpr size_t nargs = sizeof...(Args);
        static constexpr bool isClass = false;

        template <size_t i>
        struct arg
        {
            typedef typename std::tuple_element<i, std::tuple<Args...>>::type type;
        };
    };

    template<typename R, typename T, typename ...Args>
    struct disect_function<R(T::*)(Args...)>
    {
        static constexpr size_t nargs = sizeof...(Args);
        static constexpr bool isClass = true;
        static constexpr T* classT = nullptr;

        template <size_t i>
        struct arg
        {
            typedef typename std::tuple_element<i, std::tuple<Args...>>::type type;
        };
    };

    template<typename R, typename T, typename ...Args>
    struct disect_function<R(T::*)(Args...) const>
    {
        static constexpr size_t nargs = sizeof...(Args);
        static constexpr bool isClass = true;
        static constexpr T* classT = nullptr;

        template <size_t i>
        struct arg
        {
            typedef typename std::tuple_element<i, std::tuple<Args...>>::type type;
        };
    };

    template<typename R, typename ...Args>
    struct disect_function<std::function<R(Args...)>>
    {
        static constexpr size_t nargs = sizeof...(Args);
        static constexpr bool isClass = false;

        template <size_t i>
        struct arg
        {
            typedef typename std::tuple_element<i, std::tuple<Args...>>::type type;
        };
    };

    template <typename F, size_t N = 0, size_t J = 1>
    void LoopTupleT(lua_State *L)
    {
        if constexpr (N == 0)
        {
            if constexpr(!std::is_same_v<State, std::decay_t<disect_function<F>::template arg<0>::type>> && !std::is_same_v<Environment, std::decay_t<disect_function<F>::template arg<0>::type>>)
            {
                lua_pushinteger(L, detail::basic_type<std::decay_t<disect_function<F>::template arg<0>::type>>(L));
                lua_rawseti(L, -2, J + (disect_function<F>::isClass ? 1 : 0));
                LoopTupleT<F, 1, J + 1>(L);
            } else if constexpr(std::is_same_v<Object, std::decay_t<disect_function<F>::template arg<0>::type>> || std::is_same_v<ObjectRef, std::decay_t<disect_function<F>::template arg<0>::type>>)
            {
                lua_pushinteger(L, -2);
                lua_rawseti(L, -2, J + (disect_function<F>::isClass ? 1 : 0));
                LoopTupleT<F, 1, J + 1>(L);
            } else
                LoopTupleT<F, 1, J>(L);
        } else if constexpr(N < disect_function<F>::nargs) {
            if constexpr(!std::is_same_v<State, std::decay_t<disect_function<F>::template arg<N-1>::type>> && !std::is_same_v<Environment, std::decay_t<disect_function<F>::template arg<N-1>::type>>)
            {
                lua_pushinteger(L, detail::basic_type<std::decay_t<disect_function<F>::template arg<N-1>::type>>(L));
                lua_rawseti(L, -2, J + (disect_function<F>::isClass ? 1 : 0));
                LoopTupleT<F, N + 1, J + 1>(L);
            } else if constexpr(std::is_same_v<Object, std::decay_t<disect_function<F>::template arg<N-1>::type>> || std::is_same_v<ObjectRef, std::decay_t<disect_function<F>::template arg<N-1>::type>>)
            {
                lua_pushinteger(L, -2);
                lua_rawseti(L, -2, J + (disect_function<F>::isClass ? 1 : 0));
                LoopTupleT<F, N + 1, J + 1>(L);
            } else
                LoopTupleT<F, N + 1, J>(L);
        }
    }

    template<typename C, typename ...Functions>
    class OverloadedFunction {
        lua_State* L = nullptr;
    public:
        OverloadedFunction(lua_State *L, Functions... functions) {
            this->L = L;
            lua_newtable(L);
            auto fun_index = 1;
            auto push_fun = [&fun_index](lua_State* L, auto f) {
                lua_newtable(L);

                lua_newtable(L);
                if constexpr(disect_function<decltype(f)>::isClass) {
                    lua_pushinteger(L, LUA_TUSERDATA);
                    lua_rawseti(L, -2, 1);
                }

                if constexpr(disect_function<decltype(f)>::nargs > 0)
                    LoopTupleT<decltype(f)>(L);

                lua_pushinteger(L, lua_getlen(L, -1));
                lua_setfield(L, -3, "nargs");

                lua_setfield(L, -2, "argl");

                Function::fun<C>(L, f);
                lua_setfield(L, -2, "f");

                lua_pushstring(L, typeid(f).name());
                lua_setfield(L, -2, "typeid");

                lua_rawseti(L, -2, fun_index++);

                return 0;
            };
            (push_fun(L, functions), ...);
            lua_pushcclosure(L, call, 1);
        }
        static int call(lua_State* L) {
            auto argn = lua_gettop(L);

            lua_pushvalue(L, lua_upvalueindex(1));

            lua_pushnil(L);
            while(lua_next(L, -2) != 0) {
                lua_getfield(L, -1, "nargs");
                if (lua_tointeger(L, -1) != argn)
                {
                    lua_pop(L, 2);
                    continue;
                }
                lua_pop(L, 1);

                bool valid = true;

                lua_getfield(L, -1, "argl");
                lua_pushnil(L);
                while(lua_next(L, -2) != 0) {
                    if (lua_tointeger(L, -1) == -2 || lua_tointeger(L, -1) != lua_type(L, lua_tointeger(L, -2)))
                        valid = false;
                    lua_pop(L, 1);
                }
                lua_pop(L, 1);

                if (valid) {
                    lua_getfield(L, -1, "f");
                    lua_insert(L, 1);
                    lua_pop(L, 3);
                    return LuaBinding::pcall(L, argn, LUA_MULTRET);
                }

                lua_pop(L, 1);
            }

            luaL_error(L, "no matching overload with %d args and input values", argn);

            return 0;
        }
    };

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
            return _resolve::resolve_v(std::is_member_object_pointer<Sig C::*>(), mem_ptr);
        }

        template<typename F>
        concept has_call_operator = requires (F) {
            &F::operator();
        };

        template<typename F>
        concept is_cfun_style = std::is_invocable_v<F, State> || std::is_invocable_v<F, lua_State*>;
        
        template <typename T>
        concept is_pushable_ref_fun = requires (T &t) {
            { Function::fun<void>(nullptr, t) };
        };

        template <typename T>
        concept is_pushable_forward_fun = requires (T &&t) {
            { Function::fun<void>(nullptr, t) };
        };

        template <typename T>
        concept is_pushable_ref_cfun = requires (T &t) {
            { Function::cfun<void>(nullptr, t) };
        };

        template <typename T>
        concept is_pushable_forward_cfun = requires (T &&t) {
            { Function::cfun<void>(nullptr, t) };
        };

        template <typename T>
        concept is_pushable_fun =
            is_pushable_ref_fun<std::decay_t<T>> || is_pushable_forward_fun<std::decay_t<T>>;

        template <typename T>
        concept is_pushable_cfun =
            (is_pushable_ref_cfun<std::decay_t<T>> || is_pushable_forward_cfun<std::decay_t<T>>) && is_cfun_style<std::decay_t<T>>;
    }
}
#include <memory>

namespace LuaBinding {
    template<typename T>
    class TableIter
    {
    public:
        typedef std::forward_iterator_tag iterator_category;
        TableIter(lua_State* L, int index, int base) : L(L), index(index), base(base) { }
        TableIter operator++() { index++; return *this; }
        std::pair<int, T> operator*() { lua_rawgeti(L, base, index); return { index, T(L, -1, false) }; }
        bool operator!=(const TableIter& rhs) { return index != rhs.index; }
    private:
        lua_State* L;
        int index;
        int base;
    };

    template <typename T>
    concept has_lua_state = requires(T t)
    {
        { t.lua_state() } -> std::convertible_to<void*>;
    };

    class Object {
    protected:
        int idx = LUA_REFNIL;
        lua_State* L = nullptr;
        bool owned = false;
    public:
        Object() = default;
        Object(lua_State* L, int idx) : L(L)
        {
            this->idx = lua_absindex(L, idx);
        }
        Object(lua_State* L, int idx, bool owned) : L(L)
        {
            this->idx = lua_absindex(L, idx);
            this->owned = owned;
        }
        Object(const Object& other) : L(other.L), idx(other.idx), owned(false)
        {}
        Object(Object&& other) noexcept : L(other.L), idx(other.idx), owned(other.owned)
        {
            other.idx = LUA_REFNIL;
            other.owned = false;
        }
        Object& operator=(const Object& other)
        {
            this->L = other.L;
            this->idx = other.idx;
            this->owned = false;
            return *this;
        }
        ~Object() {
            if (owned)
                pop();
        }

        virtual const char* tostring()
        {
            return lua_tostring(L, idx);
        }

        virtual const char* tolstring()
        {
            return luaL_tolstring(L, -1, nullptr);
        }

        template<typename T>
        T as()
        {
            return detail::get<T>(L, idx);
        }

        template<typename T>
        T extract()
        {
            auto res = detail::get<T>(L, idx);
            pop();
            return res;
        }

        template <class T>
        operator T () const requires (!std::is_same_v<T, ObjectRef>)
        {
            return as <T> ();
        }

        template <class T> requires (!std::is_same_v<T, ObjectRef>)
        operator T ()
        {
            return as <T> ();
        }

        template<typename T>
        bool is()
        {
            if constexpr (std::is_same_v<T, void>)
            {
                return lua_isnoneornil(L, idx);
            } else {
                return detail::is<T>(L, idx);
            }
        }

        virtual bool is(int t)
        {
            return lua_type(L, idx) == t;
        }

        [[nodiscard]] virtual const void* topointer() const
        {
            return lua_topointer(L, idx);
        }

        virtual int push(int i = -1) const
        {
            lua_pushvalue(L, idx);
            if (i != -1)
                lua_insert(L, i);
            return 1;
        }

        virtual int push(int i = -1)
        {
            lua_pushvalue(L, idx);
            if (i != -1)
                lua_insert(L, i);
            return 1;
        }

        virtual void pop() {
            lua_remove(L, idx);
            this->idx = LUA_REFNIL;
        }

        template<typename Ref = ObjectRef> requires std::is_base_of_v<ObjectRef, Ref>
        Ref operator[](size_t index)
        {
            lua_geti(L, this->idx, index);
            return Ref(L, lua_gettop(L), false);
        }

        template<typename Ref = ObjectRef> requires std::is_base_of_v<ObjectRef, Ref>
        Ref operator[](const char* index)
        {
            lua_getfield(L, this->idx, index);
            return Ref(L, lua_gettop(L), false);
        }

        template<typename T>
        Object& operator=(T value)
        {
            detail::push(L, value);
            lua_replace(L, idx);
            return *this;
        }

        template<typename R>
        R call() {
            push();
            if constexpr (std::is_same_v<void, R>) {
                if (LuaBinding::pcall(L, 0, 0))
                {
#ifndef NOEXCEPTIONS
                    throw std::exception(lua_tostring(L, -1));
#endif
                }
            } else {
                if (LuaBinding::pcall(L, 0, 1))
                {
#ifndef NOEXCEPTIONS
                    throw std::exception(lua_tostring(L, -1));
#endif
                }
                auto result = LuaBinding::detail::get<R>(L, -1);
                lua_pop(L, 1);
                return result;
            }
        }

        template<int R>
        void call() {
            push();
            if (LuaBinding::pcall(L, 0, R))
            {
#ifndef NOEXCEPTIONS
                    throw std::exception(lua_tostring(L, -1));
#endif
            }
        }

        template<typename R, typename ...Params> requires (sizeof...(Params) > 0 && !std::is_same_v<std::tuple_element_t<0, std::tuple<Params...>>, Environment>)
        R call(Params... param) {
            push();
            if constexpr (sizeof...(param) > 0)
                (void)std::initializer_list<int>{ detail::push(L, param)... };
            if constexpr (std::is_same_v<void, R>) {
                if (LuaBinding::pcall(L, sizeof...(param), 0))
                {
#ifndef NOEXCEPTIONS
                    throw std::exception(lua_tostring(L, -1));
#endif
                }
            } else {
                if (LuaBinding::pcall(L, sizeof...(param), 1))
                {
#ifndef NOEXCEPTIONS
                    throw std::exception(lua_tostring(L, -1));
#endif
                }
                auto result = LuaBinding::detail::get<R>(L, -1);
                lua_pop(L, 1);
                return result;
            }
        }

        template<int R, typename ...Params> requires (sizeof...(Params) > 0 && !std::is_same_v<std::tuple_element_t<0, std::tuple<Params...>>, Environment>)
        void call(Params... param) {
            push();
            if constexpr (sizeof...(param) > 0)
                (void)std::initializer_list<int>{ detail::push(L, param)... };
            if (LuaBinding::pcall(L, sizeof...(param), R))
            {
#ifndef NOEXCEPTIONS
                    throw std::exception(lua_tostring(L, -1));
#endif
            }
        }

        template<typename R, typename Env, typename ...Params> requires (std::is_same_v<Env, Environment>)
        R call(Env env, Params... param) {
            push();
            if constexpr (sizeof...(param) > 0)
                (void)std::initializer_list<int>{ detail::push(L, param)... };
            if constexpr (std::is_same_v<void, R>) {
                if (env.pcall(sizeof...(param), 0))
                {
#ifndef NOEXCEPTIONS
                    throw std::exception(lua_tostring(L, -1));
#endif
                }
            } else {
                if (env.pcall(sizeof...(param), 1))
                {
#ifndef NOEXCEPTIONS
                    throw std::exception(lua_tostring(L, -1));
#endif
                }
                auto result = LuaBinding::detail::get<R>(L, -1);
                lua_pop(L, 1);
                return result;
            }
        }

        template<int R, typename Env, typename ...Params> requires (std::is_same_v<Env, Environment>)
        void call(Env env, Params... param) {
            push();
            if constexpr (sizeof...(param) > 0)
                (void)std::initializer_list<int>{ detail::push(L, param)... };
            if (env.pcall(sizeof...(param), R))
            {
#ifndef NOEXCEPTIONS
                    throw std::exception(lua_tostring(L, -1));
#endif
            }
        }

        template<typename Ref = ObjectRef> requires std::is_base_of_v<ObjectRef, Ref>
        Ref operator() () {
            push();
            if (LuaBinding::pcall(L, 0, 1))
            {
#ifndef NOEXCEPTIONS
                    throw std::exception(lua_tostring(L, -1));
#endif
            }
            return Ref(L, -1, false);
        }

        template<typename Ref = ObjectRef, typename ...Params> requires (std::is_base_of_v<ObjectRef, Ref>) && (sizeof...(Params) > 0 && !std::is_same_v<std::tuple_element_t<0, std::tuple<Params...>>, Environment>)
        Ref operator() (Params... param) {
            push();
            if constexpr (sizeof...(param) > 0)
                (void)std::initializer_list<int>{ detail::push(L, param)... };
            if (LuaBinding::pcall(L, sizeof...(param), 1))
            {
#ifndef NOEXCEPTIONS
                    throw std::exception(lua_tostring(L, -1));
#endif
            }
            return Ref(L, -1, false);
        }

        template<typename Ref = ObjectRef, typename Env, typename ...Params> requires (std::is_same_v<Env, Environment>)
        Ref operator()(Env env, Params... param) {
            push();
            if constexpr (sizeof...(param) > 0)
                (void)std::initializer_list<int>{ detail::push(L, param)... };
            if (env.pcall(sizeof...(param), 1))
            {
#ifndef NOEXCEPTIONS
                    throw std::exception(lua_tostring(L, -1));
#endif
            }
            return Ref(L, -1, false);
        }

        template <typename T>
        void set(const char* index, T&& other)
        {
            detail::push(L, other);
            lua_setfield(L, idx, index);
        }

        template <typename T>
        void set(int index, T&& other)
        {
            lua_pushinteger(L, index);
            detail::push(L, other);
            lua_settable(L, idx);
        }

        void unset(const char* index)
        {
            lua_pushnil(L);
            lua_setfield(L, idx, index);
        }

        void unset(int index)
        {
            lua_pushinteger(L, index);
            lua_pushnil(L);
            lua_settable(L, idx);
        }

        template <typename T>
        void fun(int index, T& other)
        {
            Function::fun<void>(L, other);
            lua_rawseti(L, idx, index);
        }

        template <typename T>
        void fun(int index, T&& other)
        {
            Function::fun<void>(L, other);
            lua_rawseti(L, idx, index);
        }

        template <typename T>
        void cfun(int index, T& other)
        {
            Function::cfun<void>(L, other);
            lua_rawseti(L, idx, index);
        }

        template <typename T>
        void cfun(int index, T&& other)
        {
            Function::cfun<void>(L, other);
            lua_rawseti(L, idx, index);
        }

        template <typename T>
        void fun(const char* index, T& other)
        {
            Function::fun<void>(L, other);
            lua_setfield(L, idx, index);
        }

        template <typename T>
        void fun(const char* index, T&& other)
        {
            Function::fun<void>(L, other);
            lua_setfield(L, idx, index);
        }

        template <typename T>
        void cfun(const char* index, T& other)
        {
            Function::cfun<void>(L, other);
            lua_setfield(L, idx, index);
        }

        template <typename T>
        void cfun(const char* index, T&& other)
        {
            Function::cfun<void>(L, other);
            lua_setfield(L, idx, index);
        }

        [[nodiscard]] int index() const
        {
            return idx;
        }

        lua_State* lua_state() const
        {
            return L;
        }

        lua_State* lua_state()
        {
            return L;
        }

        virtual int type()
        {
            return lua_type(L, idx);
        }

        virtual int type() const
        {
            return lua_type(L, idx);
        }

        bool valid()
        {
            return L && this->idx != LUA_REFNIL;
        }

        bool valid() const
        {
            return L && this->idx != LUA_REFNIL;
        }

        bool valid(lua_State *S)
        {
            return L && L == S && this->idx != LUA_REFNIL;
        }

        bool valid(lua_State *S) const
        {
            return L && L == S && this->idx != LUA_REFNIL;
        }

        template <typename T>
        bool valid(T *S) requires has_lua_state<T>
        {
            return L && S && L == S->lua_state() && this->idx != LUA_REFNIL;
        }

        template <typename T>
        bool valid(T *S) const requires has_lua_state<T>
        {
            return L && S && L == S->lua_state() && this->idx != LUA_REFNIL;
        }

        template <typename T>
        bool valid(std::shared_ptr<T> S) requires has_lua_state<T>
        {
            return L && S && L == S->lua_state() && this->idx != LUA_REFNIL;
        }

        template <typename T>
        bool valid(std::shared_ptr<T> S) const requires has_lua_state<T>
        {
            return L && S && L == S->lua_state() && this->idx != LUA_REFNIL;
        }

        bool valid(int ltype)
        {
            return L && this->idx != LUA_REFNIL && type() == ltype;
        }

        bool valid(int ltype) const
        {
            return L && this->idx != LUA_REFNIL && type() == ltype;
        }

        bool valid(lua_State *S, int ltype)
        {
            return L && L == S && this->idx != LUA_REFNIL && type() == ltype;
        }

        bool valid(lua_State *S, int ltype) const
        {
            return L && L == S && this->idx != LUA_REFNIL && type() == ltype;
        }

        template <typename T>
        bool valid(T *S, int ltype) requires has_lua_state<T>
        {
            return L && L == S->lua_state() && this->idx != LUA_REFNIL && type() == ltype;
        }

        template <typename T>
        bool valid(T *S, int ltype) const requires has_lua_state<T>
        {
            return L && L == S->lua_state() && this->idx != LUA_REFNIL && type() == ltype;
        }

        const char* type_name()
        {
            return lua_typename(L, type());
        }

        void invalidate()
        {
            L = nullptr;
            this->idx = LUA_REFNIL;
        }

        virtual int len()
        {
            return lua_getlen(L, idx);
        }

        virtual int len() const
        {
            return lua_getlen(L, idx);
        }

        TableIter<ObjectRef> begin()
        {
            return TableIter<ObjectRef>(L, 1, this->idx);
        }
        TableIter<ObjectRef> end()
        {
            return TableIter<ObjectRef>(L, lua_getlen(L, idx) + 1, this->idx);
        }
        TableIter<ObjectRef> begin() const
        {
            return TableIter<ObjectRef>(L, 1, this->idx);
        }
        TableIter<ObjectRef> end() const
        {
            return TableIter<ObjectRef>(L, lua_getlen(L, idx) + 1, this->idx);
        }
    };

    class ObjectRef : public Object {
    public:
        ObjectRef() = default;
        ObjectRef(lua_State* L, int idx, bool copy = true)
        {
            this->L = L;
            if (copy || idx != -1 && idx != lua_gettop(L))
                lua_pushvalue(L, idx);
            this->idx = luaL_ref(L, LUA_REGISTRYINDEX);
        }
        template<typename T> requires std::is_same_v<IndexProxy, T>
        ObjectRef(const T& other)
        {
            this->L = other.lua_state();
            if (other.valid(L))
            {
                other.push();
                this->idx = luaL_ref(L, LUA_REGISTRYINDEX);
            }
        }
        template<typename T> requires std::is_same_v<IndexProxy, T>
        ObjectRef& operator=(T&& other) noexcept
        {
            this->L = other.lua_state();
            if (other.valid(L))
            {
                other.push();
                this->idx = luaL_ref(L, LUA_REGISTRYINDEX);
            }
            return *this;
        }
        ObjectRef(const Object& other)
        {
            this->L = other.lua_state();
            if (other.valid(L))
            {
                other.push();
                this->idx = luaL_ref(L, LUA_REGISTRYINDEX);
            }
        }
        ObjectRef& operator=(Object&& other) noexcept {
            this->L = other.lua_state();
            if (other.valid(L))
            {
                other.push();
                this->idx = luaL_ref(L, LUA_REGISTRYINDEX);
            }
            return *this;
        }
        ObjectRef(const ObjectRef& other)
        {
            this->L = other.lua_state();
            if (other.valid(L))
            {
                other.push();
                this->idx = luaL_ref(L, LUA_REGISTRYINDEX);
            }
        }
        ObjectRef& operator=(ObjectRef&& other) noexcept {
            if (valid(other.lua_state()))
            {
                luaL_unref(L, LUA_REGISTRYINDEX, idx);
                idx = LUA_REFNIL;
                L = nullptr;
            }
            this->L = other.lua_state();
            if (other.valid(L))
            {
                other.push();
                this->idx = luaL_ref(L, LUA_REGISTRYINDEX);
            }
            return *this;
        }
        ~ObjectRef() {
            if (valid(L))
            {
                luaL_unref(L, LUA_REGISTRYINDEX, idx);
                idx = LUA_REFNIL;
                L = nullptr;
            }
        }

        const char* tostring() override
        {
            push();
            auto result = lua_tostring(L, -1);
            lua_pop(L, 1);
            return result;
        }

        const char* tolstring() override
        {
            push();
            auto result = luaL_tolstring(L, -1, nullptr);
            lua_pop(L, 1);
            return result;
        }

        template <class T>
        operator T () const
        {
            return as <T> ();
        }

        template <class T>
        operator T ()
        {
            return as <T> ();
        }

        template<typename T>
        T as()
        {
            push();
            auto t = detail::get<T>(L, -1);
            lua_pop(L, 1);
            return t;
        }

        template<typename T>
        bool is()
        {
            if constexpr (std::is_same_v<T, void>)
            {
                push();
                auto t = lua_isnoneornil(L, -1);
                lua_pop(L, 1);
                return t;
            } else {
                push();
                auto t = detail::is<T>(L, -1);
                lua_pop(L, 1);
                return t;
            }
        }

        bool is(int t) override
        {
            push();
            auto b = lua_type(L, -1) == t;
            lua_pop(L, 1);
            return b;
        }

        template <typename T>
        void set(const char* index, T&& other)
        {
            push();
            detail::push(L, other);
            lua_setfield(L, -2, index);
            lua_pop(L, 1);
        }

        template <typename T>
        void set(int index, T&& other)
        {
            push();
            lua_pushinteger(L, index);
            detail::push(L, other);
            lua_settable(L, -3);
            lua_pop(L, 1);
        }

        void unset(const char* index)
        {
            push();
            lua_pushnil(L);
            lua_setfield(L, -2, index);
            lua_pop(L, 1);
        }

        void unset(int index)
        {
            push();
            lua_pushinteger(L, index);
            lua_pushnil(L);
            lua_settable(L, -3);
            lua_pop(L, 1);
        }

        template <typename T>
        void fun(int index, T& other)
        {
            push();
            Function::fun<void>(L, other);
            lua_rawseti(L, -2, index);
            lua_pop(L, 1);
        }

        template <typename T>
        void fun(int index, T&& other)
        {
            push();
            Function::fun<void>(L, other);
            lua_rawseti(L, -2, index);
            lua_pop(L, 1);
        }

        template <typename T>
        void cfun(int index, T& other)
        {
            push();
            Function::cfun<void>(L, other);
            lua_rawseti(L, -2, index);
            lua_pop(L, 1);
        }

        template <typename T>
        void cfun(int index, T&& other)
        {
            push();
            Function::cfun<void>(L, other);
            lua_rawseti(L, -2, index);
            lua_pop(L, 1);
        }

        template <typename T>
        void fun(const char* index, T& other)
        {
            push();
            Function::fun<void>(L, other);
            lua_setfield(L, -2, index);
            lua_pop(L, 1);
        }

        template <typename T>
        void fun(const char* index, T&& other)
        {
            push();
            Function::fun<void>(L, other);
            lua_setfield(L, -2, index);
            lua_pop(L, 1);
        }

        template <typename T>
        void cfun(const char* index, T& other)
        {
            push();
            Function::cfun<void>(L, other);
            lua_setfield(L, -2, index);
            lua_pop(L, 1);
        }

        template <typename T>
        void cfun(const char* index, T&& other)
        {
            push();
            Function::cfun<void>(L, other);
            lua_setfield(L, -2, index);
            lua_pop(L, 1);
        }

        Object get() {
            push();
            return Object(L, -1, true);
        }

        template<typename IP = IndexProxy> requires std::is_base_of_v<IP, IndexProxy>
        IP operator[](size_t idx)
        {
            return IP(*this, idx);
        }

        template<typename IP = IndexProxy> requires std::is_base_of_v<IP, IndexProxy>
        IP operator[](const char* idx)
        {
            return IP(*this, idx);
        }

        [[nodiscard]] const void* topointer() const override
        {
            push();
            auto t = lua_topointer(L, -1);
            lua_pop(L, 1);
            return t;
        }

        int push(int i = -1) const override
        {
            lua_rawgeti(L, LUA_REGISTRYINDEX, idx);
            if (i != -1)
                lua_insert(L, i);
            return 1;
        }

        int push(int i = -1) override
        {
            lua_rawgeti(L, LUA_REGISTRYINDEX, idx);
            if (i != -1)
                lua_insert(L, i);
            return 1;
        }

        void pop() override
        {
            luaL_unref(L, LUA_REGISTRYINDEX, idx);
            this->idx = LUA_REFNIL;
        }

        int type() override
        {
            push();
            auto t = lua_type(L, -1);
            lua_pop(L, 1);
            return t;
        }

        int type() const override
        {
            push();
            auto t = lua_type(L, -1);
            lua_pop(L, 1);
            return t;
        }

        int len() override
        {
            push();
            auto t = lua_getlen(L, -1);
            lua_pop(L, 1);
            return t;
        }

        int len() const override
        {
            push();
            auto t = lua_getlen(L, -1);
            lua_pop(L, 1);
            return t;
        }
    };

    class IndexProxy : public ObjectRef {
    protected:
        ObjectRef& element;
        const char* str_index;
        int int_index;

    public:
        IndexProxy(ObjectRef& el, const char* str_index) : element(el), str_index(str_index), int_index(-1) {
            this->L = el.lua_state();
        }
        IndexProxy(ObjectRef& el, int int_index) : element(el), str_index(nullptr), int_index(int_index) {
            this->L = el.lua_state();
        }

        operator ObjectRef () const {
            element.push();
            if (str_index)
                lua_getfield(L, -1, str_index);
            else
                lua_rawgeti(L, -1, int_index);
            lua_remove(L, -2);
            return ObjectRef(L, -1, false);
        }

        int push(int i = -1) const override
        {
            element.push();
            if (str_index)
                lua_getfield(L, -1, str_index);
            else
                lua_rawgeti(L, -1, int_index);
            lua_remove(L, -2);
            if (i != -1)
                lua_insert(L, i);
            return 1;
        }

        int push(int i = -1) override
        {
            printf("pushing index proxy\n");
            element.push();
            if (str_index)
                lua_getfield(L, -1, str_index);
            else
                lua_rawgeti(L, -1, int_index);
            lua_remove(L, -2);
            if (i != -1)
                lua_insert(L, i);
            return 1;
        }

        void pop() override
        {
            element.push();
            lua_pushnil(L);
            if (str_index)
                lua_setfield(L, -2, str_index);
            else
                lua_rawseti(L, -2, int_index);
            lua_pop(L, 1);
        }

        template <typename T>
        IndexProxy& operator=(const T& rhs) {
            if constexpr (detail::is_pushable_cfun<T>) {
                if (str_index)
                    element.cfun(str_index, rhs);
                else
                    element.cfun(int_index, rhs);
            } else if constexpr (detail::is_pushable_fun<T>) {
                if (str_index)
                    element.fun(str_index, rhs);
                else
                    element.fun(int_index, rhs);
            } else {
                if (str_index)
                    element.set(str_index, rhs);
                else
                    element.set(int_index, rhs);
            }
            return *this;
        }

        template <typename T>
        IndexProxy& operator=(const T&& rhs) {
            if constexpr (detail::is_pushable_cfun<T>) {
                if (str_index)
                    element.cfun(str_index, rhs);
                else
                    element.cfun(int_index, rhs);
            } else if constexpr (detail::is_pushable_fun<T>) {
                if (str_index)
                    element.fun(str_index, rhs);
                else
                    element.fun(int_index, rhs);
            } else {
                if (str_index)
                    element.set(str_index, rhs);
                else
                    element.set(int_index, rhs);
            }
            return *this;
        }
    };

    template<>
    class Stack<Object> {
    public:
        static int push(lua_State* L, Object t)
        {
            return t.push();
        }
        static bool is(lua_State*, int ) {
            return true;
        }
        static Object get(lua_State* L, int index)
        {
            return Object(L, index);
        }
        static const char* basic_type_name(lua_State* L) {
            return "object";
        }
        static int basic_type(lua_State* L) {
            return -2;
        }
    };

    template<>
    class Stack<Object*> {
    public:
        static int push(lua_State* L, Object* t)
        {
            static_assert(true, "Use ObjectRef or ObjectRef& instead of ObjectRef*");
            return 0;
        }
        static bool is(lua_State* L, int index) {
            static_assert(true, "Use ObjectRef or ObjectRef& instead of ObjectRef*");
            return false;
        }
        static Object* get(lua_State* L, int index)
        {
            static_assert(true, "Use ObjectRef or ObjectRef& instead of ObjectRef*");
            return nullptr;
        }
    };

    template<>
    class Stack<ObjectRef> {
    public:
        static int push(lua_State* L, ObjectRef t)
        {
            return t.push();
        }
        static bool is(lua_State*, int ) {
            return true;
        }
        static ObjectRef get(lua_State* L, int index)
        {
            return {L, index, true};
        }
        static const char* basic_type_name(lua_State* L) {
            return "object";
        }
        static int basic_type(lua_State* L) {
            return -2;
        }
    };

    template<>
    class Stack<ObjectRef*> {
    public:
        static int push(lua_State* L, ObjectRef* t)
        {
            static_assert(true, "Use ObjectRef or ObjectRef& instead of ObjectRef*");
            return 0;
        }
        static bool is(lua_State* L, int index) {
            static_assert(true, "Use ObjectRef or ObjectRef& instead of ObjectRef*");
            return false;
        }
        static ObjectRef* get(lua_State* L, int index)
        {
            static_assert(true, "Use ObjectRef or ObjectRef& instead of ObjectRef*");
            return nullptr;
        }
    };

    template<>
    class Stack<IndexProxy> {
    public:
        static int push(lua_State* L, IndexProxy t)
        {
            return t.push();
        }
        static bool is(lua_State*, int ) {
            return true;
        }
        static IndexProxy get(lua_State* L, int index)
        {
            static_assert(true, "Cannot get IndexProxy from lua stack");
        }
        static const char* basic_type_name(lua_State* L) {
            return "object";
        }
        static int basic_type(lua_State* L) {
            return -2;
        }
    };

    template<>
    class Stack<IndexProxy*> {
    public:
        static int push(lua_State* L, IndexProxy* t)
        {
            static_assert(true, "Use IndexProxy or IndexProxy& instead of IndexProxy*");
            return 0;
        }
        static bool is(lua_State* L, int index) {
            static_assert(true, "Use IndexProxy or IndexProxy& instead of IndexProxy*");
            return false;
        }
        static IndexProxy* get(lua_State* L, int index)
        {
            static_assert(true, "Use IndexProxy or IndexProxy& instead of IndexProxy*");
            return nullptr;
        }
    };
}


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
            lua_getupvalue(L, findex, 1);
#endif
            this->idx = luaL_ref(L, LUA_REGISTRYINDEX);
        }
        explicit Environment(lua_State* L, bool)
        {
            this->L = L;
#if LUA_VERSION_NUM < 502
            lua_getfenv(L, lua_upvalueindex(1));
#else
            lua_getupvalue(L, lua_upvalueindex(1), 1);
#endif
            this->idx = luaL_ref(L, LUA_REGISTRYINDEX);
        }
        int pcall(int narg = 0, int nres = 0) {
            lua_rawgeti(L, LUA_REGISTRYINDEX, idx);
#if LUA_VERSION_NUM < 502
            lua_setfenv(L, lua_gettop(L) - narg - 1);
#else
            lua_setupvalue(L, lua_gettop(L) - narg - 1, 1);
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

#ifdef LUABINDING_DYN_CLASSES

#ifdef LUABINDING_DYN_CLASSES
#include <functional>
#include <cassert>

#ifdef LUABINDING_DYN_CLASSES
namespace LuaBinding {
    enum MemoryType {
        _char = -0x1,
        _int = -0x2,
        _float = -0x4,
        _bool = -0x8,
        _double = -0x10,
        ptr = -0x20,
        vec2 = -0x40,
        vec3 = -0x80,
        _array = -0x200,
        _short = -0x400,
        _byte = -0x800,
        uint = -0x2000,
        ushort = -0x4000,
        string = -0x8000,
        null = -0x10000,
        cstruct = -0x20000,
        vector = -0x40000,
        point = -0x100000,
        udata = -0x200000,

        stdstring = -0x400000,
        stringptr = -0x800000,
    };
    int lua_pushmemtype(lua_State* L, MemoryType type, void* data);
    int lua_pullmemtype(lua_State* L, MemoryType type, void* addr);
    unsigned int get_type_size(MemoryType mt);
}
#endif

namespace LuaBinding {

    class DynClass {
        lua_State* L = nullptr;
        const char* class_name;
    public:
        DynClass(lua_State* L, const char* name) : L(L), class_name(name)
        {
            push_metatable(L, this);

            lua_pushcclosure(L, lua_CGCFunction, 0);
            lua_setfield(L, -2, "__gc");

            lua_pushcclosure(L, lua_CIndexFunction, 0);
            lua_setfield(L, -2, "__index");

            lua_pushcclosure(L, lua_EQFunction, 0);
            lua_setfield(L, -2, "__eq");

            lua_pushcclosure(L, lua_CNewIndexFunction, 0);
            lua_setfield(L, -2, "__newindex");

            lua_newtable(L);
            lua_setfield(L, -2, "__pindex");

            lua_newtable(L);
            lua_setfield(L, -2, "__findex");

            lua_pushstring(L, name);
            lua_setfield(L, -2, "__name");

            lua_pushstring(L, name);
            lua_pushcclosure(L, tostring, 1);
            lua_setfield(L, -2, "__tostring");

            lua_pushlightuserdata(L, this);
            lua_setfield(L, -2, "__key");

            lua_pushcfunction(L, dyn_call);
            lua_setfield(L, -2, "New");

            lua_pushcfunction(L, dyn_prop);
            lua_setfield(L, -2, "Prop");

            lua_pushcfunction(L, dyn_read);
            lua_setfield(L, -2, "Read");

            lua_pushcfunction(L, dyn_cast);
            lua_setfield(L, -2, "Cast");

            push_property_index();
            lua_pushcfunction(L, get_ptr);
            lua_setfield(L, -2, "ptr");

            lua_pushcfunction(L, get_vtable);
            lua_setfield(L, -2, "vTable");

            lua_pop(L, 1);

            push_property_newindex();
            lua_pushcfunction(L, set_ptr);
            lua_setfield(L, -2, "ptr");

            lua_pop(L, 1);
        }
    private:
        static int tostring(lua_State* S)
        {
            char asdf[128];
#ifdef ENV32
                snprintf(asdf, sizeof(asdf), "%s{%X}", lua_tostring(S, lua_upvalueindex(1)), (uintptr_t)topointer(S, 1));
#elifdef ENV64
                snprintf(asdf, sizeof(asdf), "%s{%llX}", lua_tostring(S, lua_upvalueindex(1)), (uintptr_t)topointer(S, 1));
#endif
            lua_pushstring(S, asdf);
            return 1;
        }
        static int lua_CGCFunction(lua_State* S)
        {
            if (luaL_getmetafield(S, 1, "__cgc"))
            {
                lua_pushvalue(S, 1);
                pcall(S, 1, 0);
            }
            auto u = (void**)lua_touserdata(S, 1);
            if (*(u + 1) == (void*)0xC0FFEE) {
                free(*u);
            }
            return 0;
        }
        static int lua_EQFunction(lua_State* S)
        {
            if (!lua_isuserdata(S, 1) || !lua_isuserdata(S, 2))
                lua_pushboolean(S, false);
            else
                lua_pushboolean(S, *(uintptr_t*)lua_touserdata(S, 1) == *(uintptr_t*)lua_touserdata(S, 2));
            return 1;
        }
        static int lua_CIndexFunction(lua_State* S)
        {
            if (strcmp(lua_tostring(S, 2), "ptr") != 0
             && strcmp(lua_tostring(S, 2), "isValid") != 0
             && luaL_getmetafield(S, 1, "__valid"))
            {
                auto success = false;
                lua_pushvalue(S, 1);
                lua_call(S, 1, 1);
                success = lua_toboolean(S, -1);
                if (!success)
                {
                    lua_pushvalue(S, 1);
                    if (luaL_getmetafield(S, 1, "__name")) {
                        char asdf[128];
#ifdef ENV32
                            snprintf(asdf, sizeof(asdf), "%s{%X}", lua_tostring(S, -1), (uintptr_t)topointer(S, 1));
#elifdef ENV64
                            snprintf(asdf, sizeof(asdf), "%s{%llX}", lua_tostring(S, -1), (uintptr_t)topointer(S, 1));
#endif
                        lua_pop(S, 1);

                        luaL_error(S, "Tried to access invalid %s (property '%s')", asdf, lua_tostring(S, 2));
                    } else {
                        lua_pop(S, 1);
                        luaL_error(S, "Tried to access invalid unknown object (property '%s')", lua_tostring(S, 2));
                    }
                    return 0; // ??
                }
                lua_settop(S, 3);
            }
            if (luaL_getmetafield(S, 1, "__pindex"))
            {
                lua_pushvalue(S, 2);
                if (luaL_gettable(S, -2))
                {
                    lua_remove(S, -2);
                    lua_insert(S, 1);
                    lua_call(S, lua_gettop(S) - 1, LUA_MULTRET);
                    return lua_gettop(S);
                }
                else lua_pop(S, 1);
            }
            if (luaL_getmetafield(S, 1, "__findex"))
            {
                lua_pushvalue(S, 2);
                if (luaL_gettable(S, -2))
                    return 1;
                else lua_pop(S, 1);
            }
            if (luaL_getmetafield(S, 1, lua_tostring(S, 2)))
                return 1;
            if (luaL_getmetafield(S, 1, "__cindex"))
            {
                lua_insert(S, 1);
                lua_call(S, lua_gettop(S) - 1, LUA_MULTRET);
                return lua_gettop(S);
            }
            return 0;
        }
        static int lua_CNewIndexFunction(lua_State* S)
        {
            if (luaL_getmetafield(S, 1, "__pnewindex"))
            {
                lua_pushvalue(S, 2);
                if (luaL_gettable(S, -2))
                {
                    lua_pushvalue(S, 1);
                    lua_pushvalue(S, 2);
                    lua_pushvalue(S, 3);
                    lua_call(S, 3, 0);
                    return 0;
                }
                else lua_pop(S, 1);
            }
            if (luaL_getmetafield(S, 1, "__cnewindex"))
            {
                lua_pushvalue(S, 1);
                lua_pushvalue(S, 2);
                lua_pushvalue(S, 3);
                lua_call(S, 3, 0);
                return 0;
            }
            lua_getmetatable(S, 1);
            lua_pushvalue(S, 2);
            lua_pushvalue(S, 3);
            lua_settable(S, -3);
            return 0;
        }
    public:
        static int push_metatable(lua_State *L, void* _this, bool assert = false)
        {
            lua_getglobal(L, "__METASTORE");
            if (lua_rawgetp(L, -1, _this))
            {
                lua_remove(L, -2);
                return 0;
            }
            if (assert)
            {
                if constexpr (sizeof(ptrdiff_t) == 4)
                    luaL_error(L, "metatable not found in __METASTORE for %X", _this);
                else if constexpr (sizeof(ptrdiff_t) == 8)
                    luaL_error(L, "metatable not found in __METASTORE for %llX", _this);
            }
            lua_pop(L, 1);
            lua_newtable(L);
            lua_pushvalue(L, -1);
            lua_rawsetp(L, -3, _this);
            lua_remove(L, -2);
            return 1;
        }
    private:
        void set_size(unsigned int size)
        {
            push_metatable(L, this);
            lua_pushinteger(L, size);
            lua_setfield(L, -2, "__size");
            lua_pop(L, 1);
        }
        size_t get_size()
        {
            push_metatable(L, this);
            lua_getfield(L, -1, "__size");
            auto result = lua_tointeger(L, -1);
            lua_pop(L, 2);
            return result;
        }
        void push_property_index()
        {
            push_metatable(L, this);
            lua_pushstring(L, "__pindex");
            if (!luaL_gettable(L, -2))
            {
                lua_pop(L, 1);
                lua_newtable(L);
                lua_pushvalue(L, -1);
                lua_setfield(L, -3, "__pindex");
            }
            lua_remove(L, -2);
        }
        void push_property_newindex()
        {
            push_metatable(L, this);
            lua_pushstring(L, "__pnewindex");
            if (!luaL_gettable(L, -2))
            {
                lua_pop(L, 1);
                lua_newtable(L);
                lua_pushvalue(L, -1);
                lua_setfield(L, -3, "__pnewindex");
            }
            lua_remove(L, -2);
        }
        static int get_vtable(lua_State *L)
        {
            lua_pushnumber(L, (double)**(uintptr_t**)lua_touserdata(L, 1));
            return 1;
        }
        static void* topointer(lua_State *L, int idx)
        {
            return *(void**)lua_touserdata(L, idx);
        }
        static int get_ptr(lua_State *L)
        {
            lua_pushnumber(L, (double)*(uintptr_t*)lua_touserdata(L, 1));
            return 1;
        }
        static int set_ptr(lua_State *L)
        {
            *(uintptr_t*)lua_touserdata(L, 1) = (uintptr_t)lua_tonumber(L, 3);
            return 0;
        }
        static int get_prop(lua_State *L)
        {
            auto offset = lua_tointeger(L, lua_upvalueindex(1));
            auto type = lua_tointeger(L, lua_upvalueindex(2));

            auto p = *(uintptr_t*)lua_touserdata(L, 1);

            return lua_pushmemtype(L, MemoryType(type), (void*)(p + offset));
        }
        static int get_prop_struct(lua_State *L)
        {
            auto offset = lua_tointeger(L, lua_upvalueindex(1));

            lua_getfield(L, lua_upvalueindex(2), "__key");
            DynClass* c = static_cast<DynClass *>(lua_touserdata(L, -1));
            lua_pop(L, 1);

            auto p = *(uintptr_t*)lua_touserdata(L, 1);

            if (*(void**)(p + offset) == nullptr)
            {
                lua_pushnil(L);
                return 1;
            }

            auto u = (void**)lua_newuserdata(L, sizeof(void*) * 2);
            *u = *(void**)(p + offset); *(u + 1) = 0;

            push_metatable(L, c, true);
            lua_setmetatable(L, -2);

            return 1;
        }
        static int get_prop_struct_cast(lua_State *L)
        {
            auto offset = lua_tointeger(L, lua_upvalueindex(1));

            lua_getfield(L, lua_upvalueindex(2), "__key");
            DynClass* c = static_cast<DynClass *>(lua_touserdata(L, -1));
            lua_pop(L, 1);

            auto p = *(uintptr_t*)lua_touserdata(L, 1);
            auto u = (void**)lua_newuserdata(L, sizeof(void*) * 2);
            *u = (void*)(p + offset); *(u + 1) = 0;

            push_metatable(L, c, true);
            lua_setmetatable(L, -2);

            return 1;
        }
        static int set_prop(lua_State *L)
        {
            auto offset = lua_tointeger(L, lua_upvalueindex(1));
            auto type = lua_tointeger(L, lua_upvalueindex(2));

            auto p = *(uintptr_t*)lua_touserdata(L, 1);

            lua_pullmemtype(L, MemoryType(type), (void*)(p + offset));
            return 0;
        }
        static int dyn_prop(lua_State *L)
        {
            lua_getfield(L, 1, "__key");
            DynClass* c = static_cast<DynClass *>(lua_touserdata(L, -1));
            lua_pop(L, 1);

            if (lua_isnoneornil(L, 3))
            {
                lua_getglobal(L, "print");
                static char buff[100] = { '\0' };
                snprintf(buff, 100, "[Warning] %s.%s offset is nil, assuming '0'", c->class_name, lua_tolstring(L, 2, nullptr));
                lua_pushstring(L, buff);
                lua_call(L, 1, 0);
            }

            auto flag = lua_tointeger(L, 5);

            if (lua_type(L, 4) == LUA_TTABLE)
            {
                c->push_property_index();
                lua_pushvalue(L, 3);
                lua_pushvalue(L, 4);
                if (flag == 2)
                    lua_pushcclosure(L, get_prop_struct_cast, 2);
                else
                    lua_pushcclosure(L, get_prop_struct, 2);
                lua_setfield(L, -2, lua_tolstring(L, 2, nullptr));
                lua_pop(L, 1);

                if (!flag) {
                    c->push_property_newindex();
                    lua_pushvalue(L, 3);
                    lua_pushinteger(L, -0x20 /**type.ptr**/);
                    lua_pushcclosure(L, set_prop, 2);
                    lua_setfield(L, -2, lua_tolstring(L, 2, nullptr));
                    lua_pop(L, 1);
                }
            } else {
                c->push_property_index();
                lua_pushvalue(L, 3);
                lua_pushvalue(L, 4);
                lua_pushcclosure(L, get_prop, 2);
                lua_setfield(L, -2, lua_tolstring(L, 2, nullptr));
                lua_pop(L, 1);

                if (!flag) {
                    c->push_property_newindex();
                    lua_pushvalue(L, 3);
                    lua_pushvalue(L, 4);
                    lua_pushcclosure(L, set_prop, 2);
                    lua_setfield(L, -2, lua_tolstring(L, 2, nullptr));
                    lua_pop(L, 1);
                }

                c->set_size(lua_tointeger(L, 3) + get_type_size((MemoryType)lua_tointeger(L, 4)));
            }

            return 0;
        }
        static int dyn_call(lua_State *L)
        {
            lua_getfield(L, 1, "__key");
            DynClass* c = static_cast<DynClass *>(lua_touserdata(L, -1));

            auto size = lua_isinteger(L, 2) ? lua_tointeger(L, 2) : c->get_size();

            auto udata = (void*)malloc(size);
            memset(udata, 0, size);
            auto u = (void**)lua_newuserdata(L, sizeof(void*) * 2);
            *u = udata; *(u + 1) = (void*)0xC0FFEE;

            push_metatable(L, c, true);
            lua_setmetatable(L, -2);

            return 1;
        }
        static int dyn_read(lua_State *L)
        {
            auto p = (void**)(uintptr_t)lua_tonumber(L, 2);
            if (p == nullptr || *p == nullptr)
            {
                lua_pushnil(L);
                return 1;
            }

            lua_getfield(L, 1, "__key");
            DynClass* c = static_cast<DynClass *>(lua_touserdata(L, -1));

            auto u = (void**)lua_newuserdata(L, sizeof(void*) * 2);
            *u = *p; *(u + 1) = 0;

            push_metatable(L, c, true);
            lua_setmetatable(L, -2);

            return 1;
        }
        static int dyn_cast(lua_State *L)
        {
            auto p = (void*)(uintptr_t)lua_tonumber(L, 2);
            if (p == 0)
            {
                lua_pushnil(L);
                return 1;
            }

            lua_getfield(L, 1, "__key");
            DynClass* c = static_cast<DynClass *>(lua_touserdata(L, -1));

            auto u = (void**)lua_newuserdata(L, sizeof(void*) * 2);
            *u = p; *(u + 1) = 0;

            push_metatable(L, c, true);
            lua_setmetatable(L, -2);

            return 1;
        }
    };
}
#endif
#endif



#include <functional>


namespace LuaBinding {

    template<class T>
    class helper {
    public:
        static void const* key() { static char value; return &value; }
        static bool push_metatable(lua_State * L) {
            lua_getglobal(L, "__METASTORE");
            if (lua_rawgetp(L, -1, key()))
            {
                lua_remove(L, -2);
                return false;
            }
            lua_pop(L, 1);
            lua_newtable(L);
            lua_pushvalue(L, -1);
            lua_rawsetp(L, -3, key());
            lua_remove(L, -2);
            return true;
        }
    };

    template<typename T>
    class Class {
        lua_State* L = nullptr;
        const char* class_name;
    public:
        Class(lua_State* L, const char* name) : L(L), class_name(name)
        {
            if (!push_metatable())
            {
                lua_pop(L, 1);
                return;
            }

            lua_pushcclosure(L, lua_CGCFunction, 0);
            lua_setfield(L, -2, "__gc");

            lua_pushcclosure(L, lua_CIndexFunction, 0);
            lua_setfield(L, -2, "__index");

            lua_pushcclosure(L, lua_EQFunction, 0);
            lua_setfield(L, -2, "__eq");

            lua_pushcclosure(L, lua_CNewIndexFunction, 0);
            lua_setfield(L, -2, "__newindex");

            lua_pushstring(L, name);
            lua_setfield(L, -2, "__name");

            lua_pushstring(L, name);
            lua_pushcclosure(L, tostring, 1);
            lua_setfield(L, -2, "__tostring");

#ifdef LUABINDING_DYN_CLASSES
            lua_pushcfunction(L, dyn_read);
            lua_setfield(L, -2, "Read");

            lua_pushcfunction(L, dyn_cast);
            lua_setfield(L, -2, "Cast");
#endif

            lua_pop(L, 1);

            push_property_index();
            lua_pushcfunction(L, get_ptr);
            lua_setfield(L, -2, "ptr");
#ifdef LUABINDING_DYN_CLASSES
            lua_pushcfunction(L, get_vtable);
            lua_setfield(L, -2, "vTable");
            lua_pop(L, 1);

            push_property_newindex();
            lua_pushcfunction(L, set_ptr);
            lua_setfield(L, -2, "ptr");
#endif
            lua_pop(L, 1);
        }
    private:
        static int lua_CGCFunction(lua_State* S)
        {
            if (luaL_getmetafield(S, 1, "__cgc"))
            {
                lua_pushvalue(S, 1);
                pcall(S, 1, 0);
            }
            auto u = (void**)lua_touserdata(S, 1);
            if (*(u + 1) == (void*)0xC0FFEE) {
                free(*u);
            }
            return 0;
        }
        static int get_vtable(lua_State *L)
        {
            lua_pushnumber(L, (double)**(uintptr_t**)lua_touserdata(L, 1));
            return 1;
        }
        static int get_ptr(lua_State* L)
        {
            lua_pushnumber(L, (double)*(uintptr_t*)lua_touserdata(L, 1));
            return 1;
        }
        static int set_ptr(lua_State *L)
        {
            *(uintptr_t*)lua_touserdata(L, 1) = (uintptr_t)lua_tonumber(L, 3);
            return 0;
        }
        static int tostring(lua_State* S)
        {
            char asdf[128];
#ifdef ENV32
                snprintf(asdf, sizeof(asdf), "%s{%X}", lua_tostring(S, lua_upvalueindex(1)), (uintptr_t)topointer(S, 1));
#elifdef ENV64
                snprintf(asdf, sizeof(asdf), "%s{%llX}", lua_tostring(S, lua_upvalueindex(1)), (uintptr_t)topointer(S, 1));
#endif
            lua_pushstring(S, asdf);
            return 1;
        }
        static void* topointer(lua_State *L, int idx)
        {
            return *(void**)lua_touserdata(L, idx);
        }
        static int lua_EQFunction(lua_State* S)
        {
            if (!lua_isuserdata(S, 1) || !lua_isuserdata(S, 2)) lua_pushboolean(S, false);
            else
                lua_pushboolean(S, *(uintptr_t*)lua_touserdata(S, 1) == *(uintptr_t*)lua_touserdata(S, 2));
            return 1;
        }
        static int lua_CIndexFunction(lua_State* S)
        {
            if (strcmp(lua_tostring(S, 2), "ptr") != 0
                && strcmp(lua_tostring(S, 2), "valid") != 0
                && luaL_getmetafield(S, 1, "__valid"))
            {
                lua_pushvalue(S, 1);
                lua_call(S, 1, 1);
                if (!lua_toboolean(S, -1))
                {
                    lua_pushvalue(S, 1);
                    if (luaL_getmetafield(S, 1, "__name")) {
                        char asdf[128];
#ifdef ENV32
                            snprintf(asdf, sizeof(asdf), "%s{%X}", lua_tostring(S, -1), (uintptr_t)topointer(S, 1));
#elifdef ENV64
                            snprintf(asdf, sizeof(asdf), "%s{%llX}", lua_tostring(S, -1), (uintptr_t)topointer(S, 1));
#endif
                        lua_pop(S, 1);

                        luaL_error(S, "Tried to access invalid %s (property '%s')", asdf, lua_tostring(S, 2));
                    } else {
                        lua_pop(S, 1);
                        luaL_error(S, "Tried to access invalid unknown object (property '%s')", lua_tostring(S, 2));
                    }
                    return 0; // ??
                }
                lua_pop(S, 1);
            }
            if (luaL_getmetafield(S, 1, "__pindex"))
            {
                lua_pushvalue(S, 2);
                if (luaL_gettable(S, -2))
                {
                    lua_remove(S, -2);
                    lua_insert(S, 1);
                    lua_call(S, lua_gettop(S) - 1, LUA_MULTRET);
                    return lua_gettop(S);
                }
                else lua_pop(S, 1);
            }
            if (luaL_getmetafield(S, 1, "__findex"))
            {
                lua_pushvalue(S, 2);
                if (luaL_gettable(S, -2))
                    return 1;
                else lua_pop(S, 1);
            }
            if (luaL_getmetafield(S, 1, lua_tostring(S, 2)))
                return 1;
            if (luaL_getmetafield(S, 1, "__cindex"))
            {
                lua_insert(S, 1);
                lua_call(S, lua_gettop(S) - 1, LUA_MULTRET);
                return lua_gettop(S);
            }
            return 0;
        }
        static int lua_CNewIndexFunction(lua_State* S)
        {
            if (luaL_getmetafield(S, 1, "__pnewindex"))
            {
                lua_pushvalue(S, 2);
                if (luaL_gettable(S, -2))
                {
                    lua_pushvalue(S, 1);
                    lua_pushvalue(S, 2);
                    lua_pushvalue(S, 3);
                    lua_call(S, 3, 0);
                    return 0;
                }
                else lua_pop(S, 1);
            }
            if (luaL_getmetafield(S, 1, "__cnewindex"))
            {
                lua_pushvalue(S, 1);
                lua_pushvalue(S, 2);
                lua_pushvalue(S, 3);
                lua_call(S, 3, 0);
                return 0;
            }
            lua_getmetatable(S, 1);
            lua_pushvalue(S, 2);
            lua_pushvalue(S, 3);
            lua_settable(S, -3);
            return 0;
        }
    private:
        bool push_metatable()
        {
            return helper<std::remove_pointer_t<std::decay_t<T>>>::push_metatable(L);
        }
        void push_function_index()
        {
            push_metatable();
            lua_pushstring(L, "__findex");
            if (!luaL_gettable(L, -2))
            {
                lua_pop(L, 1);
                lua_newtable(L);
                lua_pushvalue(L, -1);
                lua_setfield(L, -3, "__findex");
            }
            lua_remove(L, -2);
        }
        void push_property_index()
        {
            push_metatable();
            lua_pushstring(L, "__pindex");
            if (!luaL_gettable(L, -2))
            {
                lua_pop(L, 1);
                lua_newtable(L);
                lua_pushvalue(L, -1);
                lua_setfield(L, -3, "__pindex");
            }
            lua_remove(L, -2);
        }
        void push_property_newindex()
        {
            push_metatable();
            lua_pushstring(L, "__pnewindex");
            if (!luaL_gettable(L, -2))
            {
                lua_pop(L, 1);
                lua_newtable(L);
                lua_pushvalue(L, -1);
                lua_setfield(L, -3, "__pnewindex");
            }
            lua_remove(L, -2);
        }
    public:
        template<typename F>
        Class<T>& idx_cfun(F func)
        {
            push_metatable();
            lua_pushstring(L, "__cindex");
            lua_pushcclosure(L, reinterpret_cast<lua_CFunction>(func), 0);
            lua_settable(L, -3);
            lua_pop(L, 1);
            return *this;
        }
        
        template<class... Params>
        Class<T>& ctor()
        {
            auto str = std::string(class_name);
            if (str.find('.') != std::string::npos)
            {
                auto enclosing_table = str.substr(0, str.find('.'));
                auto real_class_name = str.substr(str.find('.') + 1);
                if (!luaL_getglobal(L, enclosing_table.c_str()))
                {
                    lua_pop(L, 1);
                    lua_newtable(L);
                    lua_pushvalue(L, -1);
                    lua_setglobal(L, enclosing_table.c_str());
                }
                lua_getfield(L, -1, real_class_name.c_str());
                if (lua_isnil(L, -1))
                {
                    lua_pop(L, 1);
                    push_metatable();
                }

                lua_createtable(L, 0, 1);
                lua_pushstring(L, "__call");
                lua_pushcclosure(L, TraitsCtor<T, Params...>::f, 0);
                lua_settable(L, -3);
                //lua_pushcclosure(L, lua_CGCProtector, 0);
                //lua_setfield(L, -2, "__newindex");

                lua_setmetatable(L, -2);

                lua_setfield(L, -2, real_class_name.c_str());

                return *this;
            } else {
                if (!luaL_getglobal(L, class_name))
                {
                    lua_pop(L, 1);
                    push_metatable();
                }

                lua_createtable(L, 0, 1);
                lua_pushstring(L, "__call");
                lua_pushcclosure(L, TraitsCtor<T, Params...>::f, 0);
                lua_settable(L, -3);
                //lua_pushcclosure(L, lua_CGCProtector, 0);
                //lua_setfield(L, -2, "__newindex");

                lua_setmetatable(L, -2);

                lua_setglobal(L, class_name);

                return *this;
            }
        }

        Class<T>& make_visible()
        {
            if (!luaL_getglobal(L, class_name))
            {
                lua_pop(L, 1);
                push_metatable();
                lua_setglobal(L, class_name);
            } else lua_pop(L, 1);
            return *this;
        }

        template<typename ...Funcs>
        Class<T>& overload(const char* name, Funcs... functions) {
            push_function_index();
            OverloadedFunction<T, Funcs...>(L, functions...);
            lua_setfield(L, -2, name);
            lua_pop(L, 1);
            return *this;
        }

        template <class F>
        Class<T>& fun(const char* name, F& func)
        {
            push_function_index();
            Function::fun<T>(L, func);
            lua_setfield(L, -2, name);
            lua_pop(L, 1);
            return *this;
        }

        template <class F>
        Class<T>& fun(const char* name, F&& func)
        {
            push_function_index();
            Function::fun<T>(L, func);
            lua_setfield(L, -2, name);
            lua_pop(L, 1);
            return *this;
        }

        template <class F>
        Class<T>& sfun(const char* name, F&& func)
        {
            fun(name, std::forward<F>(func));
            make_visible();
            lua_getglobal(L, class_name);
            push_function_index();
            lua_pushstring(L, name);
            lua_rawget(L, -2);
            lua_remove(L, -2);
            lua_setfield(L, -2, name);
            lua_pop(L, 1);
            return *this;
        }

        template <class F>
        Class<T>& vfun(const char* name, F&& func)
        {
            prop_fun(name, std::forward<F>(func));
            push_metatable();
            lua_pushstring(L, "__valid");
            push_property_index();
            lua_pushstring(L, name);
            lua_rawget(L, -2);
            lua_remove(L, -2);
            lua_settable(L, -3);
            lua_pop(L, 1);
            return *this;
        }

        template <class F>
        Class<T>& vcfun(const char* name, F&& func)
        {
            prop_cfun(name, std::forward<F>(func));
            push_metatable();
            lua_pushstring(L, "__valid");
            push_property_index();
            lua_pushstring(L, name);
            lua_rawget(L, -2);
            lua_remove(L, -2);
            lua_settable(L, -3);
            lua_pop(L, 1);
            return *this;
        }

        template <class F>
        Class<T>& cfun(const char* name, F& func)
        {
            push_function_index();
            Function::cfun<T>(L, func);
            lua_setfield(L, -2, name);
            lua_pop(L, 1);
            return *this;
        }

        template <class F>
        Class<T>& cfun(const char* name, F&& func)
        {
            push_function_index();
            Function::cfun<T>(L, func);
            lua_setfield(L, -2, name);
            lua_pop(L, 1);
            return *this;
        }

        template <class F>
        Class<T>& scfun(const char* name, F&& func)
        {
            cfun(name, std::forward<F>(func));
            make_visible();
            lua_getglobal(L, class_name);
            push_function_index();
            lua_pushstring(L, name);
            lua_rawget(L, -2);
            lua_remove(L, -2);
            lua_setfield(L, -2, name);
            lua_pop(L, 1);
            return *this;
        }

        template <class F>
        Class<T>& meta_fun(const char* name, F func)
        {
            if (strcmp(name, "__gc") == 0) {
                return meta_fun("__cgc", std::forward<decltype(func)>(func));
            }
            using FnType = decltype (func);
            push_metatable();
            Function::fun<T>(L, func);
            lua_setfield(L, -2, name);
            lua_pop(L, 1);
            return *this;
        }

        template <class F>
        Class<T>& meta_cfun(const char* name, F func)
        {
            if (strcmp(name, "__gc") == 0) {
                return meta_cfun("__cgc", std::forward<decltype(func)>(func));
            }
            using func_t = decltype (func);
            push_metatable();
            Function::cfun<T>(L, func);
            lua_setfield(L, -2, name);
            lua_pop(L, 1);
            return *this;
        }

        template<class R>
        Class<T>& prop(const char* name, R(T::* prop))
        {
            using PropType = decltype (prop);

            push_property_index();

            push_property_newindex();
            new (lua_newuserdata(L, sizeof(prop))) PropType(prop);
            lua_pushvalue(L, -1);
            lua_pushcclosure(L, TraitsClassProperty<std::decay_t<R>, std::decay_t<T>>::set, 1);
            lua_setfield(L, -3, name);
            lua_remove(L, -2);

            lua_pushcclosure(L, TraitsClassProperty<std::decay_t<R>, std::decay_t<T>>::get, 1);
            lua_setfield(L, -2, name);
            lua_pop(L, 1);
            return *this;
        }

        template<class R>
        Class<T>& prop_fun(const char* name, R(* get)(), void(* set)(R) = nullptr)
        {
            using get_t = decltype(get);
            using set_t = decltype(set);

            if (set)
            {
                push_property_newindex();
                new (lua_newuserdata(L, sizeof(set))) set_t(set);
                lua_pushcclosure(L, TraitsClassPropertyNFn<R, T>::set, 1);
                lua_setfield(L, -2, name);
                lua_pop(L, 1);
            }

            push_property_index();
            new (lua_newuserdata(L, sizeof(get))) get_t(get);
            lua_pushcclosure(L, TraitsClassPropertyNFn<R, T>::get, 1);
            lua_setfield(L, -2, name);
            lua_pop(L, 1);
            return *this;
        }

        template<class R>
        Class<T>& prop_fun(const char* name, R(T::* get)(), void(T::* set)(R) = nullptr)
        {
            using get_t = decltype (get);
            using set_t = decltype (set);

            if (set)
            {
                push_property_newindex();
                new (lua_newuserdata(L, sizeof(set))) set_t(set);
                lua_pushcclosure(L, TraitsClassPropertyFn<R, T>::set, 1);
                lua_setfield(L, -2, name);
                lua_pop(L, 1);
            }

            push_property_index();
            new (lua_newuserdata(L, sizeof(get))) get_t(get);
            lua_pushcclosure(L, TraitsClassPropertyFn<R, T>::get, 1);
            lua_setfield(L, -2, name);
            lua_pop(L, 1);
            return *this;
        }

        template<class R>
        Class<T>& prop_fun(const char* name, R(T::* get)() const, void(T::* set)(R) const = nullptr)
        {
            using get_t = std::decay_t<decltype (get)>;
            using set_t = std::decay_t<decltype (set)>;

            if (set)
            {
                push_property_newindex();
                new (lua_newuserdata(L, sizeof(set))) set_t(set);
                lua_pushcclosure(L, TraitsClassPropertyFn<R, T>::set, 1);
                lua_setfield(L, -2, name);
                lua_pop(L, 1);
            }

            push_property_index();
            new (lua_newuserdata(L, sizeof(get))) get_t(get);
            lua_pushcclosure(L, TraitsClassPropertyFn<R, T>::get, 1);
            lua_setfield(L, -2, name);
            lua_pop(L, 1);
            return *this;
        }

        template <class R>
        Class<T>& prop_fun(const char* name, std::function<R()> get, std::function<void(R)> set = nullptr)
        {
            using get_t = decltype (get);
            using set_t = decltype (set);

            if (set)
            {
                push_property_newindex();
                new (lua_newuserdata(L, sizeof(set))) set_t(set);
                lua_pushcclosure(L, TraitsClassPropertyNFunFn<R, T>::set, 1);
                lua_setfield(L, -2, name);
                lua_pop(L, 1);
            }

            push_property_index();
            new (lua_newuserdata(L, sizeof(get))) get_t(get);
            lua_pushcclosure(L, TraitsClassPropertyNFunFn<R, T>::get, 1);
            lua_setfield(L, -2, name);
            lua_pop(L, 1);
            return *this;
        }

        template <class R>
        Class<T>& prop_fun(const char* name, std::function<R(T*)> get, std::function<void(T*, R)> set = nullptr)
        {
            using get_t = decltype (get);
            using set_t = decltype (set);

            if (set)
            {
                push_property_newindex();
                new (lua_newuserdata(L, sizeof(set))) set_t(set);
                lua_pushcclosure(L, TraitsClassPropertyFunFn<R, T>::set, 1);
                lua_setfield(L, -2, name);
                lua_pop(L, 1);
            }

            push_property_index();
            new (lua_newuserdata(L, sizeof(get))) get_t(get);
            lua_pushcclosure(L, TraitsClassPropertyFunFn<R, T>::get, 1);
            lua_setfield(L, -2, name);
            lua_pop(L, 1);
            return *this;
        }

        template <class F, class G>
        Class<T>& prop_fun(const char* name, F&& get, G&& set)
        {
            prop_fun(name, static_cast<function_type_t<std::decay_t<F>>>(get), static_cast<function_type_t<std::decay_t<G>>>(set));
            return *this;
        }

        template <class F>
        Class<T>& prop_fun(const char* name, F&& get)
        {
            prop_fun(name, static_cast<function_type_t<std::decay_t<F>>>(get));
            return *this;
        }

        template <class R, class U> requires std::is_integral_v<R>
        Class<T>& prop_cfun(const char* name, R(*get)(U), R(*set)(U) = nullptr)
        {
            using get_t = decltype (get);
            using set_t = decltype (set);

            if (set)
            {
                push_property_newindex();
                new (lua_newuserdata(L, sizeof(set))) set_t(set);
                lua_pushcclosure(L, TraitsClassPropertyNCFun<R, T>::set, 1);
                lua_setfield(L, -2, name);
                lua_pop(L, 1);
            }

            push_property_index();
            new (lua_newuserdata(L, sizeof(get))) get_t(get);
            lua_pushcclosure(L, TraitsClassPropertyNCFun<R, T>::get, 1);
            lua_setfield(L, -2, name);
            lua_pop(L, 1);
            return *this;
        }

        template <class R, class U> requires std::is_integral_v<R>
        Class<T>& prop_cfun(const char* name, R(T::*get)(U), R(T::*set)(U) = nullptr)
        {
            using get_t = decltype (get);
            using set_t = decltype (set);

            if (set)
            {
                push_property_newindex();
                new (lua_newuserdata(L, sizeof(set))) set_t(set);
                lua_pushcclosure(L, TraitsClassPropertyCFun<R, T>::set, 1);
                lua_setfield(L, -2, name);
                lua_pop(L, 1);
            }

            push_property_index();
            new (lua_newuserdata(L, sizeof(get))) get_t(get);
            lua_pushcclosure(L, TraitsClassPropertyCFun<R, T>::get, 1);
            lua_setfield(L, -2, name);
            lua_pop(L, 1);
            return *this;
        }

        template <class R> requires std::is_integral_v<R>
        Class<T>& prop_cfun(const char* name, std::function<R(lua_State*)> get, std::function<R(lua_State*)> set = nullptr)
        {
            using get_t = decltype (get);
            using set_t = decltype (set);

            if (set)
            {
                push_property_newindex();
                new (lua_newuserdata(L, sizeof(set))) set_t(set);
                lua_pushcclosure(L, TraitsClassPropertyNFunLCFn<R, T>::set, 1);
                lua_setfield(L, -2, name);
                lua_pop(L, 1);
            }

            push_property_index();
            new (lua_newuserdata(L, sizeof(get))) get_t(get);
            lua_pushcclosure(L, TraitsClassPropertyNFunLCFn<R, T>::get, 1);
            lua_setfield(L, -2, name);
            lua_pop(L, 1);
            return *this;
        }

        template <class R> requires std::is_integral_v<R>
        Class<T>& prop_cfun(const char* name, std::function<R(T*, lua_State*)> get, std::function<R(T*, lua_State*)> set = nullptr)
        {
            using get_t = decltype (get);
            using set_t = decltype (set);

            if (set)
            {
                push_property_newindex();
                new (lua_newuserdata(L, sizeof(set))) set_t(set);
                lua_pushcclosure(L, TraitsClassPropertyFunLCFn<R, T>::set, 1);
                lua_setfield(L, -2, name);
                lua_pop(L, 1);
            }

            push_property_index();
            new (lua_newuserdata(L, sizeof(get))) get_t(get);
            lua_pushcclosure(L, TraitsClassPropertyFunLCFn<R, T>::get, 1);
            lua_setfield(L, -2, name);
            lua_pop(L, 1);
            return *this;
        }

        template <class R, class U> requires std::is_integral_v<R>
        Class<T>& prop_cfun(const char* name, std::function<R(U)> get, std::function<R(U)> set = nullptr)
        {
            using get_t = decltype (get);
            using set_t = decltype (set);

            if (set)
            {
                push_property_newindex();
                new (lua_newuserdata(L, sizeof(set))) set_t(set);
                lua_pushcclosure(L, TraitsClassPropertyNFunCFn<R, T>::set, 1);
                lua_setfield(L, -2, name);
                lua_pop(L, 1);
            }

            push_property_index();
            new (lua_newuserdata(L, sizeof(get))) get_t(get);
            lua_pushcclosure(L, TraitsClassPropertyNFunCFn<R, T>::get, 1);
            lua_setfield(L, -2, name);
            lua_pop(L, 1);
            return *this;
        }

        template <class R, class U> requires std::is_integral_v<R>
        Class<T>& prop_cfun(const char* name, std::function<R(T*, U)> get, std::function<R(T*, U)> set = nullptr)
        {
            using get_t = decltype (get);
            using set_t = decltype (set);

            if (set)
            {
                push_property_newindex();
                new (lua_newuserdata(L, sizeof(set))) set_t(set);
                lua_pushcclosure(L, TraitsClassPropertyFunLCFn<R, T>::set, 1);
                lua_setfield(L, -2, name);
                lua_pop(L, 1);
            }

            push_property_index();
            new (lua_newuserdata(L, sizeof(get))) get_t(get);
            lua_pushcclosure(L, TraitsClassPropertyFunLCFn<R, T>::get, 1);
            lua_setfield(L, -2, name);
            lua_pop(L, 1);
            return *this;
        }

        template <class F, class G>
        Class<T>& prop_cfun(const char* name, F&& get, G&& set)
        {
            prop_cfun(name, static_cast<function_type_t<std::decay_t<F>>>(get), static_cast<function_type_t<std::decay_t<G>>>(set));
            return *this;
        }

        template <class F>
        Class<T>& prop_cfun(const char* name, F&& get)
        {
            prop_cfun(name, static_cast<function_type_t<std::decay_t<F>>>(get));
            return *this;
        }

        static int dyn_read(lua_State *L)
        {
            auto p = (void**)(uintptr_t)lua_tonumber(L, 2);
            if (p == nullptr || *p == nullptr)
            {
                lua_pushnil(L);
                return 1;
            }

            auto u = (void**)lua_newuserdata(L, sizeof(void*) * 2);
            *u = *p; *(u + 1) = 0;

            helper<std::remove_pointer_t<std::decay_t<T>>>::push_metatable(L);
            lua_setmetatable(L, -2);

            return 1;
        }
        static int dyn_cast(lua_State *L)
        {
            auto p = (void*)(uintptr_t)lua_tonumber(L, 2);
            if (p == 0)
            {
                lua_pushnil(L);
                return 1;
            }


            auto u = (void**)lua_newuserdata(L, sizeof(void*) * 2);
            *u = p; *(u + 1) = 0;

            helper<std::remove_pointer_t<std::decay_t<T>>>::push_metatable(L);
            lua_setmetatable(L, -2);

            return 1;
        }
    };
}




namespace LuaBinding {
    class StackIter
    {
    public:
        typedef std::forward_iterator_tag iterator_category;
        StackIter(lua_State* L, int index) : L(L), index(index) { }
        StackIter operator++() { index++; return *this; }
        std::pair<int, Object> operator*() { return { index, Object(L, index) }; }
        bool operator!=(const StackIter& rhs) { return index != rhs.index; }
    private:
        lua_State* L;
        int index;
    };
}


#include <vector>
#include <stdexcept>

namespace LuaBinding {
    template< class T >
    inline constexpr bool is_state_v = std::is_same<T, State*>::value;

    class State {
        lua_State* L = nullptr;
        bool view = false;

#ifdef LUABINDING_DYN_CLASSES
        std::vector<DynClass*>* get_dynamic_classes() {
            lua_getglobal(L, "__DATASTORE");
            lua_getfield(L, -1, "dynamic_class_store");
            auto p = (std::vector<DynClass*>*)lua_touserdata(L, -1);
            lua_pop(L, 2);
            return p;
        }
#endif

        ObjectRef& globals() {
            static ObjectRef globals = at("_G");
            return globals;
        }

    public:
        State() = default;
        State(bool) {
            L = luaL_newstate();
            luaL_openlibs(L);
            lua_newtable(L);
            lua_setglobal(L, "__METASTORE");
#ifdef LUABINDING_DYN_CLASSES
            lua_newtable(L);
            lua_pushlightuserdata(L, new std::vector<DynClass*>());
            lua_setfield(L, -2, "dynamic_class_store");
            lua_setglobal(L, "__DATASTORE");
#endif
        }
        State(lua_State*L) : L(L), view(true) {
            lua_getglobal(L, "__METASTORE");
            if (lua_isnil(L, -1)) {
                lua_pop(L, 1);
                lua_newtable(L);
                lua_setglobal(L, "__METASTORE");
            }
            else {
                lua_pop(L, 1);
            }
        }
        State(const State& state) : L(state.L), view(true) {
            lua_getglobal(L, "__METASTORE");
            if (lua_isnil(L, -1)) {
                lua_pop(L, 1);
                lua_newtable(L);
                lua_setglobal(L, "__METASTORE");
            }
            else {
                lua_pop(L, 1);
            }
        }

        template <class... Ts>
        State(std::tuple<Ts...> const& tup)
            : State(tup, std::index_sequence_for<Ts...>{})
        {
        }

        template <class Tuple, size_t... Is>
        State(Tuple const& tup, std::index_sequence<Is...> )
            : State(std::get<Is>(tup)...)
        {
        }

        ~State() {
            if (view) {
                if (globals().valid(this))
                    globals().pop();
            } else {
                globals().invalidate();
#ifdef LUABINDING_DYN_CLASSES
                for (auto& c : *get_dynamic_classes())
                    delete c;
                get_dynamic_classes()->clear();
                delete get_dynamic_classes();
#endif
                lua_close(L);
                L = nullptr;
            }
        }

        lua_State* lua_state() {
            return L;
        }

        lua_State* lua_state() const {
            return L;
        }

        std::vector<Object> args()
        {
            std::vector<Object> result = {};
            for (auto i = 1; i <= lua_gettop(L); i++)
                result.emplace_back(L, i);
            return result;
        }

        template<class T>
        Class<T> addClass(const char* name)
        {
            return Class<T>(L, name);
        }

#ifdef LUABINDING_DYN_CLASSES
        int addDynClass(const char* name)
        {
            get_dynamic_classes()->push_back(new DynClass(this->L, name));
            return 1;
        }
#endif

        Environment addEnv()
        {
            return Environment(L);
        }

        void pop(int n = 1)
        {
            lua_pop(L, n);
        }

        template<class ...P>
        int push(P&& ...p)
        {
            (void)std::initializer_list<int>{ detail::push(L, p)... };
            return sizeof...(p);
        }

        template<class T, class ...Params> requires std::is_constructible_v<T>
        T* alloc(Params... params)
        {
            auto t = new (malloc(sizeof(T))) T(params...);
            auto u = (void**)lua_newuserdata(L, sizeof(void*) * 2);
            *u = t; *(u + 1) = (void*)0xC0FFEE;
            helper<std::remove_pointer_t<std::decay_t<T>>>::push_metatable(L);
            lua_setmetatable(L, -2);
            return t;
        }

        template<class T, class ...Params> requires (!std::is_constructible_v<T>)
        T* alloc()
        {
            auto t = (T*)malloc(sizeof(T));
            memset(t, 0, sizeof(T));
            auto u = (void**)lua_newuserdata(L, sizeof(void*) * 2);
            *u = t; *(u + 1) = (void*)0xC0FFEE;
            helper<std::remove_pointer_t<std::decay_t<T>>>::push_metatable(L);
            lua_setmetatable(L, -2);
            return t;
        }

        template <class F>
        void fun(F& func)
        {
            Function::fun<void>(L, func);
        }

        template <class F>
        void fun(const char* name, F& func)
        {
            auto str = std::string(name);
            if (str.find('.') != std::string::npos)
            {
                auto enclosing_table = str.substr(0, str.find('.'));
                auto real_class_name = str.substr(str.find('.') + 1);
                if (!luaL_getglobal(L, enclosing_table.c_str()))
                {
                    lua_pop(L, 1);
                    lua_newtable(L);
                    lua_pushvalue(L, -1);
                    lua_setglobal(L, enclosing_table.c_str());
                }
                fun(func);
                lua_setfield(L, -2, real_class_name.c_str());
                lua_pop(L, 1);
            } else {
                fun(func);
                lua_setglobal(L, name);
            }
        }

        template <class F>
        void fun(F&& func)
        {
            Function::fun<void>(L, func);
        }

        template <class F>
        void fun(const char* name, F&& func)
        {
            auto str = std::string(name);
            if (str.find('.') != std::string::npos)
            {
                auto enclosing_table = str.substr(0, str.find('.'));
                auto real_class_name = str.substr(str.find('.') + 1);
                if (!luaL_getglobal(L, enclosing_table.c_str()))
                {
                    lua_pop(L, 1);
                    lua_newtable(L);
                    lua_pushvalue(L, -1);
                    lua_setglobal(L, enclosing_table.c_str());
                }
                fun(func);
                lua_setfield(L, -2, real_class_name.c_str());
                lua_pop(L, 1);
            } else {
                fun(func);
                lua_setglobal(L, name);
            }
        }

        template <class F>
        void cfun(F& func)
        {
            Function::cfun<void>(L, func);
        }

        template <class F>
        void cfun(const char* name, F& func)
        {
            auto str = std::string(name);
            if (str.find('.') != std::string::npos)
            {
                auto enclosing_table = str.substr(0, str.find('.'));
                auto real_class_name = str.substr(str.find('.') + 1);
                if (!luaL_getglobal(L, enclosing_table.c_str()))
                {
                    lua_pop(L, 1);
                    lua_newtable(L);
                    lua_pushvalue(L, -1);
                    lua_setglobal(L, enclosing_table.c_str());
                }
                cfun(func);
                lua_setfield(L, -2, real_class_name.c_str());
                lua_pop(L, 1);
            } else {
                cfun(func);
                lua_setglobal(L, name);
            }
        }

        template <class F>
        void cfun(F&& func)
        {
            Function::cfun<void>(L, func);
        }

        template <class F>
        void cfun(const char* name, F&& func)
        {
            auto str = std::string(name);
            if (str.find('.') != std::string::npos)
            {
                auto enclosing_table = str.substr(0, str.find('.'));
                auto real_class_name = str.substr(str.find('.') + 1);
                if (!luaL_getglobal(L, enclosing_table.c_str()))
                {
                    lua_pop(L, 1);
                    lua_newtable(L);
                    lua_pushvalue(L, -1);
                    lua_setglobal(L, enclosing_table.c_str());
                }
                cfun(func);
                lua_setfield(L, -2, real_class_name.c_str());
                lua_pop(L, 1);
            } else {
                cfun(func);
                lua_setglobal(L, name);
            }
        }

        template<typename ...Funcs>
        void overload(const char* name, Funcs... functions) {
            OverloadedFunction<void, Funcs...>(L, functions...);
            lua_setglobal(L, name);
        }

        int exec(const char* code, int argn = 0, int nres = 0)
        {
            if (luaL_loadstring(L, code) || LuaBinding::pcall(L, argn, nres))
            {
#ifndef NOEXCEPTIONS
                    throw std::exception(lua_tostring(L, -1));
#endif
            }
            return lua_gettop(L);
        }

        int exec(Environment env, const char* code, int argn = 0, int nres = 0)
        {
            if (luaL_loadstring(L, code) || env.pcall(argn, nres))
            {
#ifndef NOEXCEPTIONS
                    throw std::exception(lua_tostring(L, -1));
#endif
            }
            return lua_gettop(L);
        }

        int exec(std::vector<char>& code, int argn = 0, int nres = 0, const char* name = "=")
        {
            if (luaL_loadbuffer(L, code.data(), code.size(), name) || LuaBinding::pcall(L, argn, nres))
            {
#ifndef NOEXCEPTIONS
                    throw std::exception(lua_tostring(L, -1));
#endif
            }
            return lua_gettop(L);
        }

        int exec(Environment env, std::vector<char>& code, int argn = 0, int nres = 0, const char* name = "=")
        {
            if (luaL_loadbuffer(L, code.data(), code.size(), name) || env.pcall(argn, nres))
            {
#ifndef NOEXCEPTIONS
                    throw std::exception(lua_tostring(L, -1));
#endif
            }
            return lua_gettop(L);
        }

        static int base64_write(lua_State* L, unsigned char* str, size_t len, struct luaL_Buffer* buf)
        {
            luaL_addlstring(buf, (const char*)str, len);
            return 0;
        }

        std::string dump(const char* file)
        {
            if (luaL_loadfile(L, file))
            {
#ifndef NOEXCEPTIONS
                    throw std::exception(lua_tostring(L, -1));
#endif
            }

            luaL_Buffer buf;
            luaL_buffinit(L, &buf);
#if LUA_VERSION_NUM >= 503
            if (!lua_dump(L, (lua_Writer)base64_write, &buf, 1))
#else
            if (!lua_dump(L, (lua_Writer)base64_write, &buf))
#endif
            {
                luaL_pushresult(&buf);
                size_t len = 0;
                auto r = std::string(const_cast<char *>(lua_tolstring(L, -1, &len)), len);
                lua_pop(L, 2);
                return r;
            }
#ifndef NOEXCEPTIONS
                    throw std::exception(lua_tostring(L, -1));
#endif
        }

        ObjectRef dump(std::vector<char>& code, std::string fname)
        {
            if (luaL_loadbuffer(L, code.data(), code.size(), ("=" + fname).c_str()))
            {
#ifndef NOEXCEPTIONS
                    throw std::exception(lua_tostring(L, -1));
#endif
            }

            luaL_Buffer buf;
            luaL_buffinit(L, &buf);
#if LUA_VERSION_NUM >= 503
            if (!lua_dump(L, (lua_Writer)base64_write, &buf, 1))
#else
            if (!lua_dump(L, (lua_Writer)base64_write, &buf))
#endif
            {
                luaL_pushresult(&buf);
                lua_remove(L, -2);
                return ObjectRef(L, -1, false);
            }
#ifndef NOEXCEPTIONS
                    throw std::exception(lua_tostring(L, -1));
#endif
        }

        int n() {
            return lua_gettop(L);
        }

        template<typename R, bool C = false, typename ...Params> requires (sizeof...(Params) == 0 || !std::is_same_v<std::tuple_element_t<0, std::tuple<Params...>>, Environment>)
        R call(Params... param) {
            if constexpr (sizeof...(param) > 0)
                (void)std::initializer_list<int>{ push(param...) };
            if constexpr (std::is_same_v<void, R>) {
                if (LuaBinding::pcall(L, sizeof...(param), 0))
                {
#ifndef NOEXCEPTIONS
                    throw std::exception(lua_tostring(L, -1));
#endif
                }
            } else if constexpr (std::is_same_v<Object, R>) {
                if (LuaBinding::pcall(L, sizeof...(param), 1))
                {
#ifndef NOEXCEPTIONS
                    throw std::exception(lua_tostring(L, -1));
#endif
                }
                return Object(L, -1);
            } else if constexpr (std::is_same_v<ObjectRef, R>) {
                if (LuaBinding::pcall(L, sizeof...(param), 1))
                {
#ifndef NOEXCEPTIONS
                    throw std::exception(lua_tostring(L, -1));
#endif
                }
                return ObjectRef(L, -1);
            } else {
                if (LuaBinding::pcall(L, sizeof...(param), 1))
                {
#ifndef NOEXCEPTIONS
                    throw std::exception(lua_tostring(L, -1));
#endif
                }
                if constexpr(C) {
                    auto result = LuaBinding::detail::get<R>(L, -1);
                    lua_pop(L, 1);
                    return result;
                } else
                    return LuaBinding::detail::get<R>(L, -1);
            }
        }

        template<int R, typename ...Params> requires (sizeof...(Params) == 0 || !std::is_same_v<std::tuple_element_t<0, std::tuple<Params...>>, Environment>)
        void call(Params... param) {
            if constexpr (sizeof...(param) > 0)
                (void)std::initializer_list<int>{ detail::push(L, param)... };
            if (LuaBinding::pcall(L, sizeof...(param), R))
            {
#ifndef NOEXCEPTIONS
                    throw std::exception(lua_tostring(L, -1));
#endif
            }
        }

        template<typename R, typename Env, typename ...Params> requires (std::is_same_v<Env, Environment>)
        R call(Env env, Params... param) {
            if constexpr (sizeof...(param) > 0)
                (void)std::initializer_list<int>{ push(param...) };
            if constexpr (std::is_same_v<void, R>) {
                if (env.pcall(sizeof...(param), 0))
                {
#ifndef NOEXCEPTIONS
                    throw std::exception(lua_tostring(L, -1));
#endif
                }
            } else if constexpr (std::is_same_v<Object, R>) {
                if (env.pcall(sizeof...(param), 1))
                {
#ifndef NOEXCEPTIONS
                    throw std::exception(lua_tostring(L, -1));
#endif
                }
                return Object(L, -1);
            } else if constexpr (std::is_same_v<ObjectRef, R>) {
                if (env.pcall(sizeof...(param), 1))
                {
#ifndef NOEXCEPTIONS
                    throw std::exception(lua_tostring(L, -1));
#endif
                }
                return ObjectRef(L, -1);
            } else {
                if (env.pcall(sizeof...(param), 1))
                {
#ifndef NOEXCEPTIONS
                    throw std::exception(lua_tostring(L, -1));
#endif
                }
                return LuaBinding::detail::get<R>(lua_state(), -1);
            }
        }

        template<int R, typename Env, typename ...Params> requires (std::is_same_v<Env, Environment>)
        void call(Env env, Params... param) {
            if constexpr (sizeof...(param) > 0)
                (void)std::initializer_list<int>{ detail::push(L, param)... };
            if (env.pcall(sizeof...(param), R))
            {
#ifndef NOEXCEPTIONS
                    throw std::exception(lua_tostring(L, -1));
#endif
            }
        }

        operator lua_State* () const
        {
            return L;
        }

        operator lua_State* ()
        {
            return L;
        }

        template<class ...Params>
        void error(const char* format, Params... p)
        {
            luaL_error(L, format, std::forward<Params...>(p)...);
        }

        template<class ...Params>
        void error_if(bool cond, const char* format, Params... p)
        {
            if (cond) luaL_error(L, format, std::forward<Params...>(p)...);
        }

        Object at(int index = -1)
        {
            if (index > 0)
            {
#ifndef NOEXCEPTIONS
                if (index > lua_gettop(L))
                    throw std::out_of_range("invalid stack subscript");
#endif
                return { L, index };
            } else if (index < 0) {
                auto top = lua_gettop(L);
#ifndef NOEXCEPTIONS
                if (-index > top)
                    throw std::out_of_range("invalid stack subscript");
#endif
                return { L, top + 1 + index };
            }
#ifndef NOEXCEPTIONS
            throw std::out_of_range("wat");
#endif
            return { L, 0 };
        }

        ObjectRef at(const char* idx)
        {
            lua_getglobal(L, idx);
            return ObjectRef(L, -1, false);
        }

        Object top()
        {
            return Object(L, n());
        }

        Object front()
        {
            return Object(L, 1);
        }

        ObjectRef table(const char* idx)
        {
            lua_getglobal(L, idx);
            if (lua_type(L, -1) > 0)
            {
                return ObjectRef(L, -1, false);
            } else {
                lua_pop(L, 1);
                lua_newtable(L);
                lua_pushvalue(L, -1);
                lua_setglobal(L, idx);
                return ObjectRef(L, -1, false);
            }
        }

        Object operator[](int index)
        {
            if (index > 0)
            {
#ifndef NOEXCEPTIONS
                if (index > lua_gettop(L))
                    throw std::out_of_range("invalid stack subscript");
#endif
                return { L, index };
            } else if (index < 0) {
                auto top = lua_gettop(L);
#ifndef NOEXCEPTIONS
                if (-index > top)
                    throw std::out_of_range("invalid stack subscript");
#endif
                return { L, top + 1 + index };
            }
#ifndef NOEXCEPTIONS
            throw std::out_of_range("wat");
#endif
            return { L, 0 };
        }

        IndexProxy operator[](const char* idx)
        {
            return IndexProxy(globals(), idx);
        }

        template <class T>
        void global(const char* name, T thing)
        {
            auto str = std::string(name);
            if (str.find('.') != std::string::npos)
            {
                auto enclosing_table = str.substr(0, str.find('.'));
                auto real_class_name = str.substr(str.find('.') + 1);
                if (!luaL_getglobal(L, enclosing_table.c_str()))
                {
                    lua_pop(L, 1);
                    lua_newtable(L);
                    lua_pushvalue(L, -1);
                    lua_setglobal(L, enclosing_table.c_str());
                }
                push(thing);
                lua_setfield(L, -2, real_class_name.c_str());
                lua_pop(L, 1);
            } else {
                push(thing);
                lua_setglobal(L, name);
            }
        }

        void set_field(int idx, const char* name)
        {
            lua_setfield(L, idx, name);
        }

        StackIter begin()
        {
            return StackIter(L, 1);
        }
        StackIter end()
        {
            return StackIter(L, lua_gettop(L) + 1);
        }
        StackIter begin() const
        {
            return StackIter(L, 1);
        }
        StackIter end() const
        {
            return StackIter(L, lua_gettop(L) + 1);
        }
    };
}



#include <tuple>
#include <type_traits>

namespace LuaBinding {
    template <typename T>
    struct is_boolean : std::is_same<bool, T> {};
    template <typename T>
    inline constexpr bool is_boolean_v = is_boolean<T>::value;
    template< class T >
    struct is_void : std::is_same<void, typename std::remove_cv<T>::type> {};

    template<typename T>
    concept is_constructible_from_nullptr = requires() {
        { T(nullptr) };
    };

    template<typename T>
    concept is_default_constructible = requires() {
        { T() };
    };

    template <std::size_t I = 0, typename ... Ts>
    void assign_tup(lua_State* L, std::tuple<Ts...> &tup, int J = 0) {
        if constexpr (I == sizeof...(Ts)) {
            return;
        } else {
            using Type = typename std::tuple_element<I, std::tuple<Ts...>>::type;

            if constexpr (std::is_same_v<Type, lua_State*>) {
                std::get<I>(tup) = L;
                assign_tup<I + 1>(L, tup, J - 1);
            }
            else if constexpr (std::is_same_v<Type, State>) {
                std::get<I>(tup) = State(L);
                assign_tup<I + 1>(L, tup, J - 1);
            }
            else if constexpr (std::is_same_v<Type, Environment>) {
                std::get<I>(tup) = Environment(L, true);
                assign_tup<I + 1>(L, tup, J - 1);
            }
            else if constexpr (std::is_same_v<Type, ObjectRef> && !std::is_same_v<Type, Object>) {
                std::get<I>(tup) = ObjectRef(L, I + 1 + J);
                assign_tup<I + 1>(L, tup, J);
            }
            else if constexpr (!std::is_same_v<Type, ObjectRef> && std::is_same_v<Type, Object>) {
                std::get<I>(tup) = Object(L, I + 1 + J);
                assign_tup<I + 1>(L, tup, J);
            }
            else if constexpr (!detail::is_pushable<Type>) {
                if (!StackClass<Type>::is(L, I + 1 + J))
                    if (lua_isnoneornil(L, I + 1 + J))
                        if constexpr (std::is_convertible_v<Type, void*>)
                        {
                            std::get<I>(tup) = (Type)nullptr;
                        }
                        else
                        {
                            luaL_typeerror(L, I + 1, StackClass<Type>::type_name(L));
                        }
                    else
                    {
                        luaL_typeerror(L, I + 1, StackClass<Type>::type_name(L));
                    }
                else {
                    std::get<I>(tup) = (Type)StackClass<Type>::get(L, I + 1 + J);
                }
                assign_tup<I + 1>(L, tup, J);
            }
            else if constexpr (detail::is_pushable<Type>) {
                if (!Stack<Type>::is(L, I + 1 + J))
                    if (lua_isnoneornil(L, I + 1 + J))
                        if constexpr (is_constructible_from_nullptr<Type>)
                            std::get<I>(tup) = Type(nullptr);
                        else if constexpr (is_default_constructible<Type>)
                            std::get<I>(tup) = Type();
                        else
                            luaL_typeerror(L, I + 1 + J, Stack<Type>::type_name(L));
                    else
                        luaL_typeerror(L, I + 1 + J, Stack<Type>::type_name(L));
                else
                {
                    std::get<I>(tup) = (Type)Stack<Type>::get(L, I + 1 + J);
                }
                assign_tup<I + 1>(L, tup, J);
            }
            else static_assert(sizeof(Type) > 0, "How");
        }
    }

    template <class R, class... Params>
    class Traits {
        using ParamList = std::tuple<std::decay_t<Params>...>;
    public:
        static int f(lua_State* L) {
            auto fnptr = reinterpret_cast <R(*)(Params...)> (lua_touserdata(L, lua_upvalueindex(1)));
            ParamList params;
            assign_tup(L, params);
            if constexpr (std::is_void_v<R>) {
                std::apply(fnptr, params);
                return 0;
            } else {
                R result = std::apply(fnptr, params);
                return detail::push(L, result);
            }
        }
    };

    namespace detail {
        template <class T, class Tuple, std::size_t... I>
        constexpr T* make_from_tuple_impl(void* mem, Tuple&& t, std::index_sequence<I...>)
        {
            return new (mem) T(std::get<I>(std::forward<Tuple>(t))...);
        }
    }

    template <class T, class Tuple>
    constexpr T* make_from_tuple(void* mem, Tuple&& t)
    {
        return detail::make_from_tuple_impl<T>(mem,
            std::forward<Tuple>(t),
            std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<Tuple>>>{});
    }

    template <class T, class... Params>
    class TraitsCtor {
        using ParamList = std::tuple<std::decay_t<Params>...>;
    public:
        static int f(lua_State* L) {
            ParamList params;
            assign_tup(L, params, 1);
            auto m = (void*)malloc(sizeof(T));
            make_from_tuple<T>(m, params);
            auto u = (void**)lua_newuserdata(L, sizeof(void*) * 2);
            *u = (void*)m; *(u + 1) = (void*)0xC0FFEE;
            lua_pushvalue(L, 1);
            lua_setmetatable(L, -2);
            return 1;
        }
    };

    template <class R, class... Params>
    class TraitsSTDFunction {
        using ParamList = std::tuple<std::decay_t<Params>...>;
    public:
        static int f(lua_State* L) {
            auto fnptr = *reinterpret_cast <std::function<R(Params...)>*> (lua_touserdata(L, lua_upvalueindex(1)));
            ParamList params;
            assign_tup(L, params);
            if constexpr (std::is_void_v<R>) {
                std::apply(*fnptr, params);
                return 0;
            } else {
                R result = std::apply(*fnptr, params);
                return detail::push(L, result);
            }
        }
    };

    class TraitsCFunc {
    public:
        static int f(lua_State* L) {
            auto fnptr = reinterpret_cast <int(*)(State)> (lua_touserdata(L, lua_upvalueindex(1)));
            return fnptr(State(L));
        }
    };

    template <class R, class T, class... Params>
    class TraitsClass {
        using ParamList = std::tuple<T*, std::decay_t<Params>...>;
    public:
        static int f(lua_State* L) {
            auto fnptr = *static_cast <R(T::**)(Params...)> (lua_touserdata(L, lua_upvalueindex(1)));
            ParamList params;
            assign_tup(L, params);
            if constexpr (std::is_void_v<R>) {
                std::apply(fnptr, params);
                return 0;
            } else {
                return detail::push(L, std::apply(fnptr, params));
            }
        }
    };

    template <class R, class T, class... Params>
    class TraitsFunClass {
        using ParamList = std::tuple<std::decay_t<Params>...>;
    public:
        static int f(lua_State* L) {
            auto fnptr = *static_cast <std::function<R(Params...)>*> (lua_touserdata(L, lua_upvalueindex(1)));
            ParamList params;
            assign_tup(L, params);
            if constexpr (std::is_void_v<R>) {
                std::apply(fnptr, params);
                return 0;
            } else {
                R result = std::apply(fnptr, params);
                return detail::push(L, result);
            }
        }
    };

    template <class R, class T, class... Params>
    class TraitsNClass {
        using ParamList = std::tuple<std::decay_t<Params>...>;
    public:
        static int f(lua_State* L) {
            auto fnptr = *static_cast <R(**)(Params...)> (lua_touserdata(L, lua_upvalueindex(1)));
            ParamList params;
            assign_tup(L, params);
            if constexpr (std::is_void_v<R>) {
                std::apply(fnptr, params);
                return 0;
            } else {
                R result = std::apply(fnptr, params);
                return detail::push(L, result);
            }
        }
    };
    template <class T>
    class TraitsClassNCFunc {
    public:
        static int f(lua_State* L) {
            auto fnptr = reinterpret_cast <int(*)(State)> (lua_touserdata(L, lua_upvalueindex(1)));
            return fnptr(State(L));
        }
    };
    template <class T>
    class TraitsClassFunNCFunc {
    public:
        static int f(lua_State* L) {
            auto fnptr = *reinterpret_cast <std::function<int(State)>*> (lua_touserdata(L, lua_upvalueindex(1)));
            return fnptr(State(L));
        }
    };
    template <class T>
    class TraitsClassCFunc {
    public:
        static int f(lua_State* L) {
            auto t = StackClass<T*>::get(L, 1);
            auto fnptr = reinterpret_cast <int(T::**)(State)> (lua_touserdata(L, lua_upvalueindex(1)));
            return (t->**fnptr)(State(L));
        }
    };
    template <class T>
    class TraitsClassFunCFunc {
    public:
        static int f(lua_State* L) {
            auto t = StackClass<T*>::get(L, 1);
            auto fnptr = *reinterpret_cast <std::function<int(T*, State)>*> (lua_touserdata(L, lua_upvalueindex(1)));
            return fnptr(t, State(L));
        }
    };
    template <class T>
    class TraitsClassLCFunc {
        using func = int(T::*)(lua_State*);
    public:
        static int f(lua_State* L) {
            auto t = StackClass<T*>::get(L, 1);
            auto fnptr = (func*)lua_touserdata(L, lua_upvalueindex(1));
            return (t->**fnptr)(L);
        }
    };
    template <class T>
    class TraitsClassFunLCFunc {
        using func = int(T::*)(lua_State*);
    public:
        static int f(lua_State* L) {
            auto t = StackClass<T*>::get(L, 1);
            auto fnptr = *reinterpret_cast <std::function<int(T*, lua_State*)>*> (lua_touserdata(L, lua_upvalueindex(1)));
            return fnptr(t, L);
        }
    };
    template <class T>
    class TraitsClassFunNLCFunc {
    public:
        static int f(lua_State* L) {
            auto fnptr = *reinterpret_cast <std::function<int(lua_State*)>*> (lua_touserdata(L, lua_upvalueindex(1)));
            return fnptr(L);
        }
    };

    template <class R, class T>
    class TraitsClassProperty {
        using prop = R(T::*);
    public:
        static int set(lua_State* L) {
            auto t = StackClass<T*>::get(L, 1);
            auto p = (prop*)lua_touserdata(L, lua_upvalueindex(1));
            t->**p = detail::get<R>(L, 3);
            return 0;
        }
        static int get(lua_State* L) {
            auto t = StackClass<T*>::get(L, 1);
            auto p = (prop*)lua_touserdata(L, lua_upvalueindex(1));
            return detail::push(L, t->**p);
        }
    };

    template <class R, class T>
    class TraitsClassPropertyNFn {
        using set_t = void(*)(R);
        using get_t = R(*)();
    public:
        static int set(lua_State* L) {
            auto p = *(set_t*)lua_touserdata(L, lua_upvalueindex(1));
            p(detail::get<R>(L, 3));
            return 0;
        }
        static int get(lua_State* L) {
            auto p = *(get_t*)lua_touserdata(L, lua_upvalueindex(1));
            return detail::push(L, p());
        }
    };

    template <class R, class T>
    class TraitsClassPropertyFn {
        using set_t = void(T::*)(R);
        using get_t = R(T::*)();
    public:
        static int set(lua_State* L) {
            auto t = StackClass<T*>::get(L, 1);
            auto p = (set_t*)lua_touserdata(L, lua_upvalueindex(1));
            (t->**p)(detail::get<R>(L, 3));
            return 0;
        }
        static int get(lua_State* L) {
            auto t = StackClass<T*>::get(L, 1);
            auto p = (get_t*)lua_touserdata(L, lua_upvalueindex(1));
            return detail::push(L, (t->**p)());
        }
    };

    template <class R, class T>
    class TraitsClassPropertyNFunFn {
        using set_t = std::function<void(R)>;
        using get_t = std::function<R()>;
    public:
        static int set(lua_State* L) {
            auto p = *(set_t*)lua_touserdata(L, lua_upvalueindex(1));
            p(detail::get<R>(L, 3));
            return 0;
        }
        static int get(lua_State* L) {
            auto p = *(get_t*)lua_touserdata(L, lua_upvalueindex(1));
            return detail::push(L, p());
        }
    };

    template <class R, class T>
    class TraitsClassPropertyFunFn {
        using set_t = std::function<void(T*, R)>;
        using get_t = std::function<R(T*)>;
    public:
        static int set(lua_State* L) {
            auto t = StackClass<T*>::get(L, 1);
            auto p = *(set_t*)lua_touserdata(L, lua_upvalueindex(1));
            p(t, detail::get<R>(L, 3));
            return 0;
        }
        static int get(lua_State* L) {
            auto t = StackClass<T*>::get(L, 1);
            auto p = *(get_t*)lua_touserdata(L, lua_upvalueindex(1));
            detail::push(L, p(t));
            return 1;
        }
    };

    template <class R, class T>
    class TraitsClassPropertyNFunCFn {
        using set_t = std::function<int(State)>;
        using get_t = std::function<int(State)>;
    public:
        static int set(lua_State* L) {
            auto fnptr = *reinterpret_cast <std::function<int(State)>*> (lua_touserdata(L, lua_upvalueindex(1)));
            return fnptr(State(L));
        }
        static int get(lua_State* L) {
            auto fnptr = *reinterpret_cast <std::function<int(State)>*> (lua_touserdata(L, lua_upvalueindex(1)));
            return fnptr(State(L));
        }
    };

    template <class R, class T>
    class TraitsClassPropertyNFunLCFn {
        using set_t = std::function<int(lua_State*)>;
        using get_t = std::function<int(lua_State*)>;
    public:
        static int set(lua_State* L) {
            auto p = *(set_t*)lua_touserdata(L, lua_upvalueindex(1));
            auto fnptr = *reinterpret_cast <std::function<int(lua_State*)>*> (lua_touserdata(L, lua_upvalueindex(1)));
            return fnptr(L);
        }
        static int get(lua_State* L) {
            auto p = *(get_t*)lua_touserdata(L, lua_upvalueindex(1));
            auto fnptr = *reinterpret_cast <std::function<int(lua_State*)>*> (lua_touserdata(L, lua_upvalueindex(1)));
            return fnptr(L);
        }
    };

    template <class R, class T>
    class TraitsClassPropertyFunLCFn {
        using set_t = std::function<int(T*, lua_State*)>;
        using get_t = std::function<int(T*, lua_State*)>;
    public:
        static int set(lua_State* L) {
            auto t = StackClass<T*>::get(L, 1);
            auto p = *(set_t*)lua_touserdata(L, lua_upvalueindex(1));
            auto fnptr = *reinterpret_cast <std::function<int(T*, lua_State*)>*> (lua_touserdata(L, lua_upvalueindex(1)));
            return fnptr(t, L);
        }
        static int get(lua_State* L) {
            auto t = StackClass<T*>::get(L, 1);
            auto p = *(get_t*)lua_touserdata(L, lua_upvalueindex(1));
            auto fnptr = *reinterpret_cast <std::function<int(T*, lua_State*)>*> (lua_touserdata(L, lua_upvalueindex(1)));
            return fnptr(t, L);
        }
    };

    template <class R, class T>
    class TraitsClassPropertyNFunC {
        using set_t = std::function<int(State)>;
        using get_t = std::function<int(State)>;
    public:
        static int set(lua_State* L) {
            auto p = *(set_t*)lua_touserdata(L, lua_upvalueindex(1));
            auto fnptr = *reinterpret_cast <std::function<int(State)>*> (lua_touserdata(L, lua_upvalueindex(1)));
            return fnptr(State(L));
        }
        static int get(lua_State* L) {
            auto p = *(get_t*)lua_touserdata(L, lua_upvalueindex(1));
            auto fnptr = *reinterpret_cast <std::function<int(State)>*> (lua_touserdata(L, lua_upvalueindex(1)));
            return fnptr(State(L));
        }
    };

    template <class R, class T>
    class TraitsClassPropertyFunC {
        using set_t = std::function<int(T*, State)>;
        using get_t = std::function<int(T*, State)>;
    public:
        static int set(lua_State* L) {
            auto t = StackClass<T*>::get(L, 1);
            auto fnptr = *reinterpret_cast <std::function<int(T*, State)>*> (lua_touserdata(L, lua_upvalueindex(1)));
            return fnptr(t, State(L));
        }
        static int get(lua_State* L) {
            auto t = StackClass<T*>::get(L, 1);
            auto fnptr = *reinterpret_cast <std::function<int(T*, State)>*> (lua_touserdata(L, lua_upvalueindex(1)));
            return fnptr(t, State(L));
        }
    };

    template <class R, class T>
    class TraitsClassPropertyNCFun {
        using set_t = int(*)(State);
        using get_t = int(*)(State);
    public:
        static int set(lua_State* L) {
            auto fnptr = *reinterpret_cast <set_t*> (lua_touserdata(L, lua_upvalueindex(1)));
            return fnptr(State(L));
        }
        static int get(lua_State* L) {
            auto fnptr = *reinterpret_cast <get_t*> (lua_touserdata(L, lua_upvalueindex(1)));
            return fnptr(State(L));
        }
    };

    template <class R, class T>
    class TraitsClassPropertyCFun {
        using set_t = int(T::*)(State);
        using get_t = int(T::*)(State);
    public:
        static int set(lua_State* L) {
            auto t = StackClass<T*>::get(L, 1);
            auto fnptr = reinterpret_cast <set_t*> (lua_touserdata(L, lua_upvalueindex(1)));
            return (t->**fnptr)(State(L));
        }
        static int get(lua_State* L) {
            auto t = StackClass<T*>::get(L, 1);
            auto fnptr = reinterpret_cast <get_t*> (lua_touserdata(L, lua_upvalueindex(1)));
            return (t->**fnptr)(State(L));
        }
    };
}





namespace LuaBinding {
    static Object Nil = Object();
    static ObjectRef NilRef = ObjectRef();
}
