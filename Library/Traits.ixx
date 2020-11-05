export module LuaBinding:Traits;

import "lua.hpp";
export import :Value;
export import :Stack;
export import :Class;
export import :Object;
#include <tuple>
#include <type_traits>

export namespace LuaBinding {
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
				return Stack<R>::push(L, result);
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
			lua_rawgeti(L, LUA_REGISTRYINDEX, 4);
			auto state = (State*)lua_touserdata(L, -1);
			lua_pop(L, 1);
			auto fnptr = *static_cast <R(T::**)(Params...)> (lua_touserdata(L, lua_upvalueindex(1)));
			ParamList params;
			std::get<0>(params) = (T*)lua_touserdata(L, 1);
			assign_tup<1, 1>(L, state, params);
			if constexpr (std::is_void_v<R>) {
				std::apply(fnptr, params);
				return 0;
			}
			else {
				R result = std::apply(fnptr, params);
				return Stack<R>::push(L, result);
			}
		}
	};
	template <class T>
	class TraitsClassCFunc {
	public:
		static int f(lua_State* L) {
			auto t = (T*)lua_touserdata(L, 1);
			lua_rawgeti(L, LUA_REGISTRYINDEX, 4);
			auto state = (State*)lua_touserdata(L, -1);
			lua_pop(L, 1);
			auto fnptr = reinterpret_cast <int(T::*)(State*)> (lua_touserdata(L, lua_upvalueindex(1)));
			return fnptr(t, state);
		}
	};

	template <class R, class T>
	class TraitsClassProperty {
		using prop = R(T::*);
	public:
		static int set(lua_State* L) {
			auto t = (T*)lua_touserdata(L, 1);
			auto p = (prop*)lua_touserdata(L, lua_upvalueindex(1));
			t->** p = Stack<R, false>::get(L, 3);
			return 0;
		}
		static int get(lua_State* L) {
			auto t = (T*)lua_touserdata(L, 1);
			auto p = (prop*)lua_touserdata(L, lua_upvalueindex(1));
			printf("wat");
			return Stack<R, false>::push(L, t->* * p);
		}
	};
}