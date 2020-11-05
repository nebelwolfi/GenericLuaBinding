export module LuaBinding:Object;
export import "lua.hpp";

export namespace LuaBinding {
	class Object {
		lua_State* L = nullptr;
		int index = LUA_REFNIL;
	public:
		Object() {}
		Object(lua_State* L, int index) : L(L)
		{
			this->index = lua_absindex(L, index);
		}

		const char* tostring()
		{
			return luaL_tolstring(L, index, NULL);
		}
	};
}