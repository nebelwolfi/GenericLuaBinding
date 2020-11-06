#pragma once
#include "lua.hpp"
#include "Stack.h"

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