#pragma once

#include "Class.h"
#include "Object.h"
#include "Stack.h"
#include "lua.hpp"
#include <tuple>
#include <type_traits>

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