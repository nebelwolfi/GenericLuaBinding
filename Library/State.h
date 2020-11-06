#pragma once

#include "lua.hpp"
#include "Object.h"
#include "Traits.h"
#include "Stack.h"
#include "Class.h"
#include <vector>
#include <functional>
#include <cassert>

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