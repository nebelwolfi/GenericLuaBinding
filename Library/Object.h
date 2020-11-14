#pragma once
#include "Stack.h"

namespace LuaBinding {
    class Object {
        lua_State* L = nullptr;
        int idx = LUA_REFNIL;
        bool ref = false;
    public:
        Object() = default;
        Object(lua_State* L, int idx) : L(L)
        {
            this->idx = lua_absindex(L, idx);
        }
        Object(lua_State* L, bool ref, int ref_index)
                : L(L), ref(ref), idx(ref_index)
        { }
        static Object Ref(lua_State* L, int idx, bool copy = true)
        {
            if (copy)
                lua_pushvalue(L, idx);
            return {
                    L,
                    true,
                    luaL_ref(L, LUA_REGISTRYINDEX)
            };
        }
        ~Object() {
            if (ref)
            {
                luaL_unref(L, LUA_REGISTRYINDEX, idx);
            }
        }

        const char* tostring()
        {
            return lua_tostring(L, idx);
        }

        template<typename T>
        T as()
        {
            if (ref)
            {
                lua_rawgeti(L, LUA_REGISTRYINDEX, idx);
                auto t = detail::get<T>(L, -1);
                lua_pop(L, 1);
                return t;
            }
            else
                return detail::get<T>(L, idx);
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
        bool is()
        {
            if constexpr (std::is_same_v<T, void>)
            {
                if (ref)
                {
                    lua_rawgeti(L, LUA_REGISTRYINDEX, idx);
                    auto t = lua_isnoneornil(L, -1);
                    lua_pop(L, 1);
                    return t;
                }
                else
                    return lua_isnoneornil(L, idx);
            } else {
                if (ref)
                {
                    lua_rawgeti(L, LUA_REGISTRYINDEX, idx);
                    auto t = detail::is<T>(L, -1);
                    lua_pop(L, 1);
                    return t;
                }
                else
                    return detail::is<T>(L, idx);
            }
        }

        bool is(int t)
        {
            if (ref)
            {
                lua_rawgeti(L, LUA_REGISTRYINDEX, idx);
                auto b = lua_type(L, -1) == t;
                lua_pop(L, 1);
                return b;
            }
            else
                return lua_type(L, idx) == t;
        }

        const void* topointer() const
        {
            if (ref)
            {
                lua_rawgeti(L, LUA_REGISTRYINDEX, idx);
                auto t = lua_topointer(L, -1);
                lua_pop(L, 1);
                return t;
            }
            else
                return lua_topointer(L, idx);
        }

        Object operator[](size_t idx)
        {
            if (ref)
            {
                lua_rawgeti(L, LUA_REGISTRYINDEX, this->idx);
                lua_geti(L, -1, idx);
                lua_remove(L, -2);
            }
            else
                lua_geti(L, this->idx, idx);
            return Object(L, lua_gettop(L));
        }

        Object operator[](const char* idx)
        {
            if (ref)
            {
                lua_rawgeti(L, LUA_REGISTRYINDEX, this->idx);
                lua_getfield(L, -1, idx);
                lua_remove(L, -2);
            }
            else
                lua_getfield(L, this->idx, idx);
            return Object(L, lua_gettop(L));
        }

        bool valid()
        {
            return L && this->idx;
        }

        int push()
        {
            if (ref)
                lua_rawgeti(L, LUA_REGISTRYINDEX, idx);
            else
                lua_pushvalue(L, idx);
            return 1;
        }

        void pop() {
            if (ref)
            {
                luaL_unref(L, LUA_REGISTRYINDEX, idx);
            } else {
                lua_remove(L, idx);
            }
        }

        template<typename R, typename ...Params>
        R call(Params... param) {
            push();
            if constexpr (sizeof...(param) > 0)
                (void)std::initializer_list<int>{ detail::push(L, param...) };
            if constexpr (std::is_same_v<void, R>) {
                pcall(L, sizeof...(param), 0);
            } else {
                pcall(L, sizeof...(param), 1);
                return LuaBinding::detail::get<R>(L, -1);
            }
        }

        template <typename T>
        void set(const char* idx, T&& other)
        {
            push();
            detail::push(L, other);
            lua_setfield(L, -2, idx);
        }

        template <typename T>
        void set(int idx, T&& other)
        {
            push();
            detail::push(L, other);
            lua_seti(L, -2, idx);
        }

        int index()
        {
            return idx;
        }

        lua_State* lua_state()
        {
            return L;
        }

        int type()
        {
            if (ref)
            {
                lua_rawgeti(L, LUA_REGISTRYINDEX, this->idx);
                auto t = lua_type(L, -1);
                lua_pop(L, 1);
                return t;
            }
            return lua_type(L, idx);
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
    };
}
