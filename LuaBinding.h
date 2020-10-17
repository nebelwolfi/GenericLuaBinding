#pragma once

#include <cstdint>
#include <cassert>
#include <string>
#include <typeinfo>
#include "Dependencies/Lua/lua.hpp"
#include "Dependencies/Lua/lstate.h"
#include "FuncTraits.h"

namespace LuaBinding {
    class object;
    class table;
    class class_table;
    class lua_state;
    static int meta_ref = LUA_REFNIL;
    namespace detail {
        template<typename T>
        class key_helper { public: static void const* get() { static char value; return &value; } };
        template <class T>
        class stack
        {
            static T get (lua_state* S, int index) {
                T* const t = (T*)lua_touserdata(S, index);
                return t;
            };
            static bool isInstance (lua_state* S, int index) {
                lua_getupvalue(S, index, 1);
                lua_rawgeti(S, LUA_REGISTRYINDEX, meta_ref);
                lua_rawgetp(S, -1, detail::key_helper<T>::get());
                lua_remove(S, -2);
                auto eq = lua_rawequal(S, -1, -2);
                lua_pop(S, 2);
                return eq;
            };
        public:
            static void push (lua_state* S, T const& t) {
            }
        };
        template<typename>
        class name_helper {
        public:
            static std::string& get() { static std::string value; return value; }
            static void set(const char* name) { get() = name; }
        };
        template <class T>
        static int getVariable (lua_state* L)
        {
            T const* ptr = static_cast <T const*> (lua_touserdata (L, lua_upvalueindex (1)));
            detail::stack <T>::push (L, *ptr);
            return 1;
        }
        template <class T>
        static int setVariable (lua_state* L)
        {
            T* ptr = static_cast <T*> (lua_touserdata (L, lua_upvalueindex (1)));
            *ptr = detail::stack <T>::get (L, 1);
            return 0;
        }
        template <class C, typename T>
        static int getVariable (lua_state* L)
        {
            C* const c = (C*)lua_touserdata(L, 1);
            auto* mp = static_cast <T C::**> (lua_touserdata (L, lua_upvalueindex (1)));
            try
            {
                stack <T&>::push (L, c->**mp);
            }
            catch (const std::exception& e)
            {
                luaL_error (L, e.what ());
            }
            return 1;
        }
        template <class C, typename T>
        static int setVariable (lua_state* L)
        {
            C* const c = (C*)lua_touserdata(L, 1);
            auto* mp = static_cast <T C::**> (lua_touserdata (L, lua_upvalueindex (1)));
            try
            {
                c->**mp = stack <T>::get (L, 2);
            }
            catch (const std::exception& e)
            {
                luaL_error (L, e.what ());
            }
            return 0;
        }

        template <class ReturnType, class Params, int startParam>
        struct Invoke
        {
            template <class Fn>
            static int run (lua_State* L, Fn& fn)
            {
                try
                {
                    function_conversion::function_traits::type_lists::ArgList <Params, startParam> args (L);
                    stack <ReturnType>::push (L, function_conversion::function_traits::FuncTraits <Fn>::call (fn, args));
                    return 1;
                }
                catch (const std::exception& e)
                {
                    return luaL_error (L, e.what ());
                }
            }

            template <class T, class MemFn>
            static int run (lua_State* L, T* object, const MemFn& fn)
            {
                try
                {
                    function_conversion::function_traits::type_lists::ArgList <Params, startParam> args (L);
                    stack <ReturnType>::push (L, function_conversion::function_traits::FuncTraits <MemFn>::call (object, fn, args));
                    return 1;
                }
                catch (const std::exception& e)
                {
                    return luaL_error (L, e.what ());
                }
            }
        };
        template <class Params, int startParam>
        struct Invoke <void, Params, startParam>
        {
            template <class Fn>
            static int run (lua_State* L, Fn& fn)
            {
                try
                {
                    function_conversion::function_traits::type_lists::ArgList <Params, startParam> args (L);
                    function_conversion::function_traits::FuncTraits <Fn>::call (fn, args);
                    return 0;
                }
                catch (const std::exception& e)
                {
                    return luaL_error (L, e.what ());
                }
            }

            template <class T, class MemFn>
            static int run (lua_State* L, T* object, const MemFn& fn)
            {
                try
                {
                    function_conversion::function_traits::type_lists::ArgList <Params, startParam> args (L);
                    function_conversion::function_traits::FuncTraits <MemFn>::call (object, fn, args);
                    return 0;
                }
                catch (const std::exception& e)
                {
                    return luaL_error (L, e.what ());
                }
            }
        };
        template <class FnPtr>
        struct Call
        {
            typedef typename function_conversion::function_traits::FuncTraits <FnPtr>::Params Params;
            typedef typename function_conversion::function_traits::FuncTraits <FnPtr>::ReturnType ReturnType;

            static int f (lua_State* L)
            {
                assert (lua_islightuserdata (L, lua_upvalueindex (1)));
                auto fnptr = reinterpret_cast <FnPtr> (lua_touserdata (L, lua_upvalueindex (1)));
                assert (fnptr != 0);
                return Invoke <ReturnType, Params, 1>::run (L, fnptr);
            }
        };
        template <class MemFnPtr>
        struct CallMember
        {
            typedef typename function_conversion::function_traits::FuncTraits <MemFnPtr>::ClassType T;
            typedef typename function_conversion::function_traits::FuncTraits <MemFnPtr>::Params Params;
            typedef typename function_conversion::function_traits::FuncTraits <MemFnPtr>::ReturnType ReturnType;

            static int f (lua_State* L)
            {
                T* const t = (T*)lua_touserdata(L, 1);
                MemFnPtr const& fnptr = *static_cast <MemFnPtr const*> (lua_touserdata (L, lua_upvalueindex (1)));
                assert (fnptr != 0);
                return Invoke <ReturnType, Params, 2>::run (L, t, fnptr);
            }
        };
        template <class MemFnPtr>
        struct CallConstMember
        {
            typedef typename function_conversion::function_traits::FuncTraits <MemFnPtr>::ClassType T;
            typedef typename function_conversion::function_traits::FuncTraits <MemFnPtr>::Params Params;
            typedef typename function_conversion::function_traits::FuncTraits <MemFnPtr>::ReturnType ReturnType;

            static int f (lua_State* L)
            {
                T const* const t = (T*)lua_touserdata(L, 1);
                MemFnPtr const& fnptr = *static_cast <MemFnPtr const*> (lua_touserdata (L, lua_upvalueindex (1)));
                assert (fnptr != 0);
                return Invoke <ReturnType, Params, 2>::run (L, t, fnptr);
            }
        };
        template <class MemFnPtr>
        struct CallMetaMember
        {
            typedef typename function_conversion::function_traits::FuncTraits <MemFnPtr>::ClassType T;
            typedef typename function_conversion::function_traits::FuncTraits <MemFnPtr>::Params Params;
            typedef typename function_conversion::function_traits::FuncTraits <MemFnPtr>::ReturnType ReturnType;

            static int f(lua_State* L)
            {
                T* const t = (T*)lua_touserdata(L, 1);
                MemFnPtr const& fnptr = *static_cast <MemFnPtr const*> (lua_touserdata(L, lua_upvalueindex(1)));
                assert(fnptr != 0);
                return Invoke <ReturnType, Params, 2>::run(L, t, fnptr);
            }
        };
        template <class T>
        struct CallMemberCFunction
        {
            static int f (lua_State* L)
            {
                typedef int (T::*MFP) (lua_State* L);
                T* const t = (T*)lua_touserdata(L, 1);
                MFP const& fnptr = *static_cast <MFP const*> (lua_touserdata (L, lua_upvalueindex (1)));
                assert (fnptr != 0);
                return (t->*fnptr) (L);
            }
        };
        template <class T>
        struct CallConstMemberCFunction
        {
            static int f (lua_State* L)
            {
                typedef int (T::*MFP) (lua_State* L);
                T const* const t = (T*)lua_touserdata(L, 1);
                MFP const& fnptr = *static_cast <MFP const*> (lua_touserdata (L, lua_upvalueindex (1)));
                assert (fnptr != 0);
                return (t->*fnptr) (L);
            }
        };
    }
    class lua_state : public lua_State {
        template<typename T>
        void push_impl(T t) {
            detail::stack<T>::push(this, t);
        }
    public:
        [[nodiscard]] lua_State* state() const noexcept { return (lua_State *)this; };
        [[nodiscard]] object at(int idx) noexcept;
        [[nodiscard]] object top() noexcept;
        void open_libs() {
            luaL_openlibs(this);
        }
        int load_string(const char* s){
            auto status = luaL_loadstring(this, s);
            if (status)
            {
                throw std::exception(lua_tostring(this, -1));
            }
            return status;
        }
        int call_string(const char* s, int narg = 0, int nres = 0){
            try {
                auto status = luaL_loadstring(this, s);
                if (status) {
                    throw std::exception(lua_tostring(this, -1));
                }
                return this->pcall(narg, nres);
            } catch (const std::exception&e) {
                printf("%s", e.what());
                return 0;
            }
        }
        template<typename ... T>
        void push(const T&... t) {
            (void)std::initializer_list<int>{ (detail::stack<T>::push(this, t), 0)... };
        }
        void pop(int n = 1) {
            lua_pop(this, n);
        }
        int argn() {
            return lua_gettop(this);
        }
        template<typename T>
        class_table addClass(const char* name);
        template<typename T>
        T* alloc(size_t size = 0) {
            if (meta_ref == LUA_REFNIL) {
                lua_newtable(this);
                meta_ref = luaL_ref(this, LUA_REGISTRYINDEX);
            }
            auto u = (T*)lua_newuserdatauv(this, size == 0 ? sizeof(T) : size, 0);   // +1 | 0 | 1
            lua_rawgeti(this, LUA_REGISTRYINDEX, meta_ref);                                // +1 | 1 | 2
            lua_rawgetp(this, -1, detail::key_helper<T>::get());                                 // +1 | 2 | 3
            lua_remove(this, -2);                                                            // -1 | 3 | 2
            assert (lua_istable (this, -1));
            lua_setmetatable(this, -2);                                                // -1 | 2 | 1
            return u;
        }
        int call(int narg = 0, int nres = 0) {
            lua_getglobal(this, "debug");
            lua_getfield(this, -1, "traceback");
            lua_remove(this, -2);
            int errindex = -narg - 2;
            lua_insert(this, errindex);
            auto status = lua_pcall(this, narg, nres, errindex);
            if (!status) lua_remove(this, errindex);
            return status;
        }
        int pcall(int narg = 0, int nres = 0) {
            auto status = call(narg, nres);
            if (status)
            {
                throw std::exception(lua_tostring(this, -1));
            }
            return status;
        }
        template <class FP>
        lua_state* addCFunction(const char* name, FP func)
        {
            lua_pushcclosure(this, reinterpret_cast<lua_CFunction>(func), 0);
            lua_setglobal(this, name);
            return this;
        }
        template <class FP>
        lua_state* addFunction(const char* name, FP func)
        {
            lua_pushlightuserdata(this, reinterpret_cast<void*>(func));
            lua_pushcclosure(this, reinterpret_cast<lua_CFunction>(detail::Call<FP>::f), 1);
            lua_setglobal(this, name);
            return this;
        }
    };
    class object {
        int idx{};
        bool clean{};
    protected:
        lua_state* S = nullptr;
    public:
        object() = default;
        explicit object(lua_state* L, int idx = 0, bool clean = false)
        {
            this->clean = clean;
            this->S = L;
            this->idx = lua_absindex(static_cast<lua_State*>(L), idx == 0 ? lua_gettop(L) : idx);
        }
        ~object() { if (clean) lua_remove(S, idx); }
        [[nodiscard]] lua_state* state() const noexcept { return S; };
        [[nodiscard]] int index() const noexcept { return idx; }
        void push() const noexcept {
            lua_pushvalue(state(), index());
        }
        void pop() const noexcept {
            lua_remove(state(), index());
        }
        [[nodiscard]] const void* pointer() const noexcept {
            return lua_topointer(state(), index());
        }
        [[nodiscard]] int type() const noexcept {
            return lua_type(state(), index());
        }
        [[nodiscard]] bool valid() const noexcept {
            auto t = type();
            return index() != 0 && t != LUA_TNIL && t != LUA_TNONE;
        }
        template<typename T>
        [[nodiscard]] bool is() const noexcept {
            return detail::stack<T>::isInstance(state(), index());
        }
        template<>
        [[nodiscard]] bool is<int>() const noexcept {
            return lua_isinteger(state(), index());
        }
        template<>
        [[nodiscard]] bool is<float>() const noexcept {
            return lua_isnumber(state(), index());
        }
        template<>
        [[nodiscard]] bool is<bool>() const noexcept {
            return lua_isboolean(state(), index());
        }
        template<>
        [[nodiscard]] bool is<std::string>() const noexcept {
            return lua_isstring(state(), index());
        }
        template<>
        [[nodiscard]] bool is<const char*>() const noexcept {
            return lua_isstring(state(), index());
        }
        template<>
        [[nodiscard]] bool is<table>() const noexcept {
            return lua_istable(state(), index());
        }
        [[nodiscard]] bool is(int type) const noexcept {
            return this->type() == type;
        }
        bool operator==(const object& rhs) const noexcept {
            return lua_compare(state(), this->index(), rhs.index(), LUA_OPEQ);
        }
        [[nodiscard]] bool rawequal(const object& rhs) const noexcept {
            return lua_rawequal(state(), this->index(), rhs.index());
        }
        template <class T>
        T as() const {
            return detail::stack<T>::get(state(), index());
        }
        template <class T>
        T cast() const {
            return (T)lua_touserdata(state(), index());
        }
        template <class T>
        T check() const {
            if (is<T>())
            {
                return detail::stack<T>::get(state(), index());
            }
            luaL_error(state(), "Expected %s but found %s", detail::name_helper<T>::get(), lua_typename(state(), index()));
        }
        [[nodiscard]] const char* tostring() const {
            return lua_tostring(state(), index());
        }
        [[nodiscard]] const char* type_name() const noexcept {
            return lua_typename(state(), index());
        }
    };
    class table : public object {
    public:
        table(lua_state* L, int index) : object(L, index) {
            if (!lua_istable (L, index))
            {
                luaL_error (L, "#%d argument must be a table", index);
            }
        }
        [[nodiscard]] object operator[](size_t idx) const {
            lua_pushinteger(S, idx);
            lua_gettable(S, this->index());
            return object(S, -1, true);
        }
        [[nodiscard]] object operator[](const char* idx) const {
            lua_pushstring(S, idx);
            lua_gettable(S, this->index());
            return object(S, -1, true);
        }
    };
    class class_table : public table {
        static int lua_CIndexFunction(lua_state*S)
        {
            if (luaL_getmetafield(S, 1, "__pindex"))
            {
                lua_pushvalue(S, 2);
                if (lua_gettable(S, -2))
                {
                    lua_remove(S, -2);
                    lua_insert(S, 1);
                    S->call(lua_gettop(S) - 1, LUA_MULTRET);
                    return lua_gettop(S);
                } else lua_pop(S, 1);
            }
            if (luaL_getmetafield(S, 1, "__findex"))
            {
                lua_pushvalue(S, 2);
                if (lua_gettable(S, -2))
                {
                    return 1;
                } else lua_pop(S, 1);
            }
            if (luaL_getmetafield(S, 1, "__cindex"))
            {
                lua_insert(S, 1);
                S->call(lua_gettop(S) - 1, LUA_MULTRET);
                return lua_gettop(S);
            }
            return 0;
        }
        static int lua_CNewIndexFunction(lua_state*S)
        {
            if (luaL_getmetafield(S, 1, "__pnewindex"))
            {
                lua_pushvalue(S, 2);
                if (lua_gettable(S, -2))
                {
                    lua_remove(S, -2);
                    lua_insert(S, 1);
                    S->call(lua_gettop(S) - 1, LUA_MULTRET);
                    return 0;
                } else lua_pop(S, 1);
            }
            if (luaL_getmetafield(S, 1, "__fnewindex"))
            {
                lua_pushvalue(S, 2);
                if (lua_gettable(S, -2))
                {
                    lua_remove(S, -2);
                    lua_insert(S, 1);
                    S->call(lua_gettop(S) - 1, LUA_MULTRET);
                    return 0;
                } else lua_pop(S, 1);
            }
            if (luaL_getmetafield(S, 1, "__cnewindex"))
            {
                lua_insert(S, 1);
                S->call(lua_gettop(S) - 1, LUA_MULTRET);
                return 0;
            }
            return 0;
        }
        void push_findex_meta() {
            lua_pushstring(state(), "__findex");         // +1 | 0 | 1
            if (!lua_gettable(state(), index()))       //  0 | 1 | 1
            {
                lua_pop(state(), 1); // pop nil;        // -1 | 1 | 0
                lua_pushstring(state(), "__findex");     // +1 | 0 | 1
                lua_newtable(state());                    // +1 | 1 | 2
                lua_settable(state(), index());        // -2 | 2 | 0

                lua_pushstring(state(), "__findex");     // +1 | 0 | 1
                lua_gettable(state(), index());        //  0 | 1 | 1
            }
        }
        void push_fnewindex_meta() {
            lua_pushstring(state(), "__fnewindex");         // +1 | 0 | 1
            if (!lua_gettable(state(), index()))       //  0 | 1 | 1
            {
                lua_pop(state(), 1); // pop nil;        // -1 | 1 | 0
                lua_pushstring(state(), "__fnewindex");     // +1 | 0 | 1
                lua_newtable(state());                    // +1 | 1 | 2
                lua_settable(state(), index());        // -2 | 2 | 0

                lua_pushstring(state(), "__fnewindex");     // +1 | 0 | 1
                lua_gettable(state(), index());        //  0 | 1 | 1
            }
        }
        void push_pindex_meta() {
            lua_pushstring(state(), "__pindex");         // +1 | 0 | 1
            if (!lua_gettable(state(), index()))       //  0 | 1 | 1
            {
                lua_pop(state(), 1); // pop nil;        // -1 | 1 | 0
                lua_pushstring(state(), "__pindex");     // +1 | 0 | 1
                lua_newtable(state());                    // +1 | 1 | 2
                lua_settable(state(), index());        // -2 | 2 | 0

                lua_pushstring(state(), "__pindex");     // +1 | 0 | 1
                lua_gettable(state(), index());        //  0 | 1 | 1
            }
        }
        void push_pnewindex_meta() {
            lua_pushstring(state(), "__pnewindex");         // +1 | 0 | 1
            if (!lua_gettable(state(), index()))       //  0 | 1 | 1
            {
                lua_pop(state(), 1); // pop nil;        // -1 | 1 | 0
                lua_pushstring(state(), "__pnewindex");     // +1 | 0 | 1
                lua_newtable(state());                    // +1 | 1 | 2
                lua_settable(state(), index());        // -2 | 2 | 0

                lua_pushstring(state(), "__pnewindex");     // +1 | 0 | 1
                lua_gettable(state(), index());        //  0 | 1 | 1
            }
        }
    public:
        class_table(lua_state* L, int index) : table(L, index) {
            if (!lua_istable (L, index))
            {
                luaL_error (L, "#%d argument must be a table", index);
            }

            lua_pushstring(state(), "__index");                                                    // +1 | 1 | 2
            if (!lua_gettable(state(), this->index()))                                            //  0 | 2 | 2
            {
                lua_pop(state(), 1); // pop nil;                                                   // -1 | 2 | 1
                lua_pushstring(state(), "__index");                                                // +1 | 1 | 2
                lua_pushcclosure(state(), reinterpret_cast<lua_CFunction>(lua_CIndexFunction), 0); // +1 | 2 | 3
                lua_settable(state(), this->index());                                             // -2 | 3 | 1
            } else
                lua_pop(state(), 1); // pop nil;                                                   // -1 | 2 | 1

            lua_pushstring(state(), "__newindex");                                                 // +1 | 1 | 2
            if (!lua_gettable(state(), this->index()))                                            //  0 | 2 | 2
            {
                lua_pop(state(), 1); // pop nil;                                                   // -1 | 2 | 1
                lua_pushstring(state(), "__newindex");                                             // +1 | 1 | 2
                lua_pushcclosure(state(), reinterpret_cast<lua_CFunction>(lua_CNewIndexFunction), 0); // +1 | 2 | 3
                lua_settable(state(), this->index());                                             // -2 | 3 | 1
            } else
                lua_pop(state(), 1); // pop nil;                                                   // -1 | 2 | 1
        }
        template<typename F>
        class_table addCFunction(const char* name, F func)
        {
            push_findex_meta();                                                          // +1 | 0 | 1
            lua_pushstring(state(), name);                                            // +1 | 1 | 2
            lua_pushcclosure(state(), reinterpret_cast<lua_CFunction>(func), 0);    // +1 | 2 | 3
            lua_settable(state(), -3);                                             // -2 | 3 | 1
            lua_pop(state(), 1); // pop meta                                        // -1 | 1 | 0
            return class_table(state(), index());
        }
        template<typename F>
        class_table addCGetter(const char* name, F func)
        {
            push_pindex_meta();                                                          // +1 | 0 | 1
            lua_pushstring(state(), name);                                            // +1 | 1 | 2
            lua_pushcclosure(state(), reinterpret_cast<lua_CFunction>(func), 0);    // +1 | 2 | 3
            lua_settable(state(), -3);                                             // -2 | 3 | 1
            lua_pop(state(), 1); // pop meta                                        // -1 | 1 | 0
            return class_table(state(), index());
        }
        template<typename F>
        class_table addCSetter(const char* name, F func)
        {
            push_pnewindex_meta();                                                          // +1 | 0 | 1
            lua_pushstring(state(), name);                                            // +1 | 1 | 2
            lua_pushcclosure(state(), reinterpret_cast<lua_CFunction>(func), 0);    // +1 | 2 | 3
            lua_settable(state(), -3);                                             // -2 | 3 | 1
            lua_pop(state(), 1); // pop meta                                        // -1 | 1 | 0
            return class_table(state(), index());
        }
        template<typename F>
        class_table addCIndexFunction(F func)
        {
            lua_pushstring(state(), "__cindex");
            lua_pushcclosure(state(), reinterpret_cast<lua_CFunction>(func), 0);
            lua_settable(state(), index());
            return class_table(state(), index());
        }
        template<typename F>
        class_table addCMetaFunction(const char* name, F func)
        {
            lua_pushstring(state(), name);
            lua_pushcclosure(state(), reinterpret_cast<lua_CFunction>(func), 0);
            lua_settable(state(), index());
            return class_table(state(), index());
        }
        template <class T, class ReturnType, class... Params>
        class_table addFunction(char const* name, ReturnType(T::* func)(Params...))
        {
            using FnType = decltype (func);
            push_findex_meta();                                                                     // +1 | 0 | 1
            lua_pushstring(state(), name);                                                        // +1 | 1 | 2
            new (lua_newuserdatauv(state(), sizeof(func), 0)) FnType (func);            // +1 | 2 | 3
            lua_pushcclosure(state(), reinterpret_cast<lua_CFunction>(&detail::CallMember<FnType>::f), 1);
                                                                                                     //  0 | 3 | 3
            lua_settable(state(), -3);                                                         // -2 | 3 | 1
            lua_pop(state(), 1); // pop meta                                                    // -1 | 1 | 0
            return class_table(state(), index());
        }
        template <class T, class U>
        class_table addProperty(char const* name, U T::* mp, bool isWritable = true)
        {
            //typedef const U T::*mp_t;
            //new (lua_newuserdata (state(), sizeof (mp_t))) mp_t (mp);
            //lua_pushcclosure (state(), &detail::getVariable<T, U>, 1);
            this->addCGetter(name, &detail::getVariable<T, U>);
            //if (isWritable)
            //{
            //    this->addCGetter ( name, &detail::setVariable<T, U>);
            //}
            return *this;
        }
        void finalize() {
            this->pop();
        }
    };
    class ref {
        lua_state* S = nullptr;
        int idx = LUA_REFNIL;
    public:
        ref(lua_state* L, int idx)
        {
            this->S = L;
            lua_pushvalue(L, idx);
            this->idx = luaL_ref(L, LUA_REGISTRYINDEX);
        }
        [[nodiscard]] lua_state* state() const noexcept { return S; };
        [[nodiscard]] int index() const noexcept { return idx; }
        explicit ref(lua_state* L) {
            this->S = L;
            lua_pushvalue(L, lua_gettop(L));
            this->idx = luaL_ref(L, LUA_REGISTRYINDEX);
        }
        [[nodiscard]] object get() const noexcept {
            lua_rawgeti(S, LUA_REGISTRYINDEX, idx);
            return object(S, -1);
        }
        void push() const noexcept {
            lua_rawgeti(S, LUA_REGISTRYINDEX, idx);
        }
        void pop() const noexcept {
            luaL_unref(S, LUA_REGISTRYINDEX, idx);
        }
        [[nodiscard]] bool valid() const noexcept {
            return S != nullptr && idx != LUA_REFNIL;
        }
    };

    static int c_name_helper(lua_state*L)
    {
        char asdf[128]; snprintf(asdf, sizeof(asdf), "%s{%X}", lua_tostring(L, lua_upvalueindex(1)), (uint32_t)lua_topointer(L, 1));
        lua_pushstring(L, asdf);
        return 1;
    }

    template<typename T>
    class_table lua_state::addClass(const char* name)
    {
        if (meta_ref == LUA_REFNIL) {
            lua_newtable(this);
            meta_ref = luaL_ref(this, LUA_REGISTRYINDEX);
        }
        lua_rawgeti(this, LUA_REGISTRYINDEX, meta_ref);     // +1 | 0 | 1
        lua_newtable(this);                                       // +1 | 1 | 2
        lua_rawsetp(this, -2, detail::key_helper<T>::get());              // -1 | 2 | 1
        lua_rawgetp(this, -1, detail::key_helper<T>::get());              // +1 | 1 | 2
        lua_remove(this, -2);                                 // -1 | 2 | 1
        auto tbl = class_table(this, -1);
        lua_pushstring(this, "__tostring");
        lua_pushstring(this, name);
        lua_pushcclosure(this, reinterpret_cast<lua_CFunction>(c_name_helper), 1);
        lua_settable(this, tbl.index());
        detail::name_helper<T>::set(name);
        return tbl;
    }

    static int lua_openlib_mt(lua_state* L, const char* name, luaL_Reg* funcs, lua_CFunction indexer)
    {
        lua_newtable(L);                            // +1 | 0 | 1
        if (indexer) {
            lua_newtable(L);                        // +1 | 1 | 2
            lua_pushcfunction(L, indexer);          // +1 | 2 | 3
            lua_setfield(L, -2, "__index");   // -1 | 3 | 2
            lua_setmetatable(L, -2);        // -1 | 2 | 1
        }
        if (funcs) {
            luaL_setfuncs(L, funcs, 0);         //  0 | 1 | 1
        }
        lua_setglobal(L, name);                     // -1 | 1 | 0

        return 0;
    }

    namespace detail {
        template <>
        class stack <table>
        {
            static void push (lua_state*, table const& table)
            {
                table.push();
            }

            static table get (lua_state* L, int index)
            {
                if (!lua_istable (L, index))
                {
                    luaL_error (L, "#%d argument must be a table", index);
                }

                return table((lua_state*)L, index);
            }

            static bool isInstance (lua_state* L, int index)
            {
                return lua_istable (L, index);
            }
        };
    }

    [[nodiscard]] object lua_state::at(int idx) noexcept {
        return object(this, idx);
    }
    [[nodiscard]] object lua_state::top() noexcept {
        return object(this);
    }
}
