#pragma once

#include "DynClass.h"
#include "Object.h"
#include "Stack.h"
#include "Class.h"
#include "StackIter.h"
#include "Environment.h"
#include "Function.h"
#include <vector>
#include <functional>
#include <stdexcept>
template <typename T>
struct identity { typedef T type; };

namespace LuaBinding {
    class State;
    template<typename T>
    class Class;
    class DynClass;
    class Object;
    template<typename ...Functions>
    class OverloadedFunction;

    template< class T >
    inline constexpr bool is_state_v = std::is_same<T, State*>::value;

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
            lua_newtable(L);
            lua_setglobal(L, "__DATASTORE");

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
            auto t = new T(params...);
            auto u = (void**)lua_newuserdata(L, sizeof(void*) * 2);
            *u = t; *(u + 1) = (void*)0xC0FFEE;
            helper<std::remove_pointer_t<std::decay_t<T>>>::push_metatable(L);
            lua_setmetatable(L, -2);
            lua_remove(L, -2);
            return t;
        }

        template<class T, class ...Params> requires (!std::is_constructible_v<T>)
        T* alloc()
        {
            auto t = (T*)new char[sizeof(T)];
            auto u = (void**)lua_newuserdata(L, sizeof(void*) * 2);
            *u = t; *(u + 1) = (void*)0xC0FFEE;
            helper<std::remove_pointer_t<std::decay_t<T>>>::push_metatable(L);
            lua_setmetatable(L, -2);
            lua_remove(L, -2);
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
            OverloadedFunction(L, functions...);
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

        int exec(Environment env, const char* code, int argn = 0, int nres = 0)
        {
            if (luaL_loadstring(L, code) || env.pcall(argn, nres))
            {
                throw std::exception(lua_tostring(L, -1));
            }
            return lua_gettop(L);
        }

        int n() {
            return lua_gettop(L);
        }

        template<typename R, bool C = false, typename ...Params> requires (!std::is_same_v<std::tuple_element_t<0, std::tuple<Params...>>, Environment>)
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
                if constexpr(C) {
                    auto result = LuaBinding::detail::get<R>(L, -1);
                    lua_pop(L, 1);
                    return result;
                } else
                    return LuaBinding::detail::get<R>(L, -1);
            }
        }

        template<int R, typename ...Params> requires (!std::is_same_v<std::tuple_element_t<0, std::tuple<Params...>>, Environment>)
        void call(Params... param) {
            if constexpr (sizeof...(param) > 0)
                (void)std::initializer_list<int>{ detail::push(L, param)... };
            if (LuaBinding::pcall(L, sizeof...(param), R))
            {
                throw std::exception(lua_tostring(L, -1));
            }
        }

        template<typename R, typename Env, typename ...Params> requires (std::is_same_v<Env, Environment>)
        R call(Env env, Params... param) {
            if constexpr (sizeof...(param) > 0)
                (void)std::initializer_list<int>{ push(param...) };
            if constexpr (std::is_same_v<void, R>) {
                if (env.pcall(sizeof...(param), 0))
                {
                    throw std::exception(lua_tostring(L, -1));
                }
            } else if constexpr (std::is_same_v<Object, R>) {
                if (env.pcall(sizeof...(param), 1))
                {
                    throw std::exception(lua_tostring(L, -1));
                }
                return Object(L, -1);
            } else if constexpr (std::is_same_v<ObjectRef, R>) {
                if (env.pcall(sizeof...(param), 1))
                {
                    throw std::exception(lua_tostring(L, -1));
                }
                return ObjectRef(L, -1);
            } else {
                if (env.pcall(sizeof...(param), 1))
                {
                    throw std::exception(lua_tostring(L, -1));
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

        template<bool C = false>
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