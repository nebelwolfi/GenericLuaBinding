#pragma once
#include "Stack.h"
#include "Function.h"

namespace LuaBinding {
    class Environment;
    class ObjectRef;
    class IndexProxy;
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

    template <typename T>
    concept has_lua_state = requires(T t)
    {
        { t.lua_state() } -> std::convertible_to<void*>;
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

        virtual const char* tostring()
        {
            return lua_tostring(L, idx);
        }

        virtual const char* tolstring()
        {
            return luaL_tolstring(L, -1, nullptr);
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

        virtual int push(int i = -1) const
        {
            lua_pushvalue(L, idx);
            if (i != -1)
                lua_insert(L, i);
            return 1;
        }

        virtual int push(int i = -1)
        {
            lua_pushvalue(L, idx);
            if (i != -1)
                lua_insert(L, i);
            return 1;
        }

        virtual void pop() {
            lua_remove(L, idx);
            this->idx = LUA_REFNIL;
        }

        template<typename Ref = ObjectRef> requires std::is_base_of_v<ObjectRef, Ref>
        Ref operator[](size_t index)
        {
            lua_geti(L, this->idx, index);
            return Ref(L, lua_gettop(L), false);
        }

        template<typename Ref = ObjectRef> requires std::is_base_of_v<ObjectRef, Ref>
        Ref operator[](const char* index)
        {
            lua_getfield(L, this->idx, index);
            return Ref(L, lua_gettop(L), false);
        }

        template<typename T>
        Object& operator=(T value)
        {
            detail::push(L, value);
            lua_replace(L, idx);
            return *this;
        }

        template<typename R, bool C = false>
        R call() {
            push();
            if constexpr (std::is_same_v<void, R>) {
                if (LuaBinding::pcall(L, 0, 0))
                {
                    throw std::exception(lua_tostring(L, -1));
                }
            } else {
                if (LuaBinding::pcall(L, 0, 1))
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

        template<int R>
        void call() {
            push();
            if (LuaBinding::pcall(L, 0, R))
            {
                throw std::exception(lua_tostring(L, -1));
            }
        }

        template<typename R, bool C = false, typename ...Params> requires (sizeof...(Params) > 0 && !std::is_same_v<std::tuple_element_t<0, std::tuple<Params...>>, Environment>)
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

        template<int R, typename ...Params> requires (sizeof...(Params) > 0 && !std::is_same_v<std::tuple_element_t<0, std::tuple<Params...>>, Environment>)
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

        template<typename Ref = ObjectRef> requires std::is_base_of_v<ObjectRef, Ref>
        Ref operator() () {
            push();
            if (LuaBinding::pcall(L, 0, 1))
            {
                throw std::exception(lua_tostring(L, -1));
            }
            return Ref(L, -1, false);
        }

        template<typename Ref = ObjectRef, typename ...Params> requires (std::is_base_of_v<ObjectRef, Ref>) && (sizeof...(Params) > 0 && !std::is_same_v<std::tuple_element_t<0, std::tuple<Params...>>, Environment>)
        Ref operator() (Params... param) {
            push();
            if constexpr (sizeof...(param) > 0)
                (void)std::initializer_list<int>{ detail::push(L, param)... };
            if (LuaBinding::pcall(L, sizeof...(param), 1))
            {
                throw std::exception(lua_tostring(L, -1));
            }
            return Ref(L, -1, false);
        }

        template<typename Ref = ObjectRef, typename Env, typename ...Params> requires (std::is_same_v<Env, Environment>)
        Ref operator()(Env env, Params... param) {
            push();
            if constexpr (sizeof...(param) > 0)
                (void)std::initializer_list<int>{ detail::push(L, param)... };
            if (env.pcall(sizeof...(param), 1))
            {
                throw std::exception(lua_tostring(L, -1));
            }
            return Ref(L, -1, false);
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
        void fun(int index, T& other)
        {
            push();
            Function::fun<void>(L, other);
            lua_rawseti(L, -2, index);
            lua_pop(L, 1);
        }

        template <typename T>
        void fun(int index, T&& other)
        {
            push();
            Function::fun<void>(L, other);
            lua_rawseti(L, -2, index);
            lua_pop(L, 1);
        }

        template <typename T>
        void cfun(int index, T& other)
        {
            push();
            Function::cfun<void>(L, other);
            lua_rawseti(L, -2, index);
            lua_pop(L, 1);
        }

        template <typename T>
        void cfun(int index, T&& other)
        {
            push();
            Function::cfun<void>(L, other);
            lua_rawseti(L, -2, index);
            lua_pop(L, 1);
        }

        template <typename T>
        void fun(const char* index, T& other)
        {
            push();
            Function::fun<void>(L, other);
            lua_setfield(L, -2, index);
            lua_pop(L, 1);
        }

        template <typename T>
        void fun(const char* index, T&& other)
        {
            push();
            Function::fun<void>(L, other);
            lua_setfield(L, -2, index);
            lua_pop(L, 1);
        }

        template <typename T>
        void cfun(const char* index, T& other)
        {
            push();
            Function::cfun<void>(L, other);
            lua_setfield(L, -2, index);
            lua_pop(L, 1);
        }

        template <typename T>
        void cfun(const char* index, T&& other)
        {
            push();
            Function::cfun<void>(L, other);
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

        bool valid(lua_State *S)
        {
            return S && L == S && this->idx != LUA_REFNIL;
        }

        bool valid(lua_State *S) const
        {
            return L && L == S && this->idx != LUA_REFNIL;
        }

        template <typename T>
        bool valid(T *S) requires has_lua_state<T>
        {
            return S && L == S->lua_state() && this->idx != LUA_REFNIL;
        }

        template <typename T>
        bool valid(T *S) const requires has_lua_state<T>
        {
            return L && L == S->lua_state() && this->idx != LUA_REFNIL;
        }

        void invalidate()
        {
            L = nullptr;
            this->idx = LUA_REFNIL;
        }

        virtual int len()
        {
            return lua_getlen(L, idx);
        }

        TableIter<ObjectRef> begin()
        {
            return TableIter<ObjectRef>(L, 1, this->idx);
        }
        TableIter<ObjectRef> end()
        {
            return TableIter<ObjectRef>(L, lua_getlen(L, idx) + 1, this->idx);
        }
        TableIter<ObjectRef> begin() const
        {
            return TableIter<ObjectRef>(L, 1, this->idx);
        }
        TableIter<ObjectRef> end() const
        {
            return TableIter<ObjectRef>(L, lua_getlen(L, idx) + 1, this->idx);
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
            if (other.valid(L))
            {
                other.push();
                this->idx = luaL_ref(L, LUA_REGISTRYINDEX);
            }
        }
        ObjectRef& operator=(Object&& other) noexcept {
            this->L = other.lua_state();
            if (other.valid(L))
            {
                other.push();
                this->idx = luaL_ref(L, LUA_REGISTRYINDEX);
            }
            return *this;
        }
        ObjectRef(const ObjectRef& other)
        {
            this->L = other.lua_state();
            if (other.valid(L))
            {
                other.push();
                this->idx = luaL_ref(L, LUA_REGISTRYINDEX);
            }
        }
        ObjectRef& operator=(ObjectRef&& other) noexcept {
            this->L = other.lua_state();
            if (other.valid(L))
            {
                other.push();
                this->idx = luaL_ref(L, LUA_REGISTRYINDEX);
            }
            return *this;
        }
        ~ObjectRef() {
            if (valid(L))
            {
                luaL_unref(L, LUA_REGISTRYINDEX, idx);
                idx = LUA_REFNIL;
                L = nullptr;
            }
        }

        const char* tostring() override
        {
            push();
            auto result = lua_tostring(L, -1);
            lua_pop(L, 1);
            return result;
        }

        const char* tolstring() override
        {
            push();
            auto result = luaL_tolstring(L, -1, nullptr);
            lua_pop(L, 1);
            return result;
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
        T as()
        {
            push();
            auto t = detail::get<T>(L, -1);
            lua_pop(L, 1);
            return t;
        }

        template<typename T>
        bool is()
        {
            if constexpr (std::is_same_v<T, void>)
            {
                push();
                auto t = lua_isnoneornil(L, -1);
                lua_pop(L, 1);
                return t;
            } else {
                push();
                auto t = detail::is<T>(L, -1);
                lua_pop(L, 1);
                return t;
            }
        }

        bool is(int t) override
        {
            push();
            auto b = lua_type(L, -1) == t;
            lua_pop(L, 1);
            return b;
        }

        template<typename IP = IndexProxy> requires std::is_base_of_v<IP, IndexProxy>
        IP operator[](size_t idx)
        {
            return IP(*this, idx);
        }

        template<typename IP = IndexProxy> requires std::is_base_of_v<IP, IndexProxy>
        IP operator[](const char* idx)
        {
            return IP(*this, idx);
        }

        [[nodiscard]] const void* topointer() const override
        {
            push();
            auto t = lua_topointer(L, -1);
            lua_pop(L, 1);
            return t;
        }

        int push(int i = -1) const override
        {
            lua_rawgeti(L, LUA_REGISTRYINDEX, idx);
            if (i != -1)
                lua_insert(L, i);
            return 1;
        }

        int push(int i = -1) override
        {
            lua_rawgeti(L, LUA_REGISTRYINDEX, idx);
            if (i != -1)
                lua_insert(L, i);
            return 1;
        }

        void pop() override
        {
            luaL_unref(L, LUA_REGISTRYINDEX, idx);
            this->idx = LUA_REFNIL;
        }

        int type() override
        {
            push();
            auto t = lua_type(L, -1);
            lua_pop(L, 1);
            return t;
        }

        int len() override
        {
            push();
            auto t = lua_getlen(L, -1);
            lua_pop(L, 1);
            return t;
        }
    };

    class IndexProxy : ObjectRef {
    protected:
        ObjectRef& element;
        const char* str_index;
        int int_index;

    public:
        IndexProxy(ObjectRef& el, const char* str_index) : element(el), str_index(str_index), int_index(-1) {}
        IndexProxy(ObjectRef& el, int int_index) : element(el), str_index(nullptr), int_index(int_index) {}

        operator ObjectRef () const {
            element.push();
            if (str_index)
                lua_getfield(element.lua_state(), -1, str_index);
            else
                lua_rawgeti(element.lua_state(), -1, int_index);
            lua_remove(element.lua_state(), -2);
            return ObjectRef(element.lua_state(), -1, false);
        }

        int push(int i = -1) const override
        {
            element.push();
            if (str_index)
                lua_getfield(element.lua_state(), -1, str_index);
            else
                lua_rawgeti(element.lua_state(), -1, int_index);
            lua_remove(element.lua_state(), -2);
            if (i != -1)
                lua_insert(element.lua_state(), i);
            return 1;
        }

        int push(int i = -1) override
        {
            element.push();
            if (str_index)
                lua_getfield(element.lua_state(), -1, str_index);
            else
                lua_rawgeti(element.lua_state(), -1, int_index);
            lua_remove(element.lua_state(), -2);
            if (i != -1)
                lua_insert(element.lua_state(), i);
            return 1;
        }

        void pop() override
        {
            element.push();
            lua_pushnil(element.lua_state());
            if (str_index)
                lua_setfield(element.lua_state(), -2, str_index);
            else
                lua_rawseti(element.lua_state(), -2, int_index);
            lua_pop(element.lua_state(), 1);
        }

        template <typename T>
        IndexProxy& operator=(const T& rhs) {
            if constexpr (detail::is_pushable_cfun<T>) {
                if (int_index == -1)
                    element.cfun(str_index, rhs);
                else
                    element.cfun(int_index, rhs);
            } else if constexpr (detail::is_pushable_fun<T>) {
                if (int_index == -1)
                    element.fun(str_index, rhs);
                else
                    element.fun(int_index, rhs);
            } else {
                if (int_index == -1)
                    element.set(str_index, rhs);
                else
                    element.set(int_index, rhs);
            }
            return *this;
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