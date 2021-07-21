#pragma once

#include "Object.h"

namespace LuaBinding {
    class StackIter
    {
    public:
        typedef std::forward_iterator_tag iterator_category;
        StackIter(lua_State* L, int index) : L(L), index(index) { }
        StackIter operator++() { index++; return *this; }
        Object operator*() { return Object(L, index); }
        bool operator!=(const StackIter& rhs) { return index != rhs.index; }
    private:
        lua_State* L;
        int index;
    };
}