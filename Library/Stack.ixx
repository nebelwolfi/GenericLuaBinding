export module LuaBinding:Stack;

import "lua.hpp";
export import :Class;

export namespace LuaBinding {

	template<typename T, bool isClass = false>
	class Stack {
	public:
		static int push(lua_State* L, T t) {
			static_assert(sizeof(T) == 0, "Unspecialized use of Stack::push");
		}
		static bool is(lua_State* L, int index) {
			static_assert(sizeof(T) == 0, "Unspecialized use of Stack::get<T>");
		}
		static T get(lua_State* L, int index) {
			static_assert(sizeof(T) == 0, "Unspecialized use of Stack::get<T>");
		}
	};

	template<typename T>
	class Stack<T, true> {
	public:
		static int push(lua_State* L, T* t)
		{
			lua_pushlightuserdata(L, t);
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
			return (T)lua_newuserdata(L, index);
		}
	};

	template<>
	class Stack<int, false> {
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
	class Stack<T, false> {
	public:
		static T push(lua_State* L, T t)
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
	class Stack<T, false> {
	public:
		static T push(lua_State* L, T t)
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
	class Stack<bool, false> {
	public:
		static bool push(lua_State* L, bool t)
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
}