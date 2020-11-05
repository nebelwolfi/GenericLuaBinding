export module LuaBinding:State;

export import "lua.hpp";
export import :Object;
export import :Value;
export import :Traits;
export import :Stack;
export import :Class;
#include <vector>
#include <cassert>

template<typename T>
concept is_class = requires(T& a) {
	LuaBinding::Stack<T, true>::push(nullptr, &a);
};

template<typename T>
concept is_pushable_v = requires(T a) {
	LuaBinding::Stack<T, false>::push(nullptr, a);
};

export namespace LuaBinding {
	class State;

	template< class T >
	inline constexpr bool is_state_v = std::is_same<T, State*>::value;

	class State {
		lua_State* L;

	public:
		State() : L(luaL_newstate()) {
			luaL_openlibs(L);
			lua_pushlightuserdata(L, this);
			printf("ref %d\n", luaL_ref(L, LUA_REGISTRYINDEX));
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

		Object at(int index)
		{
			if (abs(index) > lua_gettop(L))
				throw std::out_of_range("invalid stack subscript");
			return { L, index };
		}

		template<class T>
		Class<T> addClass(const char* name)
		{
			return Class<T>(L, name);
		}

		template<class T>
		int push(T t)
		{
			return Stack<T, false>::push(L, t);
		}

		template<class T> requires is_class<T>
		int push(T& t)
		{
			return Stack<T, true>::push(L, &t);
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