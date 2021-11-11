#pragma once
#include "Stack.h"
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

    class State;

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

        template <class T, class F>
        void fun(lua_State *L, F&& func)
        {
            fun<T>(L, static_cast<function_type_t<std::decay_t<F>>>(func));
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
            lua_pushcclosure(L, TraitsCFunc<T>::f, 1);
        }

        template <class T, class F>
        void cfun(lua_State *L, F&& func)
        {
            cfun(static_cast<function_type_t<std::decay_t<F>>>(func));
        }
    }

    void TableDump(lua_State *L, int idx, const char* tabs = "")
    {
        idx = lua_absindex(L, idx);
        lua_pushnil(L);
        while (lua_next(L, idx) != 0)
        {
            int t = lua_type(L, -1);
            switch (t) {
                case LUA_TTABLE:
                    if (lua_type(L, -2) == LUA_TNUMBER)
                        printf("%s[%d] = {\n", tabs, lua_tointeger(L, -2));
                    else
                        printf("%s[%s] = {\n", tabs, lua_tostring(L, -2));
                    TableDump(L, -1, (std::string(tabs) + "\t").c_str());
                    printf("%s}\n", tabs);
                    break;
                case LUA_TSTRING:  /* strings */
                    if (lua_type(L, -2) == LUA_TNUMBER)
                        printf("%s[%d] = '%s'\n", tabs, lua_tointeger(L, -2), lua_tostring(L, -1));
                    else
                        printf("%s[%s] = '%s'\n", tabs, lua_tostring(L, -2), lua_tostring(L, -1));
                    break;
                case LUA_TBOOLEAN:  /* booleans */
                    if (lua_type(L, -2) == LUA_TNUMBER)
                        printf("%s[%d] = %s\n", tabs, lua_tointeger(L, -2), lua_toboolean(L, -1) ? "true" : "false");
                    else
                        printf("%s[%s] = %s\n", tabs, lua_tostring(L, -2), lua_toboolean(L, -1) ? "true" : "false");
                    break;
                case LUA_TNUMBER:  /* numbers */
                    if (lua_type(L, -2) == LUA_TNUMBER)
                        printf("%s[%d] = %g\n", tabs, lua_tointeger(L, -2), lua_tonumber(L, -1));
                    else
                        printf("%s[%s] = %g\n", tabs, lua_tostring(L, -2), lua_tonumber(L, -1));
                    break;
                default:  /* other values */
                    if (lua_type(L, -2) == LUA_TNUMBER)
                        printf("%s[%d] = %s\n", tabs, lua_tointeger(L, -2), lua_typename(L, lua_type(L, -1)));
                    else
                        printf("%s[%s] = %s\n", tabs, lua_tostring(L, -2), lua_typename(L, lua_type(L, -1)));
                    break;
            }
            lua_pop(L, 1);
        }
    }
    void StackDump(lua_State *L)
    {
        int i;
        int top = lua_gettop(L);
        for (i = 1; i <= top; i++) {  /* repeat for each level */
            int t = lua_type(L, i);
            switch (t) {
                case LUA_TTABLE:  /* table */
                    printf("[%d] = {\n", i);
                    TableDump(L, i, "\t");
                    printf("}\n");
                    break;
                case LUA_TSTRING:  /* strings */
                    printf("[%d] = '%s'\n", i, lua_tostring(L, i));
                    break;
                case LUA_TBOOLEAN:  /* booleans */
                    printf("[%d] = %s\n", i, lua_toboolean(L, i) ? "true" : "false");
                    break;
                case LUA_TNUMBER:  /* numbers */
                    if (lua_tonumber(L, i) > 0x100000 && lua_tonumber(L, i) < 0xffffffff)
                        printf("[%d] = %X\n", i, (uint32_t)lua_tonumber(L, i));
                    else
                        printf("[%d] = %g\n", i, lua_tonumber(L, i));
                    break;
                case LUA_TUSERDATA:  /* numbers */
                    printf("[%d] = %s %X\n", i, lua_typename(L, t), (unsigned int)lua_touserdata(L, i));
                    break;
                default:  /* other values */
                    printf("[%d] = %s\n", i, lua_typename(L, t));
                    break;
            }
        }
    }

    template<typename T>
    struct disect_function;

    template<typename R, typename ...Args>
    struct disect_function<R(*)(Args...)>
    {
        static const size_t nargs = sizeof...(Args);
        static const bool isClass = false;

        template <size_t i>
        struct arg
        {
            typedef typename std::tuple_element<i, std::tuple<Args...>>::type type;
        };
    };

    template<typename R, typename T, typename ...Args>
    struct disect_function<R(T::*)(Args...)>
    {
        static const size_t nargs = sizeof...(Args);
        static const bool isClass = true;

        template <size_t i>
        struct arg
        {
            typedef typename std::tuple_element<i, std::tuple<Args...>>::type type;
        };
    };

    template<typename R, typename ...Args>
    struct disect_function<std::function<R(Args...)>>
    {
        static const size_t nargs = sizeof...(Args);
        static const bool isClass = false;

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

                lua_pushinteger(L, lua_objlen(L, -1));
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
}