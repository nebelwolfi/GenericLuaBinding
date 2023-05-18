#pragma once
#include <functional>
#include "Function.h"

namespace LuaBinding {

    template<class T>
    class helper {
    public:
        static void const* key() { static char value; return &value; }
        static bool push_metatable(lua_State * L) {
            lua_getglobal(L, "__METASTORE");
            if (lua_rawgetp(L, -1, key()))
            {
                lua_remove(L, -2);
                return false;
            }
            lua_pop(L, 1);
            lua_newtable(L);
            lua_pushvalue(L, -1);
            lua_rawsetp(L, -3, key());
            lua_remove(L, -2);
            return true;
        }
    };

    template<typename T>
    class Class {
        lua_State* L = nullptr;
        const char* class_name;
    public:
        Class(lua_State* L, const char* name) : L(L), class_name(name)
        {
            if (!push_metatable())
            {
                lua_pop(L, 1);
                return;
            }

            lua_pushcclosure(L, lua_CGCFunction, 0);
            lua_setfield(L, -2, "__gc");

            lua_pushcclosure(L, lua_CIndexFunction, 0);
            lua_setfield(L, -2, "__index");

            lua_pushcclosure(L, lua_EQFunction, 0);
            lua_setfield(L, -2, "__eq");

            lua_pushcclosure(L, lua_CNewIndexFunction, 0);
            lua_setfield(L, -2, "__newindex");

            lua_pushstring(L, name);
            lua_setfield(L, -2, "__name");

            lua_pushstring(L, name);
            lua_pushcclosure(L, tostring, 1);
            lua_setfield(L, -2, "__tostring");

#ifdef LUABINDING_DYN_CLASSES
            lua_pushcfunction(L, dyn_read);
            lua_setfield(L, -2, "Read");

            lua_pushcfunction(L, dyn_cast);
            lua_setfield(L, -2, "Cast");
#endif

            lua_pop(L, 1);

            push_property_index();
            lua_pushcfunction(L, get_ptr);
            lua_setfield(L, -2, "ptr");
#ifdef LUABINDING_DYN_CLASSES
            lua_pushcfunction(L, get_vtable);
            lua_setfield(L, -2, "vTable");
            lua_pop(L, 1);

            push_property_newindex();
            lua_pushcfunction(L, set_ptr);
            lua_setfield(L, -2, "ptr");
#endif
            lua_pop(L, 1);
        }
    private:
        static int lua_CGCFunction(lua_State* S)
        {
            if (luaL_getmetafield(S, 1, "__cgc"))
            {
                lua_pushvalue(S, 1);
                pcall(S, 1, 0);
            }
            auto u = (void**)lua_touserdata(S, 1);
            if (*(u + 1) == (void*)0xC0FFEE) {
                free(*u);
            }
            return 0;
        }
        static int get_vtable(lua_State *L)
        {
            lua_pushnumber(L, (double)**(uintptr_t**)lua_touserdata(L, 1));
            return 1;
        }
        static int get_ptr(lua_State* L)
        {
            lua_pushnumber(L, (double)*(uintptr_t*)lua_touserdata(L, 1));
            return 1;
        }
        static int set_ptr(lua_State *L)
        {
            *(uintptr_t*)lua_touserdata(L, 1) = (uintptr_t)lua_tonumber(L, 3);
            return 0;
        }
        static int tostring(lua_State* S)
        {
            char asdf[128];
            if constexpr (sizeof(ptrdiff_t) == 4)
                snprintf(asdf, sizeof(asdf), "%s{%X}", lua_tostring(S, lua_upvalueindex(1)), (uintptr_t)topointer(S, 1));
            else if constexpr (sizeof(ptrdiff_t) == 8)
                snprintf(asdf, sizeof(asdf), "%s{%llX}", lua_tostring(S, lua_upvalueindex(1)), (uintptr_t)topointer(S, 1));
            lua_pushstring(S, asdf);
            return 1;
        }
        static void* topointer(lua_State *L, int idx)
        {
            return *(void**)lua_touserdata(L, idx);
        }
        static int lua_EQFunction(lua_State* S)
        {
            if (!lua_isuserdata(S, 1) || !lua_isuserdata(S, 2)) lua_pushboolean(S, false);
            else
                lua_pushboolean(S, *(uintptr_t*)lua_touserdata(S, 1) == *(uintptr_t*)lua_touserdata(S, 2));
            return 1;
        }
        static int lua_CIndexFunction(lua_State* S)
        {
            if (strcmp(lua_tostring(S, 2), "ptr") != 0
                && strcmp(lua_tostring(S, 2), "valid") != 0
                && luaL_getmetafield(S, 1, "__valid"))
            {
                lua_pushvalue(S, 1);
                lua_call(S, 1, 1);
                if (!lua_toboolean(S, -1))
                {
                    lua_pushvalue(S, 1);
                    if (luaL_getmetafield(S, 1, "__name")) {
                        char asdf[128];
                        if constexpr (sizeof(ptrdiff_t) == 4)
                            snprintf(asdf, sizeof(asdf), "%s{%X}", lua_tostring(S, -1), (uintptr_t)topointer(S, 1));
                        else if constexpr (sizeof(ptrdiff_t) == 8)
                            snprintf(asdf, sizeof(asdf), "%s{%llX}", lua_tostring(S, -1), (uintptr_t)topointer(S, 1));
                        lua_pop(S, 1);

                        luaL_error(S, "Tried to access invalid %s (property '%s')", asdf, lua_tostring(S, 2));
                    } else {
                        lua_pop(S, 1);
                        luaL_error(S, "Tried to access invalid unknown object (property '%s')", lua_tostring(S, 2));
                    }
                    return 0; // ??
                }
                lua_pop(S, 1);
            }
            if (luaL_getmetafield(S, 1, "__pindex"))
            {
                lua_pushvalue(S, 2);
                if (luaL_gettable(S, -2))
                {
                    lua_remove(S, -2);
                    lua_insert(S, 1);
                    lua_call(S, lua_gettop(S) - 1, LUA_MULTRET);
                    return lua_gettop(S);
                }
                else lua_pop(S, 1);
            }
            if (luaL_getmetafield(S, 1, "__findex"))
            {
                lua_pushvalue(S, 2);
                if (luaL_gettable(S, -2))
                    return 1;
                else lua_pop(S, 1);
            }
            if (luaL_getmetafield(S, 1, lua_tostring(S, 2)))
                return 1;
            if (luaL_getmetafield(S, 1, "__cindex"))
            {
                lua_insert(S, 1);
                lua_call(S, lua_gettop(S) - 1, LUA_MULTRET);
                return lua_gettop(S);
            }
            return 0;
        }
        static int lua_CNewIndexFunction(lua_State* S)
        {
            if (luaL_getmetafield(S, 1, "__pnewindex"))
            {
                lua_pushvalue(S, 2);
                if (luaL_gettable(S, -2))
                {
                    lua_pushvalue(S, 1);
                    lua_pushvalue(S, 2);
                    lua_pushvalue(S, 3);
                    lua_call(S, 3, 0);
                    return 0;
                }
                else lua_pop(S, 1);
            }
            if (luaL_getmetafield(S, 1, "__cnewindex"))
            {
                lua_pushvalue(S, 1);
                lua_pushvalue(S, 2);
                lua_pushvalue(S, 3);
                lua_call(S, 3, 0);
                return 0;
            }
            lua_getmetatable(S, 1);
            lua_pushvalue(S, 2);
            lua_pushvalue(S, 3);
            lua_settable(S, -3);
            return 0;
        }
    private:
        bool push_metatable()
        {
            return helper<std::remove_pointer_t<std::decay_t<T>>>::push_metatable(L);
        }
        void push_function_index()
        {
            push_metatable();
            lua_pushstring(L, "__findex");
            if (!luaL_gettable(L, -2))
            {
                lua_pop(L, 1);
                lua_newtable(L);
                lua_pushvalue(L, -1);
                lua_setfield(L, -3, "__findex");
            }
            lua_remove(L, -2);
        }
        void push_property_index()
        {
            push_metatable();
            lua_pushstring(L, "__pindex");
            if (!luaL_gettable(L, -2))
            {
                lua_pop(L, 1);
                lua_newtable(L);
                lua_pushvalue(L, -1);
                lua_setfield(L, -3, "__pindex");
            }
            lua_remove(L, -2);
        }
        void push_property_newindex()
        {
            push_metatable();
            lua_pushstring(L, "__pnewindex");
            if (!luaL_gettable(L, -2))
            {
                lua_pop(L, 1);
                lua_newtable(L);
                lua_pushvalue(L, -1);
                lua_setfield(L, -3, "__pnewindex");
            }
            lua_remove(L, -2);
        }
    public:
        template<typename F>
        Class<T>& idx_cfun(F func)
        {
            push_metatable();
            lua_pushstring(L, "__cindex");
            lua_pushcclosure(L, reinterpret_cast<lua_CFunction>(func), 0);
            lua_settable(L, -3);
            lua_pop(L, 1);
            return *this;
        }
        
        template<class... Params>
        Class<T>& ctor()
        {
            auto str = std::string(class_name);
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
                lua_getfield(L, -1, real_class_name.c_str());
                if (lua_isnil(L, -1))
                {
                    lua_pop(L, 1);
                    push_metatable();
                }

                lua_createtable(L, 0, 1);
                lua_pushstring(L, "__call");
                lua_pushcclosure(L, TraitsCtor<T, Params...>::f, 0);
                lua_settable(L, -3);
                //lua_pushcclosure(L, lua_CGCProtector, 0);
                //lua_setfield(L, -2, "__newindex");

                lua_setmetatable(L, -2);

                lua_setfield(L, -2, real_class_name.c_str());

                return *this;
            } else {
                if (!luaL_getglobal(L, class_name))
                {
                    lua_pop(L, 1);
                    push_metatable();
                }

                lua_createtable(L, 0, 1);
                lua_pushstring(L, "__call");
                lua_pushcclosure(L, TraitsCtor<T, Params...>::f, 0);
                lua_settable(L, -3);
                //lua_pushcclosure(L, lua_CGCProtector, 0);
                //lua_setfield(L, -2, "__newindex");

                lua_setmetatable(L, -2);

                lua_setglobal(L, class_name);

                return *this;
            }
        }

        Class<T>& make_visible()
        {
            if (!luaL_getglobal(L, class_name))
            {
                lua_pop(L, 1);
                push_metatable();
                lua_setglobal(L, class_name);
            } else lua_pop(L, 1);
            return *this;
        }

        template<typename ...Funcs>
        Class<T>& overload(const char* name, Funcs... functions) {
            push_function_index();
            OverloadedFunction<T, Funcs...>(L, functions...);
            lua_setfield(L, -2, name);
            lua_pop(L, 1);
            return *this;
        }

        template <class F>
        Class<T>& fun(const char* name, F& func)
        {
            push_function_index();
            Function::fun<T>(L, func);
            lua_setfield(L, -2, name);
            lua_pop(L, 1);
            return *this;
        }

        template <class F>
        Class<T>& fun(const char* name, F&& func)
        {
            push_function_index();
            Function::fun<T>(L, func);
            lua_setfield(L, -2, name);
            lua_pop(L, 1);
            return *this;
        }

        template <class F>
        Class<T>& sfun(const char* name, F&& func)
        {
            fun(name, std::forward<F>(func));
            make_visible();
            lua_getglobal(L, class_name);
            push_function_index();
            lua_pushstring(L, name);
            lua_rawget(L, -2);
            lua_remove(L, -2);
            lua_setfield(L, -2, name);
            lua_pop(L, 1);
            return *this;
        }

        template <class F>
        Class<T>& vfun(const char* name, F&& func)
        {
            prop_fun(name, std::forward<F>(func));
            push_metatable();
            lua_pushstring(L, "__valid");
            push_property_index();
            lua_pushstring(L, name);
            lua_rawget(L, -2);
            lua_remove(L, -2);
            lua_settable(L, -3);
            lua_pop(L, 1);
            return *this;
        }

        template <class F>
        Class<T>& vcfun(const char* name, F&& func)
        {
            prop_cfun(name, std::forward<F>(func));
            push_metatable();
            lua_pushstring(L, "__valid");
            push_property_index();
            lua_pushstring(L, name);
            lua_rawget(L, -2);
            lua_remove(L, -2);
            lua_settable(L, -3);
            lua_pop(L, 1);
            return *this;
        }

        template <class F>
        Class<T>& cfun(const char* name, F& func)
        {
            push_function_index();
            Function::cfun<T>(L, func);
            lua_setfield(L, -2, name);
            lua_pop(L, 1);
            return *this;
        }

        template <class F>
        Class<T>& cfun(const char* name, F&& func)
        {
            push_function_index();
            Function::cfun<T>(L, func);
            lua_setfield(L, -2, name);
            lua_pop(L, 1);
            return *this;
        }

        template <class F>
        Class<T>& scfun(const char* name, F&& func)
        {
            cfun(name, std::forward<F>(func));
            make_visible();
            lua_getglobal(L, class_name);
            push_function_index();
            lua_pushstring(L, name);
            lua_rawget(L, -2);
            lua_remove(L, -2);
            lua_setfield(L, -2, name);
            lua_pop(L, 1);
            return *this;
        }

        template <class F>
        Class<T>& meta_fun(const char* name, F func)
        {
            if (strcmp(name, "__gc") == 0) {
                return meta_fun("__cgc", std::forward<decltype(func)>(func));
            }
            using FnType = decltype (func);
            push_metatable();
            Function::fun<T>(L, func);
            lua_setfield(L, -2, name);
            lua_pop(L, 1);
            return *this;
        }

        template <class F>
        Class<T>& meta_cfun(const char* name, F func)
        {
            if (strcmp(name, "__gc") == 0) {
                return meta_cfun("__cgc", std::forward<decltype(func)>(func));
            }
            using func_t = decltype (func);
            push_metatable();
            Function::cfun<T>(L, func);
            lua_setfield(L, -2, name);
            lua_pop(L, 1);
            return *this;
        }

        template<class R>
        Class<T>& prop(const char* name, R(T::* prop))
        {
            using PropType = decltype (prop);

            push_property_index();

            push_property_newindex();
            new (lua_newuserdata(L, sizeof(prop))) PropType(prop);
            lua_pushvalue(L, -1);
            lua_pushcclosure(L, TraitsClassProperty<std::decay_t<R>, std::decay_t<T>>::set, 1);
            lua_setfield(L, -3, name);
            lua_remove(L, -2);

            lua_pushcclosure(L, TraitsClassProperty<std::decay_t<R>, std::decay_t<T>>::get, 1);
            lua_setfield(L, -2, name);
            lua_pop(L, 1);
            return *this;
        }

        template<class R>
        Class<T>& prop_fun(const char* name, R(* get)(), void(* set)(R) = nullptr)
        {
            using get_t = decltype(get);
            using set_t = decltype(set);

            if (set)
            {
                push_property_newindex();
                new (lua_newuserdata(L, sizeof(set))) set_t(set);
                lua_pushcclosure(L, TraitsClassPropertyNFn<R, T>::set, 1);
                lua_setfield(L, -2, name);
                lua_pop(L, 1);
            }

            push_property_index();
            new (lua_newuserdata(L, sizeof(get))) get_t(get);
            lua_pushcclosure(L, TraitsClassPropertyNFn<R, T>::get, 1);
            lua_setfield(L, -2, name);
            lua_pop(L, 1);
            return *this;
        }

        template<class R>
        Class<T>& prop_fun(const char* name, R(T::* get)(), void(T::* set)(R) = nullptr)
        {
            using get_t = decltype (get);
            using set_t = decltype (set);

            if (set)
            {
                push_property_newindex();
                new (lua_newuserdata(L, sizeof(set))) set_t(set);
                lua_pushcclosure(L, TraitsClassPropertyFn<R, T>::set, 1);
                lua_setfield(L, -2, name);
                lua_pop(L, 1);
            }

            push_property_index();
            new (lua_newuserdata(L, sizeof(get))) get_t(get);
            lua_pushcclosure(L, TraitsClassPropertyFn<R, T>::get, 1);
            lua_setfield(L, -2, name);
            lua_pop(L, 1);
            return *this;
        }

        template<class R>
        Class<T>& prop_fun(const char* name, R(T::* get)() const, void(T::* set)(R) const = nullptr)
        {
            using get_t = std::decay_t<decltype (get)>;
            using set_t = std::decay_t<decltype (set)>;

            if (set)
            {
                push_property_newindex();
                new (lua_newuserdata(L, sizeof(set))) set_t(set);
                lua_pushcclosure(L, TraitsClassPropertyFn<R, T>::set, 1);
                lua_setfield(L, -2, name);
                lua_pop(L, 1);
            }

            push_property_index();
            new (lua_newuserdata(L, sizeof(get))) get_t(get);
            lua_pushcclosure(L, TraitsClassPropertyFn<R, T>::get, 1);
            lua_setfield(L, -2, name);
            lua_pop(L, 1);
            return *this;
        }

        template <class R>
        Class<T>& prop_fun(const char* name, std::function<R()> get, std::function<void(R)> set = nullptr)
        {
            using get_t = decltype (get);
            using set_t = decltype (set);

            if (set)
            {
                push_property_newindex();
                new (lua_newuserdata(L, sizeof(set))) set_t(set);
                lua_pushcclosure(L, TraitsClassPropertyNFunFn<R, T>::set, 1);
                lua_setfield(L, -2, name);
                lua_pop(L, 1);
            }

            push_property_index();
            new (lua_newuserdata(L, sizeof(get))) get_t(get);
            lua_pushcclosure(L, TraitsClassPropertyNFunFn<R, T>::get, 1);
            lua_setfield(L, -2, name);
            lua_pop(L, 1);
            return *this;
        }

        template <class R>
        Class<T>& prop_fun(const char* name, std::function<R(T*)> get, std::function<void(T*, R)> set = nullptr)
        {
            using get_t = decltype (get);
            using set_t = decltype (set);

            if (set)
            {
                push_property_newindex();
                new (lua_newuserdata(L, sizeof(set))) set_t(set);
                lua_pushcclosure(L, TraitsClassPropertyFunFn<R, T>::set, 1);
                lua_setfield(L, -2, name);
                lua_pop(L, 1);
            }

            push_property_index();
            new (lua_newuserdata(L, sizeof(get))) get_t(get);
            lua_pushcclosure(L, TraitsClassPropertyFunFn<R, T>::get, 1);
            lua_setfield(L, -2, name);
            lua_pop(L, 1);
            return *this;
        }

        template <class F, class G>
        Class<T>& prop_fun(const char* name, F&& get, G&& set)
        {
            prop_fun(name, static_cast<function_type_t<std::decay_t<F>>>(get), static_cast<function_type_t<std::decay_t<G>>>(set));
            return *this;
        }

        template <class F>
        Class<T>& prop_fun(const char* name, F&& get)
        {
            prop_fun(name, static_cast<function_type_t<std::decay_t<F>>>(get));
            return *this;
        }

        template <class R, class U> requires std::is_integral_v<R>
        Class<T>& prop_cfun(const char* name, R(*get)(U), R(*set)(U) = nullptr)
        {
            using get_t = decltype (get);
            using set_t = decltype (set);

            if (set)
            {
                push_property_newindex();
                new (lua_newuserdata(L, sizeof(set))) set_t(set);
                lua_pushcclosure(L, TraitsClassPropertyNCFun<R, T>::set, 1);
                lua_setfield(L, -2, name);
                lua_pop(L, 1);
            }

            push_property_index();
            new (lua_newuserdata(L, sizeof(get))) get_t(get);
            lua_pushcclosure(L, TraitsClassPropertyNCFun<R, T>::get, 1);
            lua_setfield(L, -2, name);
            lua_pop(L, 1);
            return *this;
        }

        template <class R, class U> requires std::is_integral_v<R>
        Class<T>& prop_cfun(const char* name, R(T::*get)(U), R(T::*set)(U) = nullptr)
        {
            using get_t = decltype (get);
            using set_t = decltype (set);

            if (set)
            {
                push_property_newindex();
                new (lua_newuserdata(L, sizeof(set))) set_t(set);
                lua_pushcclosure(L, TraitsClassPropertyCFun<R, T>::set, 1);
                lua_setfield(L, -2, name);
                lua_pop(L, 1);
            }

            push_property_index();
            new (lua_newuserdata(L, sizeof(get))) get_t(get);
            lua_pushcclosure(L, TraitsClassPropertyCFun<R, T>::get, 1);
            lua_setfield(L, -2, name);
            lua_pop(L, 1);
            return *this;
        }

        template <class R> requires std::is_integral_v<R>
        Class<T>& prop_cfun(const char* name, std::function<R(lua_State*)> get, std::function<R(lua_State*)> set = nullptr)
        {
            using get_t = decltype (get);
            using set_t = decltype (set);

            if (set)
            {
                push_property_newindex();
                new (lua_newuserdata(L, sizeof(set))) set_t(set);
                lua_pushcclosure(L, TraitsClassPropertyNFunLCFn<R, T>::set, 1);
                lua_setfield(L, -2, name);
                lua_pop(L, 1);
            }

            push_property_index();
            new (lua_newuserdata(L, sizeof(get))) get_t(get);
            lua_pushcclosure(L, TraitsClassPropertyNFunLCFn<R, T>::get, 1);
            lua_setfield(L, -2, name);
            lua_pop(L, 1);
            return *this;
        }

        template <class R> requires std::is_integral_v<R>
        Class<T>& prop_cfun(const char* name, std::function<R(T*, lua_State*)> get, std::function<R(T*, lua_State*)> set = nullptr)
        {
            using get_t = decltype (get);
            using set_t = decltype (set);

            if (set)
            {
                push_property_newindex();
                new (lua_newuserdata(L, sizeof(set))) set_t(set);
                lua_pushcclosure(L, TraitsClassPropertyFunLCFn<R, T>::set, 1);
                lua_setfield(L, -2, name);
                lua_pop(L, 1);
            }

            push_property_index();
            new (lua_newuserdata(L, sizeof(get))) get_t(get);
            lua_pushcclosure(L, TraitsClassPropertyFunLCFn<R, T>::get, 1);
            lua_setfield(L, -2, name);
            lua_pop(L, 1);
            return *this;
        }

        template <class R, class U> requires std::is_integral_v<R>
        Class<T>& prop_cfun(const char* name, std::function<R(U)> get, std::function<R(U)> set = nullptr)
        {
            using get_t = decltype (get);
            using set_t = decltype (set);

            if (set)
            {
                push_property_newindex();
                new (lua_newuserdata(L, sizeof(set))) set_t(set);
                lua_pushcclosure(L, TraitsClassPropertyNFunCFn<R, T>::set, 1);
                lua_setfield(L, -2, name);
                lua_pop(L, 1);
            }

            push_property_index();
            new (lua_newuserdata(L, sizeof(get))) get_t(get);
            lua_pushcclosure(L, TraitsClassPropertyNFunCFn<R, T>::get, 1);
            lua_setfield(L, -2, name);
            lua_pop(L, 1);
            return *this;
        }

        template <class R, class U> requires std::is_integral_v<R>
        Class<T>& prop_cfun(const char* name, std::function<R(T*, U)> get, std::function<R(T*, U)> set = nullptr)
        {
            using get_t = decltype (get);
            using set_t = decltype (set);

            if (set)
            {
                push_property_newindex();
                new (lua_newuserdata(L, sizeof(set))) set_t(set);
                lua_pushcclosure(L, TraitsClassPropertyFunLCFn<R, T>::set, 1);
                lua_setfield(L, -2, name);
                lua_pop(L, 1);
            }

            push_property_index();
            new (lua_newuserdata(L, sizeof(get))) get_t(get);
            lua_pushcclosure(L, TraitsClassPropertyFunLCFn<R, T>::get, 1);
            lua_setfield(L, -2, name);
            lua_pop(L, 1);
            return *this;
        }

        template <class F, class G>
        Class<T>& prop_cfun(const char* name, F&& get, G&& set)
        {
            prop_cfun(name, static_cast<function_type_t<std::decay_t<F>>>(get), static_cast<function_type_t<std::decay_t<G>>>(set));
            return *this;
        }

        template <class F>
        Class<T>& prop_cfun(const char* name, F&& get)
        {
            prop_cfun(name, static_cast<function_type_t<std::decay_t<F>>>(get));
            return *this;
        }

        static int dyn_read(lua_State *L)
        {
            auto p = (void**)(uintptr_t)lua_tonumber(L, 2);
            if (p == nullptr || *p == nullptr)
            {
                lua_pushnil(L);
                return 1;
            }

            auto u = (void**)lua_newuserdata(L, sizeof(void*) * 2);
            *u = *p; *(u + 1) = 0;

            helper<std::remove_pointer_t<std::decay_t<T>>>::push_metatable(L);
            lua_setmetatable(L, -2);

            return 1;
        }
        static int dyn_cast(lua_State *L)
        {
            auto p = (void*)(uintptr_t)lua_tonumber(L, 2);
            if (p == 0)
            {
                lua_pushnil(L);
                return 1;
            }


            auto u = (void**)lua_newuserdata(L, sizeof(void*) * 2);
            *u = p; *(u + 1) = 0;

            helper<std::remove_pointer_t<std::decay_t<T>>>::push_metatable(L);
            lua_setmetatable(L, -2);

            return 1;
        }
    };
}