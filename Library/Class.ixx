export module LuaBinding:Class;
export import "lua.hpp";

int pcall(lua_State *L, int narg = 0, int nres = 0) {
	lua_getglobal(L, "debug");
	lua_getfield(L, -1, "traceback");
	lua_remove(L, -2);
	int errindex = -narg - 2;
	lua_insert(L, errindex);
	auto status = lua_pcall(L, narg, nres, errindex);
	if (!status) lua_remove(L, errindex);
	return status;
}

export namespace LuaBinding {
	template<class T>
	class helper {
	public:
		static void const* key() { static char value; return &value; }
	};

	export int metatables_index = LUA_REFNIL;

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
					return lua_gettop(S);
				}
				else lua_pop(S, 1);
			}
			if (luaL_getmetafield(S, 1, "__fnewindex"))
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
		void fun(const char* name, R(T::* func)(Params...))
		{
			using FnType = decltype (func);
			push_function_index();
			new (lua_newuserdatauv(L, sizeof(func), 1)) FnType(func);
			lua_pushcclosure(L, TraitsClass<R, T, Params...>::f, 1);
			lua_setfield(L, -2, name);
			lua_pop(L, 1);
		}

		template <class R> requires std::is_integral_v<R>
		void cfun(const char* name, R(*func)(void*))
		{
			push_function_index();
			lua_pushlightuserdata(L, reinterpret_cast<void*>(func));
			lua_pushcclosure(L, TraitsClassCFunc<T>::f, 1);
			lua_setfield(L, -2, name);
			lua_pop(L, 1);
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
	};
}