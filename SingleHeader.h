#pragma once

#include "lua.hpp"
#include <tuple>
#include <type_traits>
#include <functional>
#include <vector>
#include <functional>
#include <cassert>


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
};

template<typename T>
class StackClass {
public:
  static int push(lua_State* L, T& t)
  {
    lua_pushlightuserdata(L, &t);
    lua_rawgeti(L, LUA_REGISTRYINDEX, metatables_index);
    lua_rawgetp(L, -1, helper<T>::key());
    lua_setmetatable(L, -3);
    lua_pop(L, 1);
    return 1;
  }
  static bool is(lua_State* L, int index) {
    return lua_islightuserdata(L, index) || lua_isuserdata(L, index);
  }
  static T get(lua_State* L, int index)
  {
    if (!is(L, index))
      luaL_argerror(L, index, typeid(T).name());
    if constexpr (std::is_convertible_v<T, void*>){
      return (T)lua_touserdata(L, index);
    } else {
      return *(T*)lua_touserdata(L, index);
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
    return lua_isinteger(L, index);
  }
  static int get(lua_State* L, int index)
  {
    return lua_tointeger(L, index);
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
    return lua_tonumber(L, index);
  }
};

template<class T> requires std::is_same_v<T, const char*>
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
};

template<>
class Stack<bool> {
public:
  static int push(lua_State* L, bool t)
  {
    lua_pushboolean(L, t);
    return 1;
  }
  static bool is(lua_State* L, int index) {
    return true;
  }
  static bool get(lua_State* L, int index)
  {
    return lua_toboolean(L, index);
  }
};

namespace detail {
template<typename T>
concept is_pushable = requires(T a) {
  { Stack<T>::is(nullptr, 0) };
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

template<class T> requires !is_pushable<T>
int push(lua_State*L, T& t)
{
  return StackClass<T>::push(L, t);
}

template<class T> requires is_pushable<T>
T get(lua_State* L, int index)
{
  return Stack<T>::get(L, index);
}

template<class T> requires !is_pushable<T>
T get(lua_State* L, int index)
{
  return StackClass<T>::get(L, index);
}
}
}


namespace LuaBinding {
class Object {
  lua_State* L = nullptr;
  int index = LUA_REFNIL;
public:
  Object() = default;
  Object(lua_State* L, int index) : L(L)
  {
    this->index = lua_absindex(L, index);
  }

  const char* tostring()
  {
    return luaL_tolstring(L, index, NULL);
  }

  template<typename T>
  T as()
  {
    return detail::get<T>(L, index);
  }

  template<typename T>
  bool is()
  {
    return Stack<T>::is(L, index);
  }

  Object operator[](size_t idx)
  {
    lua_geti(L, index, idx);
    return Object(L, lua_gettop(L));
  }

  Object operator[](const char* idx)
  {
    lua_getfield(L, index, idx);
    return Object(L, lua_gettop(L));
  }

  int push()
  {
    lua_pushvalue(L, index);
    return 1;
  }
};

template<>
class Stack<Object> {
public:
  static int push(lua_State* L, Object t)
  {
    return t.push();
  }
  static bool is(lua_State* L, int index) {
    return true;
  }
  static Object get(lua_State* L, int index)
  {
    return Object(L, index);
  }
};
}


template<typename T>
concept is_pushable = requires(T a) {
  { LuaBinding::Stack<T>::is(nullptr, 0) };
};

namespace LuaBinding {
class State;

template <typename T>
struct is_boolean :std::is_same<bool, T> {};
template <typename T>
inline constexpr bool is_boolean_v = is_boolean<T>::value;

template< class T >
struct is_void : std::is_same<void, typename std::remove_cv<T>::type> {};

template <std::size_t I = 0, std::size_t J = 0, typename ... Ts>
void assign_tup(lua_State* L, State* S, std::tuple<Ts...> &tup) {
  if constexpr (I == sizeof...(Ts)) {
    return;
  } else {
    using Type = typename std::tuple_element<I, std::tuple<Ts...>>::type;

    if constexpr (std::is_same_v<Type, State*>) {
      std::get<I>(tup) = S;
    }
    else if constexpr (std::is_same_v<Type, Object>) {
      std::get<I>(tup) = Object(L, I + 1 - J);
    }
    else if constexpr (!is_pushable<Type>) {
      if (!StackClass<Type>::is(L, I + 1 - J))
        luaL_typeerror(L, I + 1 - J, typeid(Type).name());
      std::get<I>(tup) = (Type)StackClass<Type>::get(L, I + 1 - J);
    }
    else if constexpr (Stack<Type>::get != 0) {
      if (!Stack<Type>::is(L, I + 1 - J))
        luaL_typeerror(L, I + 1 - J, typeid(Type).name());
      std::get<I>(tup) = (Type)Stack<Type>::get(L, I + 1 - J);
    }
    else if constexpr (std::is_convertible<Type, void*>::value) {
      std::get<I>(tup) = (Type)lua_touserdata(L, I + 1 - J);
    }
    else
    {
      static_assert(sizeof(Type) > 0, "Unhandled specialization");
    }
    assign_tup<I + 1>(L, S, tup);
  }
}

template <class R, class... Params>
class Traits {
  using ParamList = std::tuple<Params...>;
public:
  static int f(lua_State* L) {
    auto state = (State*)lua_touserdata(L, lua_upvalueindex(1));
    auto fnptr = reinterpret_cast <R(*)(Params...)> (lua_touserdata(L, lua_upvalueindex(2)));
    ParamList params;
    assign_tup(L, state, params);
    if constexpr (std::is_void_v<R>) {
      std::apply(fnptr, params);
      return 0;
    } else {
      R result = std::apply(fnptr, params);
      return detail::push(L, result);
    }
  }
};

template <class R, class... Params>
class TraitsSTDFunction {
  using ParamList = std::tuple<Params...>;
public:
  static int f(lua_State* L) {
    auto state = (State*)lua_touserdata(L, lua_upvalueindex(1));
    auto fnptr = reinterpret_cast <std::function<R(Params...)>*> (lua_touserdata(L, lua_upvalueindex(2)));
    ParamList params;
    assign_tup(L, state, params);
    if constexpr (std::is_void_v<R>) {
      std::apply(*fnptr, params);
      return 0;
    } else {
      R result = std::apply(*fnptr, params);
      return detail::push(L, result);
    }
  }
};

class TraitsCFunc {
public:
  static int f(lua_State* L) {
    auto state = (State*)lua_touserdata(L, lua_upvalueindex(1));
    auto fnptr = reinterpret_cast <int(*)(State*)> (lua_touserdata(L, lua_upvalueindex(2)));
    return fnptr(state);
  }
};

template <class R, class T, class... Params>
class TraitsClass {
  using ParamList = std::tuple<T*, Params...>;
public:
  static int f(lua_State* L) {
    printf("hi1\n");
    lua_rawgeti(L, LUA_REGISTRYINDEX, 4);
    auto state = (State*)lua_touserdata(L, -1);
    lua_pop(L, 1);
    auto fnptr = *static_cast <R(T::**)(Params...)> (lua_touserdata(L, lua_upvalueindex(1)));
    ParamList params;
    std::get<0>(params) = StackClass<T*>::get(L, 1);
    assign_tup<1, 1>(L, state, params);
    printf("hi2\n");
    if constexpr (std::is_void_v<R>) {
      std::apply(fnptr, params);
      return 0;
    } else {
      R result = std::apply(fnptr, params);
      return detail::push(L, result);
    }
  }
};

template <class R, class T, class... Params>
class TraitsFunClass {
  using ParamList = std::tuple<Params...>;
public:
  static int f(lua_State* L) {
    lua_rawgeti(L, LUA_REGISTRYINDEX, 4);
    auto state = (State*)lua_touserdata(L, -1);
    lua_pop(L, 1);
    auto fnptr = *static_cast <std::function<R(Params...)>*> (lua_touserdata(L, lua_upvalueindex(1)));
    ParamList params;
    assign_tup(L, state, params);
    if constexpr (std::is_void_v<R>) {
      std::apply(fnptr, params);
      return 0;
    } else {
      R result = std::apply(fnptr, params);
      return detail::push(L, result);
    }
  }
};

template <class R, class T, class... Params>
class TraitsNClass {
  using ParamList = std::tuple<Params...>;
public:
  static int f(lua_State* L) {
    lua_rawgeti(L, LUA_REGISTRYINDEX, 4);
    auto state = (State*)lua_touserdata(L, -1);
    lua_pop(L, 1);
    auto fnptr = *static_cast <R(**)(Params...)> (lua_touserdata(L, lua_upvalueindex(1)));
    ParamList params;
    assign_tup(L, state, params);
    if constexpr (std::is_void_v<R>) {
      std::apply(fnptr, params);
      return 0;
    } else {
      R result = std::apply(fnptr, params);
      return detail::push(L, result);
    }
  }
};
template <class T>
class TraitsClassNCFunc {
public:
  static int f(lua_State* L) {
    auto t = (T*)lua_touserdata(L, 1);
    lua_rawgeti(L, LUA_REGISTRYINDEX, 4);
    auto state = (State*)lua_touserdata(L, -1);
    lua_pop(L, 1);
    auto fnptr = reinterpret_cast <int(*)(State*)> (lua_touserdata(L, lua_upvalueindex(1)));
    return fnptr(state);
  }
};
template <class T>
class TraitsClassFunNCFunc {
public:
  static int f(lua_State* L) {
    auto t = (T*)lua_touserdata(L, 1);
    lua_rawgeti(L, LUA_REGISTRYINDEX, 4);
    auto state = (State*)lua_touserdata(L, -1);
    lua_pop(L, 1);
    auto fnptr = *reinterpret_cast <std::function<int(State*)>*> (lua_touserdata(L, lua_upvalueindex(1)));
    return fnptr(state);
  }
};
template <class T>
class TraitsClassCFunc {
public:
  static int f(lua_State* L) {
    auto t = StackClass<T>::get(L, 1);
    lua_rawgeti(L, LUA_REGISTRYINDEX, 4);
    auto state = (State*)lua_touserdata(L, -1);
    lua_pop(L, 1);
    auto fnptr = reinterpret_cast <int(T::*)(State*)> (lua_touserdata(L, lua_upvalueindex(1)));
    return fnptr(t, state);
  }
};
template <class T>
class TraitsClassFunCFunc {
public:
  static int f(lua_State* L) {
    auto t = StackClass<T>::get(L, 1);
    lua_rawgeti(L, LUA_REGISTRYINDEX, 4);
    auto state = (State*)lua_touserdata(L, -1);
    lua_pop(L, 1);
    auto fnptr = reinterpret_cast <std::function<int(T*, State*)>*> (lua_touserdata(L, lua_upvalueindex(1)));
    return fnptr(t, state);
  }
};
template <class T>
class TraitsClassLCFunc {
  using func = int(T::*)(lua_State*);
public:
  static int f(lua_State* L) {
    auto t = StackClass<T>::get(L, 1);
    auto fnptr = (func*)lua_touserdata(L, lua_upvalueindex(1));
    return (t->**fnptr)(L);
  }
};
template <class T>
class TraitsClassFunLCFunc {
  using func = int(T::*)(lua_State*);
public:
  static int f(lua_State* L) {
    auto t = StackClass<T>::get(L, 1);
    auto fnptr = reinterpret_cast <std::function<int(T*, lua_State*)>*> (lua_touserdata(L, lua_upvalueindex(1)));
    return fnptr(t, L);
  }
};
template <class T>
class TraitsClassFunNLCFunc {
public:
  static int f(lua_State* L) {
    auto t = StackClass<T>::get(L, 1);
    auto fnptr = *reinterpret_cast <std::function<int(lua_State*)>*> (lua_touserdata(L, lua_upvalueindex(1)));
    return fnptr(L);
  }
};

template <class R, class T>
class TraitsClassProperty {
  using prop = R(T::*);
public:
  static int set(lua_State* L) {
    auto t = StackClass<T*>::get(L, 1);
    auto p = (prop*)lua_touserdata(L, lua_upvalueindex(1));
    t->**p = detail::get<R>(L, 3);
    return 0;
  }
  static int get(lua_State* L) {
    auto t = StackClass<T*>::get(L, 1);
    auto p = (prop*)lua_touserdata(L, lua_upvalueindex(1));
    return detail::push(L, t->**p);
  }
};

template <class R, class T>
class TraitsClassPropertyNFn {
  using set_t = void(*)(R);
  using get_t = R(*)();
public:
  static int set(lua_State* L) {
    auto p = *(set_t*)lua_touserdata(L, lua_upvalueindex(1));
    p(detail::get<R>(L, 3));
    return 0;
  }
  static int get(lua_State* L) {
    auto p = *(get_t*)lua_touserdata(L, lua_upvalueindex(1));
    return detail::push(L, p());
  }
};

template <class R, class T>
class TraitsClassPropertyFn {
  using set_t = void(T::*)(R);
  using get_t = R(T::*)();
public:
  static int set(lua_State* L) {
    auto t = StackClass<T*>::get(L, 1);
    auto p = (set_t*)lua_touserdata(L, lua_upvalueindex(1));
    (t->**p)(detail::get<R>(L, 3));
    return 0;
  }
  static int get(lua_State* L) {
    auto t = StackClass<T*>::get(L, 1);
    auto p = (get_t*)lua_touserdata(L, lua_upvalueindex(1));
    return detail::push(L, (t->**p)());
  }
};

template <class R, class T>
class TraitsClassPropertyNFunFn {
  using set_t = std::function<void(R)>;
  using get_t = std::function<R()>;
public:
  static int set(lua_State* L) {
    auto t = StackClass<T*>::get(L, 1);
    auto p = *(set_t*)lua_touserdata(L, lua_upvalueindex(1));
    p(detail::get<R>(L, 3));
    return 0;
  }
  static int get(lua_State* L) {
    auto t = StackClass<T*>::get(L, 1);
    auto p = *(get_t*)lua_touserdata(L, lua_upvalueindex(1));
    return detail::push(L, p());
  }
};

template <class R, class T>
class TraitsClassPropertyFunFn {
  using set_t = std::function<void(T*, R)>;
  using get_t = std::function<R(T*)>;
public:
  static int set(lua_State* L) {
    auto t = StackClass<T*>::get(L, 1);
    auto p = *(set_t*)lua_touserdata(L, lua_upvalueindex(1));
    p(t, detail::get<R>(L, 3));
    return 0;
  }
  static int get(lua_State* L) {
    auto t = StackClass<T*>::get(L, 1);
    auto p = *(get_t*)lua_touserdata(L, lua_upvalueindex(1));
    return detail::push(L, p(t));
  }
};

template <class R>
class TraitsClassPropertyNFunCFn {
  using set_t = std::function<int(State*)>;
  using get_t = std::function<int(State*)>;
public:
  static int set(lua_State* L) {
    lua_rawgeti(L, LUA_REGISTRYINDEX, 4);
    auto state = (State*)lua_touserdata(L, -1);
    lua_pop(L, 1);
    auto p = *(set_t*)lua_touserdata(L, lua_upvalueindex(1));
    auto fnptr = *reinterpret_cast <std::function<int(State*)>*> (lua_touserdata(L, lua_upvalueindex(1)));
    return fnptr(state);
  }
  static int get(lua_State* L) {
    lua_rawgeti(L, LUA_REGISTRYINDEX, 4);
    auto state = (State*)lua_touserdata(L, -1);
    lua_pop(L, 1);
    auto p = *(get_t*)lua_touserdata(L, lua_upvalueindex(1));
    auto fnptr = *reinterpret_cast <std::function<int(State*)>*> (lua_touserdata(L, lua_upvalueindex(1)));
    return fnptr(state);
  }
};

template <class R>
class TraitsClassPropertyNFunLCFn {
  using set_t = std::function<int(lua_State*)>;
  using get_t = std::function<int(lua_State*)>;
public:
  static int set(lua_State* L) {
    auto p = *(set_t*)lua_touserdata(L, lua_upvalueindex(1));
    auto fnptr = *reinterpret_cast <std::function<int(lua_State*)>*> (lua_touserdata(L, lua_upvalueindex(1)));
    return fnptr(L);
  }
  static int get(lua_State* L) {
    auto p = *(get_t*)lua_touserdata(L, lua_upvalueindex(1));
    auto fnptr = *reinterpret_cast <std::function<int(lua_State*)>*> (lua_touserdata(L, lua_upvalueindex(1)));
    return fnptr(L);
  }
};

template <class R, class T>
class TraitsClassPropertyFunLCFn {
  using set_t = std::function<void(T*, lua_State*)>;
  using get_t = std::function<R(T*, lua_State*)>;
public:
  static int set(lua_State* L) {
    auto t = StackClass<T>::get(L, 1);
    auto p = *(set_t*)lua_touserdata(L, lua_upvalueindex(1));
    auto fnptr = reinterpret_cast <std::function<int(T*, lua_State*)>*> (lua_touserdata(L, lua_upvalueindex(1)));
    return fnptr(t, L);
  }
  static int get(lua_State* L) {
    auto t = StackClass<T>::get(L, 1);
    auto p = *(get_t*)lua_touserdata(L, lua_upvalueindex(1));
    auto fnptr = reinterpret_cast <std::function<int(T*, lua_State*)>*> (lua_touserdata(L, lua_upvalueindex(1)));
    return fnptr(t, L);
  }
};

template <class R>
class TraitsClassPropertyNFunC {
  using set_t = std::function<int(State*)>;
  using get_t = std::function<int(State*)>;
public:
  static int set(lua_State* L) {
    lua_rawgeti(L, LUA_REGISTRYINDEX, 4);
    auto state = (State*)lua_touserdata(L, -1);
    lua_pop(L, 1);
    auto p = *(set_t*)lua_touserdata(L, lua_upvalueindex(1));
    auto fnptr = *reinterpret_cast <std::function<int(State*)>*> (lua_touserdata(L, lua_upvalueindex(1)));
    return fnptr(state);
  }
  static int get(lua_State* L) {
    lua_rawgeti(L, LUA_REGISTRYINDEX, 4);
    auto state = (State*)lua_touserdata(L, -1);
    lua_pop(L, 1);
    auto p = *(get_t*)lua_touserdata(L, lua_upvalueindex(1));
    auto fnptr = *reinterpret_cast <std::function<int(State*)>*> (lua_touserdata(L, lua_upvalueindex(1)));
    return fnptr(state);
  }
};

template <class R, class T>
class TraitsClassPropertyFunC {
  using set_t = std::function<int(T*, State*)>;
  using get_t = std::function<int(T*, State*)>;
public:
  static int set(lua_State* L) {
    auto t = StackClass<T>::get(L, 1);
    lua_rawgeti(L, LUA_REGISTRYINDEX, 4);
    auto state = (State*)lua_touserdata(L, -1);
    lua_pop(L, 1);
    auto p = *(set_t*)lua_touserdata(L, lua_upvalueindex(1));
    auto fnptr = reinterpret_cast <std::function<int(T*, State*)>*> (lua_touserdata(L, lua_upvalueindex(1)));
    return fnptr(t, state);
  }
  static int get(lua_State* L) {
    auto t = StackClass<T>::get(L, 1);
    lua_rawgeti(L, LUA_REGISTRYINDEX, 4);
    auto state = (State*)lua_touserdata(L, -1);
    lua_pop(L, 1);
    auto p = *(get_t*)lua_touserdata(L, lua_upvalueindex(1));
    auto fnptr = reinterpret_cast <std::function<int(T*, State*)>*> (lua_touserdata(L, lua_upvalueindex(1)));
    return fnptr(t, state);
  }
};

template <class R>
class TraitsClassPropertyNCFun {
  using set_t = void(*)(R);
  using get_t = R(*)();
public:
  static int set(lua_State* L) {
    lua_rawgeti(L, LUA_REGISTRYINDEX, 4);
    auto state = (State*)lua_touserdata(L, -1);
    lua_pop(L, 1);
    auto p = *(set_t*)lua_touserdata(L, lua_upvalueindex(1));
    auto fnptr = *reinterpret_cast <set_t*> (lua_touserdata(L, lua_upvalueindex(1)));
    return fnptr(state);
  }
  static int get(lua_State* L) {
    lua_rawgeti(L, LUA_REGISTRYINDEX, 4);
    auto state = (State*)lua_touserdata(L, -1);
    lua_pop(L, 1);
    auto p = *(get_t*)lua_touserdata(L, lua_upvalueindex(1));
    auto fnptr = *reinterpret_cast <get_t*> (lua_touserdata(L, lua_upvalueindex(1)));
    return fnptr(state);
  }
};

template <class R, class T>
class TraitsClassPropertyCFun {
  using set_t = void(T::*)(R);
  using get_t = R(T::*)();
public:
  static int set(lua_State* L) {
    auto t = StackClass<T>::get(L, 1);
    lua_rawgeti(L, LUA_REGISTRYINDEX, 4);
    auto state = (State*)lua_touserdata(L, -1);
    lua_pop(L, 1);
    auto p = *(set_t*)lua_touserdata(L, lua_upvalueindex(1));
    auto fnptr = reinterpret_cast <set_t*> (lua_touserdata(L, lua_upvalueindex(1)));
    return fnptr(t, state);
  }
  static int get(lua_State* L) {
    auto t = StackClass<T>::get(L, 1);
    lua_rawgeti(L, LUA_REGISTRYINDEX, 4);
    auto state = (State*)lua_touserdata(L, -1);
    lua_pop(L, 1);
    auto p = *(get_t*)lua_touserdata(L, lua_upvalueindex(1));
    auto fnptr = reinterpret_cast <get_t*> (lua_touserdata(L, lua_upvalueindex(1)));
    return fnptr(t, state);
  }
};
}


template <typename T>
struct identity { typedef T type; };

namespace LuaBinding {
class State;

template< class T >
inline constexpr bool is_state_v = std::is_same<T, State*>::value;

class State {
  lua_State* L;

public:
  State() : L(luaL_newstate()) {
    luaL_openlibs(L);
    lua_pushlightuserdata(L, this);
    luaL_ref(L, LUA_REGISTRYINDEX);
    //printf("ref %d\n", );
  }

  ~State() {
    lua_close(L);
  }

  lua_State* lua_state() {
    return L;
  }

  std::vector<Object> args()
  {
    std::vector<Object> result = {};
    for (auto i = 1; i <= lua_gettop(L); i++)
      result.emplace_back(L, i);
    return result;
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

  template<class T>
  Class<T> addClass(const char* name)
  {
    return Class<T>(L, name);
  }

  template<class T> requires is_pushable<T>
  int push(T&& t)
  {
    return Stack<T>::push(L, t);
  }

  template<class T> requires is_pushable<T>
  int push(T& t)
  {
    return Stack<T>::push(L, t);
  }

  template<class T> requires !is_pushable<T>
  int push(T& t)
  {
    return StackClass<T>::push(L, t);
  }

  template<class T>
  T* alloc()
  {
    auto t = new ((T*)lua_newuserdata(L, sizeof(T))) T();
    lua_rawgeti(L, LUA_REGISTRYINDEX, metatables_index);
    lua_rawgetp(L, -1, helper<T>::key());
    lua_setmetatable(L, -3);
    lua_pop(L, 1);
    return t;
  }

  template <class R, class... Params>
  void fun(R(*func)(Params...))
  {
    lua_pushlightuserdata(L, this);
    lua_pushlightuserdata(L, reinterpret_cast<void*>(func));
    lua_pushcclosure(L, Traits<R, Params...>::f, 2);
  }

  template <class R, class... Params>
  void fun(const char* name, R(* func)(Params...))
  {
    fun(func);
    lua_setglobal(L, name);
  }

  template <class R, class... Params>
  void fun(std::function<R(Params...)> func)
  {
    using Fun = decltype (func);
    lua_pushlightuserdata(L, this);
    new (lua_newuserdatauv(L, sizeof(func), 1)) Fun(func);
    lua_pushcclosure(L, TraitsSTDFunction<R, Params...>::f, 2);
  }

  template <class R, class... Params>
  void fun(const char* name, std::function<R(Params...)> func)
  {
    fun(func);
    lua_setglobal(L, name);
  }

  template <typename F>
  void fun(F& func)
  {
    fun(static_cast<function_type_t<F>>(func));
  }

  template <typename F>
  void fun(const char* name, F&& func)
  {
    fun(func);
    lua_setglobal(L, name);
  }

  template <class R> requires std::is_integral_v<R>
  void cfun(R(*func)(State*))
  {
    lua_pushlightuserdata(L, this);
    lua_pushlightuserdata(L, reinterpret_cast<void*>(func));
    lua_pushcclosure(L, TraitsCFunc::f, 2);
  }

  template <class R> requires std::is_integral_v<R>
  void cfun(const char* name, R(*func)(State*))
  {
    cfun(func);
    lua_setglobal(L, name);
  }

  int pcall(int narg = 0, int nres = 0) {
    lua_getglobal(L, "debug");
    lua_getfield(L, -1, "traceback");
    lua_remove(L, -2);
    int errindex = -narg - 2;
    lua_insert(L, errindex);
    auto status = lua_pcall(L, narg, nres, errindex);
    if (!status) lua_remove(L, errindex);
    return status;
  }

  int exec(const char* code, int argn = 0, int nres = 0)
  {
    if (luaL_loadstring(L, code) || pcall(argn, nres))
    {
      printf("Error %s\n", lua_tostring(L, -1));
    }
    return lua_gettop(L);
  }
};
}