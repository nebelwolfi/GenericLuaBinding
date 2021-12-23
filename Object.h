#pragma once
#include "Stack.h"

namespace LuaBinding {
    class Environment;
    class ObjectRef;
    template<typename T>
    class TableIter
    {
    public:
        typedef std::forward_iterator_tag iterator_category;
        TableIter(lua_State* L, int index, int base) : L(L), index(index), base(base) { }
        TableIter operator++() { index++; return *this; }
        T operator*() { lua_rawgeti(L, base, index); return T(L, -1, false); }
        bool operator!=(const TableIter& rhs) { return index != rhs.index; }
    private:
        lua_State* L;
        int index;
        int base;
    };

    class Object {
    protected:
        int idx = LUA_REFNIL;
        lua_State* L = nullptr;
    public:
        Object() = default;
        Object(lua_State* L, int idx) : L(L)
        {
            this->idx = lua_absindex(L, idx);
        }
        ~Object() = default;

        const char* tostring()
        {
            return lua_tostring(L, idx);
        }

        const char* tolstring()
        {
            lua_getglobal(L, "tostring");
            push();
            LuaBinding::pcall(L, 1, 1);
            auto result = lua_tostring(L, -1);
            lua_pop(L, 1);
            return result;
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

        virtual Object operator[](size_t index)
        {
            lua_geti(L, this->idx, index);
            return Object(L, lua_gettop(L));
        }

        virtual Object operator[](const char* index)
        {
            lua_getfield(L, this->idx, index);
            return Object(L, lua_gettop(L));
        }

        virtual int push() const
        {
            lua_pushvalue(L, idx);
            return 1;
        }

        virtual void pop() const
        {
            lua_remove(L, idx);
        }

        virtual int push()
        {
            lua_pushvalue(L, idx);
            return 1;
        }

        virtual void pop() {
            lua_remove(L, idx);
            this->idx = LUA_REFNIL;
        }

        template<typename R, bool C = false, typename ...Params> requires (!std::is_same_v<std::tuple_element_t<0, std::tuple<Params...>>, Environment>)
        R call(Params... param) {
            push();
            if constexpr (sizeof...(param) > 0)
                (void)std::initializer_list<int>{ detail::push(L, param)... };
            if constexpr (std::is_same_v<void, R>) {
                if (LuaBinding::pcall(L, sizeof...(param), 0))
                {
                    throw std::exception(lua_tostring(L, -1));
                }
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
            push();
            if constexpr (sizeof...(param) > 0)
                (void)std::initializer_list<int>{ detail::push(L, param)... };
            if (LuaBinding::pcall(L, sizeof...(param), R))
            {
                throw std::exception(lua_tostring(L, -1));
            }
        }

        template<typename R, bool C = false, typename Env, typename ...Params> requires (std::is_same_v<Env, Environment>)
        R call(Env env, Params... param) {
            push();
            if constexpr (sizeof...(param) > 0)
                (void)std::initializer_list<int>{ detail::push(L, param)... };
            if constexpr (std::is_same_v<void, R>) {
                if (env.pcall(sizeof...(param), 0))
                {
                    throw std::exception(lua_tostring(L, -1));
                }
            } else {
                if (env.pcall(sizeof...(param), 1))
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

        template<int R, typename Env, typename ...Params> requires (std::is_same_v<Env, Environment>)
        void call(Env env, Params... param) {
            push();
            if constexpr (sizeof...(param) > 0)
                (void)std::initializer_list<int>{ detail::push(L, param)... };
            if (env.pcall(sizeof...(param), R))
            {
                throw std::exception(lua_tostring(L, -1));
            }
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

        template <typename T>
        void fun(const char* index, T& other)
        {
            push();
            detail::fun(L, other);
            lua_setfield(L, -2, index);
            lua_pop(L, 1);
        }

        template <typename T>
        void fun(const char* index, T&& other)
        {
            push();
            detail::fun(L, other);
            lua_setfield(L, -2, index);
            lua_pop(L, 1);
        }

        template <typename T>
        void cfun(const char* index, T& other)
        {
            push();
            detail::cfun(L, other);
            lua_setfield(L, -2, index);
            lua_pop(L, 1);
        }

        template <typename T>
        void cfun(const char* index, T&& other)
        {
            push();
            detail::cfun(L, other);
            lua_setfield(L, -2, index);
            lua_pop(L, 1);
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

        bool valid() const
        {
            return L && this->idx != LUA_REFNIL;
        }

        bool valid()
        {
            return L && this->idx != LUA_REFNIL;
        }

        virtual int len()
        {
            return get_len(L, idx);
        }

        TableIter<ObjectRef> begin()
        {
            return TableIter<ObjectRef>(L, 1, this->idx);
        }
        TableIter<ObjectRef> end()
        {
            return TableIter<ObjectRef>(L, get_len(L, idx) + 1, this->idx);
        }
        TableIter<ObjectRef> begin() const
        {
            return TableIter<ObjectRef>(L, 1, this->idx);
        }
        TableIter<ObjectRef> end() const
        {
            return TableIter<ObjectRef>(L, get_len(L, idx) + 1, this->idx);
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
        ObjectRef(const Object& other)
        {
            this->L = other.lua_state();
            if (other.valid())
            {
                other.push();
                this->idx = luaL_ref(L, LUA_REGISTRYINDEX);
            }
        }
        ObjectRef& operator=(Object&& other) noexcept {
            this->L = other.lua_state();
            if (other.valid())
            {
                other.push();
                this->idx = luaL_ref(L, LUA_REGISTRYINDEX);
            }
            return *this;
        }
        ObjectRef(const ObjectRef& other)
        {
            this->L = other.lua_state();
            if (other.valid())
            {
                other.push();
                this->idx = luaL_ref(L, LUA_REGISTRYINDEX);
            }
        }
        ObjectRef& operator=(ObjectRef&& other) noexcept {
            this->L = other.lua_state();
            if (other.valid())
            {
                other.push();
                this->idx = luaL_ref(L, LUA_REGISTRYINDEX);
            }
            return *this;
        }
        ~ObjectRef() {
            if (valid())
            {
                luaL_unref(L, LUA_REGISTRYINDEX, idx);
                idx = LUA_REFNIL;
                L = nullptr;
            }
        }

        template<typename T>
        T as()
        {
            lua_rawgeti(L, LUA_REGISTRYINDEX, idx);
            auto t = detail::get<T>(L, -1);
            lua_pop(L, 1);
            return t;
        }

        template<typename T>
        bool is()
        {
            if constexpr (std::is_same_v<T, void>)
            {
                lua_rawgeti(L, LUA_REGISTRYINDEX, idx);
                auto t = lua_isnoneornil(L, -1);
                lua_pop(L, 1);
                return t;
            } else {
                lua_rawgeti(L, LUA_REGISTRYINDEX, idx);
                auto t = detail::is<T>(L, -1);
                lua_pop(L, 1);
                return t;
            }
        }

        bool is(int t) override
        {
            lua_rawgeti(L, LUA_REGISTRYINDEX, idx);
            auto b = lua_type(L, -1) == t;
            lua_pop(L, 1);
            return b;
        }

        [[nodiscard]] const void* topointer() const override
        {
            lua_rawgeti(L, LUA_REGISTRYINDEX, idx);
            auto t = lua_topointer(L, -1);
            lua_pop(L, 1);
            return t;
        }

        Object operator[](size_t idx) override
        {
            lua_rawgeti(L, LUA_REGISTRYINDEX, this->idx);
            lua_geti(L, -1, idx);
            lua_remove(L, -2);
            return Object(L, lua_gettop(L));
        }

        Object operator[](const char* idx) override
        {
            lua_rawgeti(L, LUA_REGISTRYINDEX, this->idx);
            lua_getfield(L, -1, idx);
            lua_remove(L, -2);
            return Object(L, lua_gettop(L));
        }

        int push() const override
        {
            lua_rawgeti(L, LUA_REGISTRYINDEX, idx);
            return 1;
        }

        void pop() const override
        {
            luaL_unref(L, LUA_REGISTRYINDEX, idx);
        }

        int push() override
        {
            lua_rawgeti(L, LUA_REGISTRYINDEX, idx);
            return 1;
        }

        void pop() override
        {
            luaL_unref(L, LUA_REGISTRYINDEX, idx);
            this->idx = LUA_REFNIL;
        }

        int type() override
        {
            lua_rawgeti(L, LUA_REGISTRYINDEX, this->idx);
            auto t = lua_type(L, -1);
            lua_pop(L, 1);
            return t;
        }

        int len() override
        {
            lua_rawgeti(L, LUA_REGISTRYINDEX, this->idx);
            auto t = get_len(L, -1);
            lua_pop(L, 1);
            return t;
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
}