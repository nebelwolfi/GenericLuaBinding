#pragma once

#include <map>
#include <optional>
#include "Class.h"

namespace LuaBinding {
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
    };

    template<typename T>
    class StackClass {
    public:
        static int push(lua_State* L, T& t) requires (!std::is_convertible_v<T, void*>)
        {
            new (lua_newuserdatauv(L, sizeof(T), 1)) T(t);
            lua_pushnil(L);
            lua_setiuservalue(L, -2, 1);
            helper<std::remove_pointer_t<std::decay_t<T>>>::push_metatable(L);
            lua_setmetatable(L, -2);
            return 1;
        }
        static int push(lua_State* L, T&& t) requires (!std::is_convertible_v<T, void*>)
        {
            new (lua_newuserdatauv(L, sizeof(T), 1)) T(t);
            lua_pushnil(L);
            lua_setiuservalue(L, -2, 1);
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
            lua_newuserdatauv(L, sizeof(void*), 1);
            lua_createtable(L, 1, 0);
            lua_pushlightuserdata(L, t);
            lua_seti(L, -2, 1);
            lua_setiuservalue(L, -2, 1);
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
            lua_newuserdatauv(L, sizeof(void*), 1);
            lua_createtable(L, 1, 0);
            lua_pushlightuserdata(L, t);
            lua_seti(L, -2, 1);
            lua_setiuservalue(L, -2, 1);
            helper<std::remove_pointer_t<std::decay_t<T>>>::push_metatable(L);
            lua_setmetatable(L, -2);
            return 1;
        }
        static bool is(lua_State* L, int index) {
            return lua_islightuserdata(L, index) || lua_isuserdata(L, index);
        }
        static const char* type_name(lua_State* L) {
            helper<std::remove_pointer_t<std::decay_t<T>>>::push_metatable(L);
            if (lua_getfield(L, -1, "__name"))
            {
                auto name = lua_tostring(L, -1);
                lua_pop(L, 2);
                return name;
            }
            lua_pop(L, 2);
            return typeid(T).name();
        }
        static T get(lua_State* L, int index)
        {
            if (!is(L, index))
            {
                if (lua_isnoneornil(L, index))
                    if constexpr (std::is_convertible_v<T, void*>)
                        return nullptr;
                    else
                        luaL_typeerror(L, index, type_name(L));
                else
                    luaL_typeerror(L, index, type_name(L));
            }
            if (lua_getiuservalue(L, index, 1) == LUA_TNIL)
            {
                lua_pop(L, 1);
                if constexpr (std::is_convertible_v<T, void*>){
                    return (T)lua_touserdata(L, index);
                } else {
                    return *(T*)lua_touserdata(L, index);
                }
            }
            else
            {
                lua_geti(L, -1, 1);
                auto p = lua_touserdata(L, -1);
                lua_pop(L, 2);
                if constexpr (std::is_convertible_v<T, void*>){
                    return (T)p;
                } else {
                    return *(T*)p;
                }
            }
        }
    };

    template<>
    class Stack<int> {
    public:
        static int push(lua_State* L, int t)
        {
            lua_pushinteger(L, t);
            return 1;
        }
        static bool is(lua_State* L, int index) {
            return lua_isnumber(L, index);
        }
        static int get(lua_State* L, int index)
        {
            return (int)lua_tointeger(L, index);
        }
        static const char* type_name(lua_State* L) {
            return "integer";
        }
    };

    template<>
    class Stack<unsigned int> {
    public:
        static int push(lua_State* L, unsigned int t)
        {
            lua_pushinteger(L, t);
            return 1;
        }
        static bool is(lua_State* L, int index) {
            return lua_isnumber(L, index);
        }
        static unsigned int get(lua_State* L, int index)
        {
            return (int)lua_tointeger(L, index);
        }
        static const char* type_name(lua_State* L) {
            return "unsigned integer";
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
    };

    template<class T> requires std::is_integral_v<T>
    class Stack<T> {
    public:
        static int push(lua_State* L, T t)
        {
            lua_pushinteger(L, t);
            return 1;
        }
        static bool is(lua_State* L, int index) {
            return lua_isnumber(L, index);
        }
        static T get(lua_State* L, int index)
        {
            return (T)lua_tointeger(L, index);
        }
        static const char* type_name(lua_State* L) {
            return "integer";
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
            return lua_tostring(L, index);
        }
        static const char* type_name(lua_State* L) {
            return "string";
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
            return lua_tostring(L, index);
        }
        static const char* type_name(lua_State* L) {
            return "string";
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
            return std::string(lua_tolstring(L, index, nullptr));
        }
        static const char* type_name(lua_State* L) {
            return "string";
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
            return lua_isinteger(L, index);
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
    };

    template<typename K, typename V>
    class Stack<std::pair<K, V>> {
    public:
        static int push(lua_State* L, std::pair<K, V> t)
        {
            lua_createtable(L, 2, 0);
            detail::push<K>(L, t.first);
            lua_seti(L, -2, 1);
            detail::push<K>(L, t.second);
            lua_seti(L, -2, 2);
            return 1;
        }
        static bool is(lua_State* L, int index) {
            return lua_isinteger(L, index);
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
            static char buff[100];
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
                lua_seti(L, -2, i + 1);
            }
            return 1;
        }
        static bool is(lua_State* L, int index) {
            return lua_istable(L, index);
        }
        static std::vector<T> get(lua_State* L, int index)
        {
            index = lua_absindex(L, index);
            lua_len(L, index);
            size_t len = lua_tointeger(L, -1);
            lua_pop(L, 1);
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
            static char buff[100];
            if constexpr (detail::is_pushable<T>)
            {
                snprintf(buff, 100, "table{%s}", Stack<T>::type_name(L));
            } else
                snprintf(buff, 100, "table{%s}", StackClass<T>::type_name(L));
            return buff;
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
                lua_seti(L, -2, i);
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
            static char buff[100];
            if constexpr (detail::is_pushable<T>)
            {
                snprintf(buff, 100, "table{%s}", Stack<T>::type_name(L));
            } else
                snprintf(buff, 100, "table{%s}", StackClass<T>::type_name(L));
            return buff;
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
            static char buff[100];
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
            static char buff[100];
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
    };

    namespace detail {
        template<typename T>
        concept is_pushable = requires(T a) {
            { LuaBinding::Stack<std::decay_t<T>>::is(nullptr, 0) };
        };

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

        template<class T> requires (!is_pushable<T>)
        int push(lua_State*L, T& t)
        {
            return StackClass<T>::push(L, t);
        }

        template<class T> requires (!is_pushable<T>)
        int push(lua_State*L, T&& t)
        {
            return StackClass<T>::push(L, t);
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
        T get(lua_State* L, int index)
        {
            return Stack<T>::get(L, index);
        }

        template<class T> requires (!is_pushable<T>)
        T get(lua_State* L, int index)
        {
            return StackClass<T>::get(L, index);
        }
    }
}
