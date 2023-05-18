#pragma once
#ifdef LUABINDING_DYN_CLASSES
#include "DynClass.h"
#endif
#include "Object.h"
#include "Stack.h"
#include "Class.h"
#include "StackIter.h"
#include "Environment.h"
#include "Function.h"
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