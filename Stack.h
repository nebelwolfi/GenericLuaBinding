#pragma once

#include <map>
#include <optional>
#include <vector>
#include <unordered_map>
#include "Object.h"

namespace LuaBinding {
    template<typename T>
    class Stack;
    template<typename T>
    class StackClass;

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

        template<class T>
        int fun(lua_State* L, T& t);

        template<class T>
        int fun(lua_State* L, T&& t);

        template<class T>
        int cfun(lua_State* L, T& t);

        template<class T>
        int cfun(lua_State* L, T&& t);

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
            if (!lua_isuserdata(L, index) || !lua_isnumber(L, index)) return false;
            return true;
        }
        static bool is(lua_State* L, int index) requires (!std::is_same_v<T, void*>) {
            if (!lua_isuserdata(L, index)) return false;
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
            return *(T*)lua_touserdata(L, index);
        }
        static T get(lua_State* L, int index) requires (!std::is_convertible_v<T, void*>)
        {
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
            return (uint32_t)lua_tonumber(L, index);
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
            size_t len = get_len(L, index);
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