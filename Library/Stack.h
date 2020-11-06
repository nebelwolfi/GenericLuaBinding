#pragma once

#include "lua.hpp"
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