#pragma once

namespace LuaBinding {
    namespace detail
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
    using function_type_t = typename detail::function_traits<F>::function_type;

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
            fun<T>(static_cast<function_type_t<std::decay_t<F>>>(func));
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
}