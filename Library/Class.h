#pragma once
#include "lua.hpp"
#include <functional>

namespace LuaBinding {
  static int pcall(lua_State *L, int narg = 0, int nres = 0) {
    lua_getglobal(L, "debug");
    lua_getfield(L, -1, "traceback");
    lua_remove(L, -2);
    int errindex = -narg - 2;
    lua_insert(L, errindex);
    auto status = lua_pcall(L, narg, nres, errindex);
    if (!status) lua_remove(L, errindex);
    return status;
  }

  namespace detail
  {
    template <typename F>
    struct function_traits : public function_traits<decltype(&F::operator())> {};

    template <typename R, typename C, typename... Args>
    struct function_traits<R (C::*)(Args...) const>
    {
      using function_type = std::function<R (Args...)>;
    };
  }

  template <typename F>
  using function_type_t = typename detail::function_traits<F>::function_type;

  template<class T>
  class helper {
  public:
    static void const* key() { static char value; return &value; }
  };

  static int metatables_index = LUA_REFNIL;

  template<typename T>
  class Class {
    lua_State* L = nullptr;
  public:
    Class() {}
    Class(lua_State* L, const char* name) : L(L)
    {
      if (metatables_index == LUA_REFNIL)
      {
        lua_newtable(L);
        metatables_index = luaL_ref(L, LUA_REGISTRYINDEX);
      }
      lua_rawgeti(L, LUA_REGISTRYINDEX, metatables_index);
      lua_newtable(L);
      lua_pushvalue(L, -1);
      lua_rawsetp(L, -3, helper<T>::key());
      lua_remove(L, -2),
          lua_pushstring(L, "__index");
      lua_pushcclosure(L, lua_CIndexFunction, 0);
      lua_settable(L, -3);
      lua_pushstring(L, "__newindex");
      lua_pushcclosure(L, lua_CNewIndexFunction, 0);
      lua_settable(L, -3);
      lua_pushstring(L, "__tostring");
      lua_pushstring(L, name);
      lua_pushcclosure(L, tostring, 1);
      lua_settable(L, -3);
      lua_pop(L, 1);
    }
  private:
    static int tostring(lua_State* S)
    {
      char asdf[128];
      snprintf(asdf, sizeof(asdf), "%s{%llX}", lua_tostring(S, lua_upvalueindex(1)), (uint64_t)lua_topointer(S, 1));
      lua_pushstring(S, asdf);
      return 1;
    }
    static int lua_CIndexFunction(lua_State* S)
    {
      if (luaL_getmetafield(S, 1, "__pindex"))
      {
        lua_pushvalue(S, 2);
        if (lua_gettable(S, -2))
        {
          lua_remove(S, -2);
          lua_insert(S, 1);
          pcall(S, lua_gettop(S) - 1, LUA_MULTRET);
          return lua_gettop(S);
        }
        else lua_pop(S, 1);
      }
      if (luaL_getmetafield(S, 1, "__findex"))
      {
        lua_pushvalue(S, 2);
        if (lua_gettable(S, -2))
        {
          return 1;
        }
        else lua_pop(S, 1);
      }
      return 0;
    }
    static int lua_CNewIndexFunction(lua_State* S)
    {
      if (luaL_getmetafield(S, 1, "__pnewindex"))
      {
        lua_pushvalue(S, 2);
        if (lua_gettable(S, -2))
        {
          lua_remove(S, -2);
          lua_insert(S, 1);
          pcall(S, lua_gettop(S) - 1, LUA_MULTRET);
          return 0;
        }
        else lua_pop(S, 1);
      }
      if (luaL_getmetafield(S, 1, "__fnewindex"))
      {
        lua_pushvalue(S, 2);
        if (lua_gettable(S, -2))
        {
          return 0;
        }
        else lua_pop(S, 1);
      }
      luaL_error(S, "No writable member %s", lua_tostring(S, 2));
      return 0;
    }
  private:
    void push_metatable()
    {
      lua_rawgeti(L, LUA_REGISTRYINDEX, metatables_index);
      lua_rawgetp(L, -1, helper<T>::key());
      lua_remove(L, -2);
    }
    void push_function_index()
    {
      push_metatable();
      lua_pushstring(L, "__findex");
      if (!lua_gettable(L, -2))
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
      if (!lua_gettable(L, -2))
      {
        lua_pop(L, 1);
        lua_newtable(L);
        lua_pushvalue(L, -1);
        lua_setfield(L, -3, "__pindex");
      }
      lua_remove(L, -2);
    }
    void push_function_newindex()
    {
      push_metatable();
      lua_pushstring(L, "__fnewindex");
      if (!lua_gettable(L, -2))
      {
        lua_pop(L, 1);
        lua_newtable(L);
        lua_pushvalue(L, -1);
        lua_setfield(L, -3, "__fnewindex");
      }
      lua_remove(L, -2);
    }
    void push_property_newindex()
    {
      push_metatable();
      lua_pushstring(L, "__pnewindex");
      if (!lua_gettable(L, -2))
      {
        lua_pop(L, 1);
        lua_newtable(L);
        lua_pushvalue(L, -1);
        lua_setfield(L, -3, "__pnewindex");
      }
      lua_remove(L, -2);
    }
  public:
    template <class R, class... Params>
    void fun(const char* name, R(* func)(Params...))
    {
      using FnType = decltype (func);
      push_function_index();
      new (lua_newuserdatauv(L, sizeof(func), 1)) FnType(func);
      lua_pushcclosure(L, TraitsNClass<R, T, Params...>::f, 1);
      lua_setfield(L, -2, name);
      lua_pop(L, 1);
    }

    template <class R, class... Params>
    void fun(const char* name, R(T::* func)(Params...))
    {
      using FnType = decltype (func);
      push_function_index();
      new (lua_newuserdatauv(L, sizeof(func), 1)) FnType(func);
      lua_pushcclosure(L, TraitsClass<R, T, Params...>::f, 1);
      lua_setfield(L, -2, name);
      lua_pop(L, 1);
    }

    template <class R, class... Params>
    void fun(const char* name, std::function<R(Params...)> func)
    {
      using FnType = decltype (func);
      push_function_index();
      new (lua_newuserdatauv(L, sizeof(func), 1)) FnType(func);
      lua_pushcclosure(L, TraitsFunClass<R, T, Params...>::f, 1);
      lua_setfield(L, -2, name);
      lua_pop(L, 1);
    }

    template <class F>
    void fun(const char* name, F&& func)
    {
      fun(name, static_cast<function_type_t<F>>(func));
    }

    template <class R> requires std::is_integral_v<R>
    void cfun(const char* name, R(T::*func)(lua_State*))
    {
      using func_t = decltype (func);
      push_function_index();
      new (lua_newuserdatauv(L, sizeof(func), 1)) func_t(func);
      lua_pushcclosure(L, TraitsClassLCFunc<T>::f, 1);
      lua_setfield(L, -2, name);
      lua_pop(L, 1);
    }

    template <class R> requires std::is_integral_v<R>
    void cfun(const char* name, std::function<R(T*, lua_State*)> func)
    {
      using FnType = decltype (func);
      push_function_index();
      new (lua_newuserdatauv(L, sizeof(func), 1)) FnType(func);
      lua_pushcclosure(L, TraitsClassFunLCFunc<T>::f, 1);
      lua_setfield(L, -2, name);
      lua_pop(L, 1);
    }

    template <class R> requires std::is_integral_v<R>
    void cfun(const char* name, R(*func)(lua_State*))
    {
      push_function_index();
      lua_pushcclosure(L, func, 0);
      lua_setfield(L, -2, name);
      lua_pop(L, 1);
    }

    template <class R> requires std::is_integral_v<R>
    void cfun(const char* name, std::function<R(lua_State*)> func)
    {
      using FnType = decltype (func);
      push_function_index();
      new (lua_newuserdatauv(L, sizeof(func), 1)) FnType(func);
      lua_pushcclosure(L, TraitsClassFunNLCFunc<T>::f, 1);
      lua_setfield(L, -2, name);
      lua_pop(L, 1);
    }

    template <class R, class U> requires std::is_integral_v<R>
    void cfun(const char* name, R(T::*func)(U*))
    {
      using func_t = decltype (func);
      push_function_index();
      new (lua_newuserdatauv(L, sizeof(func), 1)) func_t(func);
      lua_pushcclosure(L, TraitsClassCFunc<T>::f, 1);
      lua_setfield(L, -2, name);
      lua_pop(L, 1);
    }

    template <class R, class U> requires std::is_integral_v<R>
    void cfun(const char* name, R(*func)(U*))
    {
      push_function_index();
      lua_pushlightuserdata(L, reinterpret_cast<void*>(func));
      lua_pushcclosure(L, TraitsClassNCFunc<T>::f, 1);
      lua_setfield(L, -2, name);
      lua_pop(L, 1);
    }

    template <class R, class U> requires std::is_integral_v<R>
    void cfun(const char* name, std::function<R(U*)> func)
    {
      using FnType = decltype (func);
      push_function_index();
      new (lua_newuserdatauv(L, sizeof(func), 1)) FnType(func);
      lua_pushcclosure(L, TraitsClassFunNCFunc<T>::f, 1);
      lua_setfield(L, -2, name);
      lua_pop(L, 1);
    }

    template <class R, class U> requires std::is_integral_v<R>
    void cfun(const char* name, std::function<R(T*, U*)> func)
    {
      using FnType = decltype (func);
      push_function_index();
      new (lua_newuserdatauv(L, sizeof(func), 1)) FnType(func);
      lua_pushcclosure(L, TraitsClassFunCFunc<T>::f, 1);
      lua_setfield(L, -2, name);
      lua_pop(L, 1);
    }

    template <class F>
    void cfun(const char* name, F&& func)
    {
      cfun(name, static_cast<function_type_t<F>>(func));
    }

    template <class R, class... Params>
    void meta_fun(const char* name, R(* func)(Params...))
    {
      using FnType = decltype (func);
      push_metatable();
      new (lua_newuserdatauv(L, sizeof(func), 1)) FnType(func);
      lua_pushcclosure(L, TraitsNClass<R, T, Params...>::f, 1);
      lua_setfield(L, -2, name);
      lua_pop(L, 1);
    }

    template <class R, class... Params>
    void meta_fun(const char* name, R(T::* func)(Params...))
    {
      using FnType = decltype (func);
      push_metatable();
      new (lua_newuserdatauv(L, sizeof(func), 1)) FnType(func);
      lua_pushcclosure(L, TraitsClass<R, T, Params...>::f, 1);
      lua_setfield(L, -2, name);
      lua_pop(L, 1);
    }

    template <class R, class... Params>
    void meta_fun(const char* name, std::function<R(Params...)> func)
    {
      using FnType = decltype (func);
      push_metatable();
      new (lua_newuserdatauv(L, sizeof(func), 1)) FnType(func);
      lua_pushcclosure(L, TraitsFunClass<R, T, Params...>::f, 1);
      lua_setfield(L, -2, name);
      lua_pop(L, 1);
    }

    template <class F>
    void meta_fun(const char* name, F&& func)
    {
      meta_fun(name, static_cast<function_type_t<F>>(func));
    }

    template <class R> requires std::is_integral_v<R>
    void meta_cfun(const char* name, R(T::*func)(lua_State*))
    {
      using func_t = decltype (func);
      push_metatable();
      new (lua_newuserdatauv(L, sizeof(func), 1)) func_t(func);
      lua_pushcclosure(L, TraitsClassLCFunc<T>::f, 1);
      lua_setfield(L, -2, name);
      lua_pop(L, 1);
    }

    template <class R> requires std::is_integral_v<R>
    void meta_cfun(const char* name, std::function<R(T*, lua_State*)> func)
    {
      using FnType = decltype (func);
      push_metatable();
      new (lua_newuserdatauv(L, sizeof(func), 1)) FnType(func);
      lua_pushcclosure(L, TraitsClassFunLCFunc<T>::f, 1);
      lua_setfield(L, -2, name);
      lua_pop(L, 1);
    }

    template <class R> requires std::is_integral_v<R>
    void meta_cfun(const char* name, R(*func)(lua_State*))
    {
      push_metatable();
      lua_pushcclosure(L, func, 0);
      lua_setfield(L, -2, name);
      lua_pop(L, 1);
    }

    template <class R> requires std::is_integral_v<R>
    void meta_cfun(const char* name, std::function<R(lua_State*)> func)
    {
      using FnType = decltype (func);
      push_metatable();
      new (lua_newuserdatauv(L, sizeof(func), 1)) FnType(func);
      lua_pushcclosure(L, TraitsClassFunNLCFunc<T>::f, 1);
      lua_setfield(L, -2, name);
      lua_pop(L, 1);
    }

    template <class R, class U> requires std::is_integral_v<R>
    void meta_cfun(const char* name, R(T::*func)(U*))
    {
      using func_t = decltype (func);
      push_metatable();
      new (lua_newuserdatauv(L, sizeof(func), 1)) func_t(func);
      lua_pushcclosure(L, TraitsClassCFunc<T>::f, 1);
      lua_setfield(L, -2, name);
      lua_pop(L, 1);
    }

    template <class R, class U> requires std::is_integral_v<R>
    void meta_cfun(const char* name, R(*func)(U*))
    {
      push_metatable();
      lua_pushlightuserdata(L, reinterpret_cast<void*>(func));
      lua_pushcclosure(L, TraitsClassNCFunc<T>::f, 1);
      lua_setfield(L, -2, name);
      lua_pop(L, 1);
    }

    template <class R, class U> requires std::is_integral_v<R>
    void meta_cfun(const char* name, std::function<R(T*, U*)> func)
    {
      using FnType = decltype (func);
      push_metatable();
      new (lua_newuserdatauv(L, sizeof(func), 1)) FnType(func);
      lua_pushcclosure(L, TraitsClassFunCFunc<T>::f, 1);
      lua_setfield(L, -2, name);
      lua_pop(L, 1);
    }

    template <class F>
    void meta_cfun(const char* name, F&& func)
    {
      meta_cfun(name, static_cast<function_type_t<F>>(func));
    }

    template<class R>
    void prop(const char* name, R(T::* prop))
    {
      using PropType = decltype (prop);

      push_property_index();

      push_property_newindex();
      new (lua_newuserdatauv(L, sizeof(prop), 1)) PropType(prop);
      lua_pushvalue(L, -1);
      lua_pushcclosure(L, TraitsClassProperty<R, T>::set, 1);
      lua_setfield(L, -3, name);
      lua_remove(L, -2);

      lua_pushcclosure(L, TraitsClassProperty<R, T>::get, 1);
      lua_setfield(L, -2, name);
      lua_pop(L, 1);
    }

    template<class R>
    void prop_fun(const char* name, R(* get)(), void(* set)(R) = nullptr)
    {
      using get_t = decltype(get);
      using set_t = decltype(set);

      if (set)
      {
        push_property_newindex();
        new (lua_newuserdatauv(L, sizeof(set), 1)) set_t(set);
        lua_pushcclosure(L, TraitsClassPropertyNFn<R, T>::set, 1);
        lua_setfield(L, -2, name);
        lua_pop(L, 1);
      }

      push_property_index();
      new (lua_newuserdatauv(L, sizeof(get), 1)) get_t(get);
      lua_pushcclosure(L, TraitsClassPropertyNFn<R, T>::get, 1);
      lua_setfield(L, -2, name);
      lua_pop(L, 1);
    }

    template<class R>
    void prop_fun(const char* name, R(T::* get)(), void(T::* set)(R) = nullptr)
    {
      using get_t = decltype (get);
      using set_t = decltype (set);

      if (set)
      {
        push_property_newindex();
        new (lua_newuserdatauv(L, sizeof(set), 1)) set_t(set);
        lua_pushcclosure(L, TraitsClassPropertyFn<R, T>::set, 1);
        lua_setfield(L, -2, name);
        lua_pop(L, 1);
      }

      push_property_index();
      new (lua_newuserdatauv(L, sizeof(get), 1)) get_t(get);
      lua_pushcclosure(L, TraitsClassPropertyFn<R, T>::get, 1);
      lua_setfield(L, -2, name);
      lua_pop(L, 1);
    }

    template <class R>
    void prop_fun(const char* name, std::function<R()> get, std::function<void(R)> set = nullptr)
    {
      using get_t = decltype (get);
      using set_t = decltype (set);

      if (set)
      {
        push_property_newindex();
        new (lua_newuserdatauv(L, sizeof(set), 1)) set_t(set);
        lua_pushcclosure(L, TraitsClassPropertyNFunFn<R, T>::set, 1);
        lua_setfield(L, -2, name);
        lua_pop(L, 1);
      }

      push_property_index();
      new (lua_newuserdatauv(L, sizeof(get), 1)) get_t(get);
      lua_pushcclosure(L, TraitsClassPropertyNFunFn<R, T>::get, 1);
      lua_setfield(L, -2, name);
      lua_pop(L, 1);
    }

    template <class R>
    void prop_fun(const char* name, std::function<R(T*)> get, std::function<void(T*, R)> set = nullptr)
    {
      using get_t = decltype (get);
      using set_t = decltype (set);

      if (set)
      {
        push_property_newindex();
        new (lua_newuserdatauv(L, sizeof(set), 1)) set_t(set);
        lua_pushcclosure(L, TraitsClassPropertyFunFn<R, T>::set, 1);
        lua_setfield(L, -2, name);
        lua_pop(L, 1);
      }

      push_property_index();
      new (lua_newuserdatauv(L, sizeof(get), 1)) get_t(get);
      lua_pushcclosure(L, TraitsClassPropertyFunFn<R, T>::get, 1);
      lua_setfield(L, -2, name);
      lua_pop(L, 1);
    }

    template <class F, class G>
    void prop_fun(const char* name, F&& get, G&& set)
    {
      prop_fun(name, static_cast<function_type_t<F>>(get), static_cast<function_type_t<G>>(set));
    }

    template <class F, class G>
    void prop_fun(const char* name, F&& get)
    {
      prop_fun(name, static_cast<function_type_t<F>>(get));
    }

    template <class R, class U> requires std::is_integral_v<R>
    void prop_cfun(const char* name, R(*get)(U*), R(*set)(U*) = nullptr)
    {
      using get_t = decltype (get);
      using set_t = decltype (set);

      if (set)
      {
        push_property_newindex();
        new (lua_newuserdatauv(L, sizeof(set), 1)) set_t(set);
        lua_pushcclosure(L, TraitsClassPropertyNCFun<R>::set, 1);
        lua_setfield(L, -2, name);
        lua_pop(L, 1);
      }

      push_property_index();
      new (lua_newuserdatauv(L, sizeof(get), 1)) get_t(get);
      lua_pushcclosure(L, TraitsClassPropertyNCFun<R>::get, 1);
      lua_setfield(L, -2, name);
      lua_pop(L, 1);
    }

    template <class R, class U> requires std::is_integral_v<R>
    void prop_cfun(const char* name, R(T::*get)(U*), R(T::*set)(U*) = nullptr)
    {
      using get_t = decltype (get);
      using set_t = decltype (set);

      if (set)
      {
        push_property_newindex();
        new (lua_newuserdatauv(L, sizeof(set), 1)) set_t(set);
        lua_pushcclosure(L, TraitsClassPropertyCFun<R>::set, 1);
        lua_setfield(L, -2, name);
        lua_pop(L, 1);
      }

      push_property_index();
      new (lua_newuserdatauv(L, sizeof(get), 1)) get_t(get);
      lua_pushcclosure(L, TraitsClassPropertyCFun<R>::get, 1);
      lua_setfield(L, -2, name);
      lua_pop(L, 1);
    }

    template <class R> requires std::is_integral_v<R>
    void prop_cfun(const char* name, std::function<R(lua_State*)> get, std::function<R(lua_State*)> set = nullptr)
    {
      using get_t = decltype (get);
      using set_t = decltype (set);

      if (set)
      {
        push_property_newindex();
        new (lua_newuserdatauv(L, sizeof(set), 1)) set_t(set);
        lua_pushcclosure(L, TraitsClassPropertyNFunLCFn<R>::set, 1);
        lua_setfield(L, -2, name);
        lua_pop(L, 1);
      }

      push_property_index();
      new (lua_newuserdatauv(L, sizeof(get), 1)) get_t(get);
      lua_pushcclosure(L, TraitsClassPropertyNFunLCFn<R>::get, 1);
      lua_setfield(L, -2, name);
      lua_pop(L, 1);
    }

    template <class R> requires std::is_integral_v<R>
    void prop_cfun(const char* name, std::function<R(T*, lua_State*)> get, std::function<R(T*, lua_State*)> set = nullptr)
    {
      using get_t = decltype (get);
      using set_t = decltype (set);

      if (set)
      {
        push_property_newindex();
        new (lua_newuserdatauv(L, sizeof(set), 1)) set_t(set);
        lua_pushcclosure(L, TraitsClassPropertyFunLCFn<R, T>::set, 1);
        lua_setfield(L, -2, name);
        lua_pop(L, 1);
      }

      push_property_index();
      new (lua_newuserdatauv(L, sizeof(get), 1)) get_t(get);
      lua_pushcclosure(L, TraitsClassPropertyFunLCFn<R, T>::get, 1);
      lua_setfield(L, -2, name);
      lua_pop(L, 1);
    }

    template <class R, class U> requires std::is_integral_v<R>
    void prop_cfun(const char* name, std::function<R(U*)> get, std::function<R(U*)> set = nullptr)
    {
      using get_t = decltype (get);
      using set_t = decltype (set);

      if (set)
      {
        push_property_newindex();
        new (lua_newuserdatauv(L, sizeof(set), 1)) set_t(set);
        lua_pushcclosure(L, TraitsClassPropertyNFunCFn<R>::set, 1);
        lua_setfield(L, -2, name);
        lua_pop(L, 1);
      }

      push_property_index();
      new (lua_newuserdatauv(L, sizeof(get), 1)) get_t(get);
      lua_pushcclosure(L, TraitsClassPropertyNFunCFn<R>::get, 1);
      lua_setfield(L, -2, name);
      lua_pop(L, 1);
    }

    template <class R, class U> requires std::is_integral_v<R>
    void prop_cfun(const char* name, std::function<R(T*, U*)> get, std::function<R(T*, U*)> set = nullptr)
    {
      using get_t = decltype (get);
      using set_t = decltype (set);

      if (set)
      {
        push_property_newindex();
        new (lua_newuserdatauv(L, sizeof(set), 1)) set_t(set);
        lua_pushcclosure(L, TraitsClassPropertyFunCFn<R, T>::set, 1);
        lua_setfield(L, -2, name);
        lua_pop(L, 1);
      }

      push_property_index();
      new (lua_newuserdatauv(L, sizeof(get), 1)) get_t(get);
      lua_pushcclosure(L, TraitsClassPropertyFunCFn<R, T>::get, 1);
      lua_setfield(L, -2, name);
      lua_pop(L, 1);
    }

    template <class F, class G>
    void prop_cfun(const char* name, F&& get, G&& set = nullptr)
    {
      prop_cfun(name, static_cast<function_type_t<F>>(get), static_cast<function_type_t<G>>(set));
    }

    template <class F>
    void prop_cfun(const char* name, F&& get)
    {
      prop_cfun(name, static_cast<function_type_t<F>>(get));
    }
  };
}