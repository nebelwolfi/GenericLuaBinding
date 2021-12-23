#pragma once
#include <functional>
#include <cassert>
#include "MemoryTypes.h"

namespace LuaBinding {

    class DynClass {
        lua_State* L = nullptr;
        const char* class_name;
    public:
        DynClass(lua_State* L, const char* name) : L(L), class_name(name)
        {
            push_metatable(L, this);

            lua_pushcclosure(L, lua_CGCFunction, 0);
            lua_setfield(L, -2, "__gc");

            lua_pushcclosure(L, lua_CIndexFunction, 0);
            lua_setfield(L, -2, "__index");

            lua_pushcclosure(L, lua_EQFunction, 0);
            lua_setfield(L, -2, "__eq");

            lua_pushcclosure(L, lua_CNewIndexFunction, 0);
            lua_setfield(L, -2, "__newindex");

            lua_newtable(L);
            lua_setfield(L, -2, "__pindex");

            lua_newtable(L);
            lua_setfield(L, -2, "__findex");

            lua_pushstring(L, name);
            lua_setfield(L, -2, "__name");

            lua_pushstring(L, name);
            lua_pushcclosure(L, tostring, 1);
            lua_setfield(L, -2, "__tostring");

            lua_pushlightuserdata(L, this);
            lua_setfield(L, -2, "__key");

            lua_pushcfunction(L, dyn_call);
            lua_setfield(L, -2, "New");

            lua_pushcfunction(L, dyn_prop);
            lua_setfield(L, -2, "Prop");

            lua_pushcfunction(L, dyn_read);
            lua_setfield(L, -2, "Read");

            lua_pushcfunction(L, dyn_cast);
            lua_setfield(L, -2, "Cast");

            push_property_index();
            lua_pushcfunction(L, get_ptr);
            lua_setfield(L, -2, "ptr");

            lua_pushcfunction(L, get_vtable);
            lua_setfield(L, -2, "vTable");

            lua_pop(L, 1);

            push_property_newindex();
            lua_pushcfunction(L, set_ptr);
            lua_setfield(L, -2, "ptr");

            lua_pop(L, 1);
        }
    private:
        static int tostring(lua_State* S)
        {
            char asdf[128];
            if constexpr (sizeof(ptrdiff_t) == 4)
                snprintf(asdf, sizeof(asdf), "%s{%X}", lua_tostring(S, lua_upvalueindex(1)), (uint32_t)topointer(S, 1));
            else if constexpr (sizeof(ptrdiff_t) == 8)
                snprintf(asdf, sizeof(asdf), "%s{%llX}", lua_tostring(S, lua_upvalueindex(1)), (uint64_t)topointer(S, 1));
            lua_pushstring(S, asdf);
            return 1;
        }
        static int lua_CGCFunction(lua_State* S)
        {
            if (luaL_getmetafield(S, 1, "__cgc"))
            {
                lua_pushvalue(S, 1);
                pcall(S, 1, 0);
            }
            auto u = (void**)lua_touserdata(S, 1);
            if (*(u + 1) == (void*)0xC0FFEE) {
                free(*u);
            }
            return 0;
        }
        static int lua_EQFunction(lua_State* S)
        {
            if (!lua_isuserdata(S, 1) || !lua_isuserdata(S, 2))
                lua_pushboolean(S, false);
            else
                lua_pushboolean(S, *(uint32_t*)lua_touserdata(S, 1) == *(uint32_t*)lua_touserdata(S, 2));
            return 1;
        }
        static int lua_CIndexFunction(lua_State* S)
        {
            if (strcmp(lua_tostring(S, 2), "ptr") != 0
             && strcmp(lua_tostring(S, 2), "isValid") != 0
             && luaL_getmetafield(S, 1, "__valid"))
            {
                auto success = false;
                lua_pushvalue(S, 1);
                __try {
                    lua_call(S, 1, 1);
                    success = lua_toboolean(S, -1);
                } __except(1) { }
                if (!success)
                {
                    lua_pushvalue(S, 1);
                    if (luaL_getmetafield(S, 1, "__name")) {
                        char asdf[128];
                        if constexpr (sizeof(ptrdiff_t) == 4)
                            snprintf(asdf, sizeof(asdf), "%s{%X}", lua_tostring(S, lua_upvalueindex(1)), (uint32_t)topointer(S, 1));
                        else if constexpr (sizeof(ptrdiff_t) == 8)
                            snprintf(asdf, sizeof(asdf), "%s{%llX}", lua_tostring(S, lua_upvalueindex(1)), (uint64_t)topointer(S, 1));
                        lua_pop(S, 1);

                        luaL_error(S, "Tried to access invalid %s (property '%s')", asdf, lua_tostring(S, 2));
                    } else {
                        lua_pop(S, 1);
                        luaL_error(S, "Tried to access invalid unknown object (property '%s')", lua_tostring(S, 2));
                    }
                    return 0; // ??
                }
                lua_settop(S, 3);
            }
            if (luaL_getmetafield(S, 1, "__pindex"))
            {
                lua_pushvalue(S, 2);
                if (luaL_gettable(S, -2))
                {
                    lua_remove(S, -2);
                    lua_insert(S, 1);
                    lua_call(S, lua_gettop(S) - 1, LUA_MULTRET);
                    return lua_gettop(S);
                }
                else lua_pop(S, 1);
            }
            if (luaL_getmetafield(S, 1, "__findex"))
            {
                lua_pushvalue(S, 2);
                if (luaL_gettable(S, -2))
                    return 1;
                else lua_pop(S, 1);
            }
            if (luaL_getmetafield(S, 1, lua_tostring(S, 2)))
                return 1;
            if (luaL_getmetafield(S, 1, "__cindex"))
            {
                lua_insert(S, 1);
                lua_call(S, lua_gettop(S) - 1, LUA_MULTRET);
                return lua_gettop(S);
            }
            return 0;
        }
        static int lua_CNewIndexFunction(lua_State* S)
        {
            if (luaL_getmetafield(S, 1, "__pnewindex"))
            {
                lua_pushvalue(S, 2);
                if (luaL_gettable(S, -2))
                {
                    lua_pushvalue(S, 1);
                    lua_pushvalue(S, 2);
                    lua_pushvalue(S, 3);
                    lua_call(S, 3, 0);
                    return 0;
                }
                else lua_pop(S, 1);
            }
            if (luaL_getmetafield(S, 1, "__cnewindex"))
            {
                lua_pushvalue(S, 1);
                lua_pushvalue(S, 2);
                lua_pushvalue(S, 3);
                lua_call(S, 3, 0);
                return 0;
            }
            lua_getmetatable(S, 1);
            lua_pushvalue(S, 2);
            lua_pushvalue(S, 3);
            lua_settable(S, -3);
            return 0;
        }
    public:
        static int push_metatable(lua_State *L, void* _this, bool assert = false)
        {
            lua_getglobal(L, "__METASTORE");
            if (lua_rawgetp(L, -1, _this))
            {
                lua_remove(L, -2);
                return 0;
            }
            if (assert)
            {
                luaL_error(L, "metatable not found in __METASTORE for %X", _this);
            }
            lua_pop(L, 1);
            lua_newtable(L);
            lua_pushvalue(L, -1);
            lua_rawsetp(L, -3, _this);
            lua_remove(L, -2);
            return 1;
        }
    private:
        void set_size(unsigned int size)
        {
            push_metatable(L, this);
            lua_pushinteger(L, size);
            lua_setfield(L, -2, "__size");
            lua_pop(L, 1);
        }
        size_t get_size()
        {
            push_metatable(L, this);
            lua_getfield(L, -1, "__size");
            auto result = lua_tointeger(L, -1);
            lua_pop(L, 2);
            return result;
        }
        void push_property_index()
        {
            push_metatable(L, this);
            lua_pushstring(L, "__pindex");
            if (!luaL_gettable(L, -2))
            {
                lua_pop(L, 1);
                lua_newtable(L);
                lua_pushvalue(L, -1);
                lua_setfield(L, -3, "__pindex");
            }
            lua_remove(L, -2);
        }
        void push_property_newindex()
        {
            push_metatable(L, this);
            lua_pushstring(L, "__pnewindex");
            if (!luaL_gettable(L, -2))
            {
                lua_pop(L, 1);
                lua_newtable(L);
                lua_pushvalue(L, -1);
                lua_setfield(L, -3, "__pnewindex");
            }
            lua_remove(L, -2);
        }
        static int get_vtable(lua_State *L)
        {
            lua_pushnumber(L, (double)**(uint32_t**)lua_touserdata(L, 1));
            return 1;
        }
        static void* topointer(lua_State *L, int idx)
        {
            return *(void**)lua_touserdata(L, idx);
        }
        static int get_ptr(lua_State *L)
        {
            lua_pushnumber(L, (double)*(uint32_t*)lua_touserdata(L, 1));
            return 1;
        }
        static int set_ptr(lua_State *L)
        {
            *(uint32_t*)lua_touserdata(L, 1) = (uint32_t)lua_tonumber(L, 3);
            return 0;
        }
        static int get_prop(lua_State *L)
        {
            auto offset = lua_tointeger(L, lua_upvalueindex(1));
            auto type = lua_tointeger(L, lua_upvalueindex(2));

            auto p = *(uint32_t*)lua_touserdata(L, 1);

            return lua_pushmemtype(L, MemoryType(type), (void*)(p + offset));
        }
        static int get_prop_struct(lua_State *L)
        {
            auto offset = lua_tointeger(L, lua_upvalueindex(1));

            lua_getfield(L, lua_upvalueindex(2), "__key");
            DynClass* c = static_cast<DynClass *>(lua_touserdata(L, -1));
            lua_pop(L, 1);

            auto p = *(uint32_t*)lua_touserdata(L, 1);

            if (*(void**)(p + offset) == nullptr)
            {
                lua_pushnil(L);
                return 1;
            }

            auto u = (void**)lua_newuserdata(L, sizeof(void*) * 2);
            *u = *(void**)(p + offset); *(u + 1) = 0;

            push_metatable(L, c, true);
            lua_setmetatable(L, -2);

            return 1;
        }
        static int get_prop_struct_cast(lua_State *L)
        {
            auto offset = lua_tointeger(L, lua_upvalueindex(1));

            lua_getfield(L, lua_upvalueindex(2), "__key");
            DynClass* c = static_cast<DynClass *>(lua_touserdata(L, -1));
            lua_pop(L, 1);

            auto p = *(uint32_t*)lua_touserdata(L, 1);
            auto u = (void**)lua_newuserdata(L, sizeof(void*) * 2);
            *u = (void*)(p + offset); *(u + 1) = 0;

            push_metatable(L, c, true);
            lua_setmetatable(L, -2);

            return 1;
        }
        static int set_prop(lua_State *L)
        {
            auto offset = lua_tointeger(L, lua_upvalueindex(1));
            auto type = lua_tointeger(L, lua_upvalueindex(2));

            auto p = *(uint32_t*)lua_touserdata(L, 1);

            lua_pullmemtype(L, MemoryType(type), (void*)(p + offset));
            return 0;
        }
        static int dyn_prop(lua_State *L)
        {
            lua_getfield(L, 1, "__key");
            DynClass* c = static_cast<DynClass *>(lua_touserdata(L, -1));
            lua_pop(L, 1);

            if (lua_isnoneornil(L, 3))
            {
                lua_getglobal(L, "print");
                static char buff[100] = { '\0' };
                snprintf(buff, 100, "[Warning] %s.%s offset is nil, assuming '0'", c->class_name, lua_tolstring(L, 2, nullptr));
                lua_pushstring(L, buff);
                lua_call(L, 1, 0);
            }

            auto flag = lua_tointeger(L, 5);

            if (lua_type(L, 4) == LUA_TTABLE)
            {
                c->push_property_index();
                lua_pushvalue(L, 3);
                lua_pushvalue(L, 4);
                if (flag == 2)
                    lua_pushcclosure(L, get_prop_struct_cast, 2);
                else
                    lua_pushcclosure(L, get_prop_struct, 2);
                lua_setfield(L, -2, lua_tolstring(L, 2, nullptr));
                lua_pop(L, 1);

                if (!flag) {
                    c->push_property_newindex();
                    lua_pushvalue(L, 3);
                    lua_pushinteger(L, -0x20 /**type.ptr**/);
                    lua_pushcclosure(L, set_prop, 2);
                    lua_setfield(L, -2, lua_tolstring(L, 2, nullptr));
                    lua_pop(L, 1);
                }
            } else {
                c->push_property_index();
                lua_pushvalue(L, 3);
                lua_pushvalue(L, 4);
                lua_pushcclosure(L, get_prop, 2);
                lua_setfield(L, -2, lua_tolstring(L, 2, nullptr));
                lua_pop(L, 1);

                if (!flag) {
                    c->push_property_newindex();
                    lua_pushvalue(L, 3);
                    lua_pushvalue(L, 4);
                    lua_pushcclosure(L, set_prop, 2);
                    lua_setfield(L, -2, lua_tolstring(L, 2, nullptr));
                    lua_pop(L, 1);
                }

                c->set_size(lua_tointeger(L, 3) + get_type_size((MemoryType)lua_tointeger(L, 4)));
            }

            return 0;
        }
        static int dyn_call(lua_State *L)
        {
            lua_getfield(L, 1, "__key");
            DynClass* c = static_cast<DynClass *>(lua_touserdata(L, -1));

            auto size = lua_isinteger(L, 2) ? lua_tointeger(L, 2) : c->get_size();

            auto udata = (void*)malloc(size);
            memset(udata, 0, size);
            auto u = (void**)lua_newuserdata(L, sizeof(void*) * 2);
            *u = udata; *(u + 1) = (void*)0xC0FFEE;

            push_metatable(L, c, true);
            lua_setmetatable(L, -2);

            return 1;
        }
        static int dyn_read(lua_State *L)
        {
            auto p = (void**)(uint32_t)lua_tonumber(L, 2);
            if (p == nullptr || *p == nullptr)
            {
                lua_pushnil(L);
                return 1;
            }

            lua_getfield(L, 1, "__key");
            DynClass* c = static_cast<DynClass *>(lua_touserdata(L, -1));

            auto u = (void**)lua_newuserdata(L, sizeof(void*) * 2);
            *u = *p; *(u + 1) = 0;

            push_metatable(L, c, true);
            lua_setmetatable(L, -2);

            return 1;
        }
        static int dyn_cast(lua_State *L)
        {
            auto p = (void*)(uint32_t)lua_tonumber(L, 2);
            if (p == 0)
            {
                lua_pushnil(L);
                return 1;
            }

            lua_getfield(L, 1, "__key");
            DynClass* c = static_cast<DynClass *>(lua_touserdata(L, -1));

            auto u = (void**)lua_newuserdata(L, sizeof(void*) * 2);
            *u = p; *(u + 1) = 0;

            push_metatable(L, c, true);
            lua_setmetatable(L, -2);

            return 1;
        }
    };
}