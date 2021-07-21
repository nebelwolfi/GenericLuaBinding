#pragma once

#include "Class.h"
#include "DynClass.h"
#include "Object.h"
#include "Stack.h"
#include "StackIter.h"
#include <vector>
#include <functional>
#include <stdexcept>
//#include <SEH.h>

template <typename T>
struct identity { typedef T type; };

namespace LuaBinding {
    class State;
    template<typename T>
    class Class;
    class DynClass;
    class Object;

    template< class T >
    inline constexpr bool is_state_v = std::is_same<T, State*>::value;

    //static int wrap_exceptions(lua_State *L, lua_CFunction f)
    //{
    //    __try {
    //        return f(L);
    //    }
    //    __except (SEH::filter(_exception_code(), (EXCEPTION_POINTERS*)_exception_info()))
    //    {
    //        SEH::print_err("JitWrapper", _exception_code());
    //    }
    //    return 0;
    //}

    class State {
        lua_State* L = nullptr;
        bool view = false;
        std::vector<DynClass*> dynamic_classes = {};

    public:
        State() {
            L = luaL_newstate();
            luaL_openlibs(L);
            lua_newtable(L);
            lua_setglobal(L, "__METASTORE");

            //lua_pushlightuserdata(L, (void *)wrap_exceptions);
            //luaJIT_setmode(L, -1, LUAJIT_MODE_WRAPCFUNC|LUAJIT_MODE_ON);
            //lua_pop(L, 1);
        }
        State(lua_State*L) : L(L), view(true) { }
        State(const State& state) : L(state.L), view(true) { }

        ~State() {
            if (!view) {
                lua_close(L);
                L = nullptr;
                for (auto& c : dynamic_classes)
                    delete c;
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

        int addDynClass(const char* name)
        {
            dynamic_classes.push_back(new DynClass(L, name));
            return 1;
        }

        void pop(int n)
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
            auto t = new ((T*)lua_newuserdatauv(L, sizeof(T), 1)) T(params...);
            helper<std::remove_pointer_t<std::decay_t<T>>>::push_metatable(L);
            lua_setmetatable(L, -2);
            return t;
        }

        template<class T, class ...Params> requires (!std::is_constructible_v<T>)
        T* alloc()
        {
            auto t = (T*)lua_newuserdatauv(L, sizeof(T), 1);
            memset(t, 0, sizeof(T));
            helper<std::remove_pointer_t<std::decay_t<T>>>::push_metatable(L);
            lua_setmetatable(L, -2);
            return t;
        }

        template <class R, class... Params>
        void fun(R(*func)(Params...))
        {
            lua_pushlightuserdata(L, this);
            lua_pushlightuserdata(L, reinterpret_cast<void*>(func));
            lua_pushcclosure(L, Traits<R, Params...>::f, 2);
        }

        template <class R, class... Params>
        void fun(const char* name, R(* func)(Params...))
        {
            fun(func);
            lua_setglobal(L, name);
        }

        template <class R, class... Params>
        void fun(std::function<R(Params...)> func)
        {
            using Fun = decltype (func);
            lua_pushlightuserdata(L, this);
            new (lua_newuserdatauv(L, sizeof(func), 1)) Fun(func);
            lua_pushcclosure(L, TraitsSTDFunction<R, Params...>::f, 2);
        }

        template <class R, class... Params>
        void fun(const char* name, std::function<R(Params...)> func)
        {
            fun(func);
            lua_setglobal(L, name);
        }

        template <typename F>
        void fun(F&& func)
        {
            fun(static_cast<function_type_t<F>>(func));
        }

        template <typename F>
        void fun(const char* name, F&& func)
        {
            fun(func);
            lua_setglobal(L, name);
        }

        template <class R> requires std::is_integral_v<R>
        void cfun(R(*func)(State*))
        {
            lua_pushlightuserdata(L, this);
            lua_pushlightuserdata(L, reinterpret_cast<void*>(func));
            lua_pushcclosure(L, func, 2);
        }

        template <class R> requires std::is_integral_v<R>
        void cfun(const char* name, R(*func)(State*))
        {
            cfun(func);
            lua_setglobal(L, name);
        }

        template <class R> requires std::is_integral_v<R>
        void cfun(R(*func)(lua_State*))
        {
            lua_pushcclosure(L, func, 0);
        }

        template <class R> requires std::is_integral_v<R>
        void cfun(const char* name, R(*func)(lua_State*))
        {
            cfun(func);
            lua_setglobal(L, name);
        }

        int exec(const char* code, int argn = 0, int nres = 0)
        {
            if (luaL_loadstring(L, code) || LuaBinding::pcall(L, argn, nres))
            {
                throw std::exception(lua_tostring(L, -1));
            }
            return lua_gettop(L);
        }

        int n() {
            return lua_gettop(L);
        }

        template<typename R, typename ...Params>
        R call(Params... param) {
            if constexpr (sizeof...(param) > 0)
                (void)std::initializer_list<int>{ push(param...) };
            if constexpr (std::is_same_v<void, R>) {
                if (LuaBinding::pcall(L, sizeof...(param), 0))
                {
                    throw std::exception(lua_tostring(L, -1));
                }
            } else if constexpr (std::is_same_v<Object, R>) {
                if (LuaBinding::pcall(L, sizeof...(param), 1))
                {
                    throw std::exception(lua_tostring(L, -1));
                }
                return Object(L, -1);
            } else if constexpr (std::is_same_v<ObjectRef, R>) {
                if (LuaBinding::pcall(L, sizeof...(param), 1))
                {
                    throw std::exception(lua_tostring(L, -1));
                }
                return ObjectRef(L, -1);
            } else {
                if (LuaBinding::pcall(L, sizeof...(param), 1))
                {
                    throw std::exception(lua_tostring(L, -1));
                }
                return LuaBinding::detail::get<R>(lua_state(), -1);
            }
        }

        template<int R, typename ...Params>
        void call(Params... param) {
            push();
            if constexpr (sizeof...(param) > 0)
                (void)std::initializer_list<int>{ detail::push(L, param)... };
            if (LuaBinding::pcall(L, sizeof...(param), R))
            {
                throw std::exception(lua_tostring(L, -1));
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

        Object at(int index = -1)
        {
            if (index > 0)
            {
                if (index > lua_gettop(L))
                    throw std::out_of_range("invalid stack subscript");
                return { L, index };
            } else if (index < 0) {
                auto top = lua_gettop(L);
                if (-index > top)
                    throw std::out_of_range("invalid stack subscript");
                return { L, top + 1 + index };
            }
            throw std::out_of_range("wat");
        }

        ObjectRef at(const char* idx)
        {
            lua_getglobal(L, idx);
            return ObjectRef(L, -1, false);
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

        template <class T>
        void global(const char* name, T thing)
        {
            push(thing);
            lua_setglobal(L, name);
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