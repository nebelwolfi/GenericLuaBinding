#pragma once
#include "Stack.h"

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
            lua_pushcclosure(L, TraitsCFunc<T>::f, 1);
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