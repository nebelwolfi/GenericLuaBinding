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

            lua_pushcclosure(L, lua_CIndexFunction, 0);
            lua_setfield(L, -2, "__index");

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
        }
    private:
        static int tostring(lua_State* S)
        {
            char asdf[128];
            if constexpr (sizeof(uint32_t) == 4)
                snprintf(asdf, sizeof(asdf), "%s{%X}", lua_tostring(S, lua_upvalueindex(1)), (uint32_t)topointer(S, 1));
            else
                snprintf(asdf, sizeof(asdf), "%s{%llX}", lua_tostring(S, lua_upvalueindex(1)), (uint32_t)topointer(S, 1));
            lua_pushstring(S, asdf);
            return 1;
        }
        static int lua_CIndexFunction(lua_State* S)
        {
            if (strcmp(lua_tostring(S, 2), "ptr") != 0
             && strcmp(lua_tostring(S, 2), "isValid") != 0
             && luaL_getmetafield(S, 1, "__valid"))
            {
                lua_pushvalue(S, 1);
                if (lua_pcall(S, 1, 1, 0) != LUA_OK)
                    lua_error(S);
                if (!lua_toboolean(S, -1))
                {
                    lua_pushvalue(S, 1);
                    if (luaL_getmetafield(S, 1, "__name")) {

                        void* p;
                        if (lua_type(S, 1) != LUA_TUSERDATA) {
                            lua_rawgeti(S, 1, 0xC0FFEE);
                            p = lua_touserdata(S, -1);
                            lua_pop(S, 1);
                        } else {
                            p = lua_touserdata(S, 1);
                        }

                        char asdf[128];
                        if constexpr (sizeof(uint32_t) == 4)
                            snprintf(asdf, sizeof(asdf), "%s{%X}", lua_tostring(S, -1), (uint32_t)p);
                        else
                            snprintf(asdf, sizeof(asdf), "%s{%llX}", lua_tostring(S, -1), (uint32_t)p);
                        lua_pop(S, 1);

                        luaL_error(S, "Tried to access invalid %s (property '%s')", asdf, lua_tostring(S, 2));
                    } else {
                        lua_pop(S, 1);
                        luaL_error(S, "Tried to access invalid unknown object (property '%s')", lua_tostring(S, 2));
                    }
                    return 0; // ??
                }
                lua_pop(S, 1);
            }
            if (luaL_getmetafield(S, 1, "__pindex"))
            {
                lua_pushvalue(S, 2);
                if (luaL_gettable(S, -2))
                {
                    lua_remove(S, -2);
                    lua_insert(S, 1);
                    if (pcall(S, lua_gettop(S) - 1, LUA_MULTRET) != LUA_OK)
                        lua_error(S);
                    return lua_gettop(S);
                }
                else lua_pop(S, 1);
            }
            if (luaL_getmetafield(S, 1, "__findex"))
            {
                lua_pushvalue(S, 2);
                if (luaL_gettable(S, -2))
                {
                    return 1;
                }
                else lua_pop(S, 1);
            }
            if (luaL_getmetafield(S, 1, "__cindex"))
            {
                lua_insert(S, 1);
                if (pcall(S, lua_gettop(S) - 1, LUA_MULTRET) != LUA_OK)
                    lua_error(S);
                return lua_gettop(S);
            }
            if (lua_istable(S, 1)) {
                lua_rawget(S, 1);
                return 1;
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
                    if (pcall(S, 3, 0) != LUA_OK)
                        lua_error(S);
                    return 0;
                }
                else lua_pop(S, 1);
            }
            if (luaL_getmetafield(S, 1, "__cnewindex"))
            {
                lua_pushvalue(S, 1);
                lua_pushvalue(S, 2);
                lua_pushvalue(S, 3);
                if (pcall(S, 3, 0) != LUA_OK)
                    lua_error(S);
                return 0;
            }
            if (lua_istable(S, 1)) {
                lua_rawset(S, 1);
            } else {
                luaL_error(S, "No writable member %s", lua_tostring(S, 2));
            }
            return 0;
        }
    private:
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
            void* p;
            if (lua_type(L, 1) != LUA_TUSERDATA) {
                lua_rawgeti(L, 1, 0xC0FFEE);
                p = lua_touserdata(L, -1);
                lua_pop(L, 1);
            } else {
                p = lua_touserdata(L, 1);
            }

            lua_pushnumber(L, (double)(uint32_t)*(void**)p);
            return 1;
        }
        static void* topointer(lua_State *L, int idx)
        {
            void* p;
            if (lua_type(L, idx) != LUA_TUSERDATA) {
                lua_rawgeti(L, idx, 0xC0FFEE);
                p = lua_touserdata(L, -1);
                lua_pop(L, 1);
            } else {
                p = lua_touserdata(L, idx);
            }

            return p;
        }
        static int get_ptr(lua_State *L)
        {
            void* p;
            if (lua_type(L, 1) != LUA_TUSERDATA) {
                lua_rawgeti(L, 1, 0xC0FFEE);
                p = lua_touserdata(L, -1);
                lua_pop(L, 1);
            } else {
                p = lua_touserdata(L, 1);
            }

            lua_pushnumber(L, (double)(uint32_t)p);
            return 1;
        }
        static int get_prop(lua_State *L)
        {
            auto offset = lua_tointeger(L, lua_upvalueindex(1));
            auto type = lua_tointeger(L, lua_upvalueindex(2));

            void* p;
            if (lua_type(L, 1) != LUA_TUSERDATA) {
                lua_rawgeti(L, 1, 0xC0FFEE);
                p = lua_touserdata(L, -1);
                lua_pop(L, 1);
            } else {
                p = lua_touserdata(L, 1);
            }

            return lua_pushmemtype(L, MemoryType(type), (void*)((uint32_t)p + offset));
        }
        static int get_prop_struct(lua_State *L)
        {
            auto offset = lua_tointeger(L, lua_upvalueindex(1));

            lua_getfield(L, lua_upvalueindex(2), "__key");
            DynClass* c = static_cast<DynClass *>(lua_touserdata(L, -1));
            lua_pop(L, 1);

            void* p;
            if (lua_type(L, 1) != LUA_TUSERDATA) {
                lua_rawgeti(L, 1, 0xC0FFEE);
                p = lua_touserdata(L, -1);
                lua_pop(L, 1);
            } else {
                p = lua_touserdata(L, 1);
            }

            if (*(void**)((uint32_t)p + offset) == nullptr)
            {
                lua_pushnil(L);
                return 1;
            }

            lua_createtable(L, 1, 0);

            lua_pushlightuserdata(L, *(void**)((uint32_t)p + offset));
            lua_rawseti(L, -2, 0xC0FFEE);

            push_metatable(L, c, true);
            lua_setmetatable(L, -2);

            return 1;
        }
        static int set_prop(lua_State *L)
        {
            auto offset = lua_tointeger(L, lua_upvalueindex(1));
            auto type = lua_tointeger(L, lua_upvalueindex(2));

            void* p;
            if (lua_type(L, 1) != LUA_TUSERDATA) {
                lua_rawgeti(L, 1, 0xC0FFEE);
                p = lua_touserdata(L, -1);
                lua_pop(L, 1);
            } else {
                p = lua_touserdata(L, 1);
            }

            auto addr = (void *) ((uint32_t)p + offset);

            lua_pullmemtype(L, MemoryType(type), addr);
            return 0;
        }
        static int dyn_prop(lua_State *L)
        {
            lua_getfield(L, 1, "__key");
            DynClass* c = static_cast<DynClass *>(lua_touserdata(L, -1));

            if (lua_type(L, 4) == LUA_TTABLE)
            {
                c->push_property_index();
                lua_pushvalue(L, 3);
                lua_pushvalue(L, 4);
                lua_pushcclosure(L, get_prop_struct, 2);
                lua_setfield(L, -2, lua_tolstring(L, 2, nullptr));
                lua_pop(L, 1);

                if (lua_isboolean(L, 5) && lua_toboolean(L, 5) == true) {
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

                if (lua_isboolean(L, 5) && lua_toboolean(L, 5) == true) {
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
            auto udata = lua_newuserdatauv(L, size, 0);
            memset(udata, 0, size);

            push_metatable(L, c, true);
            lua_setmetatable(L, -2);

            return 1;
        }
        static int dyn_read(lua_State *L)
        {
            if (*(void**)lua_tointeger(L, 2) == nullptr)
            {
                lua_pushnil(L);
                return 1;
            }

            lua_getfield(L, 1, "__key");
            DynClass* c = static_cast<DynClass *>(lua_touserdata(L, -1));

            lua_createtable(L, 1, 0);

            lua_pushlightuserdata(L, *(void**)(uint32_t)lua_tonumber(L, 2));

            lua_rawseti(L, -2, 0xC0FFEE);

            push_metatable(L, c, true);
            lua_setmetatable(L, -2);

            return 1;
        }
        static int dyn_cast(lua_State *L)
        {
            if (lua_tointeger(L, 2) == 0)
            {
                lua_pushnil(L);
                return 1;
            }

            lua_getfield(L, 1, "__key");
            DynClass* c = static_cast<DynClass *>(lua_touserdata(L, -1));

            lua_createtable(L, 1, 0);

            lua_pushlightuserdata(L, (void*)(uint32_t)lua_tonumber(L, 2));

            lua_rawseti(L, -2, 0xC0FFEE);

            push_metatable(L, c, true);
            lua_setmetatable(L, -2);

            return 1;
        }
    };
}