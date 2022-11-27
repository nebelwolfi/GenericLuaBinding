#pragma once
namespace LuaBinding {
    enum MemoryType {
        _char = -0x1,
        _int = -0x2,
        _float = -0x4,
        _bool = -0x8,
        _double = -0x10,
        ptr = -0x20,
        vec2 = -0x40,
        vec3 = -0x80,
        _array = -0x200,
        _short = -0x400,
        _byte = -0x800,
        uint = -0x2000,
        ushort = -0x4000,
        string = -0x8000,
        null = -0x10000,
        cstruct = -0x20000,
        vector = -0x40000,
        point = -0x100000,
        udata = -0x200000,

        stdstring = -0x400000,
        stringptr = -0x800000,
    };
    int lua_pushmemtype(lua_State* L, MemoryType type, void* data);
    int lua_pullmemtype(lua_State* L, MemoryType type, void* addr);
    size_t get_type_size(MemoryType mt);
}