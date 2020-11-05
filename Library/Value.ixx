export module LuaBinding:Value;
export import "lua.hpp";

export namespace LuaBinding {
    typedef union Value {
        struct GCObject* gc;    /* collectable objects */
        void* p;         /* light userdata */
        lua_CFunction f; /* light C functions */
        lua_Integer i;   /* integer numbers */
        lua_Number n;    /* float numbers */
    } Value;

    template <class T> requires std::is_integral
    Value to_value(T& t) {
        Value v{};
        v.i = t;
        return v;
    }

    template <class T> requires std::is_floating_point
    Value to_value(T& t) {
        Value v{};
        v.n = t;
        return v;
    }

    template <class T> requires std::is_function
    Value to_value(T& t) {
        Value v{};
        v.f = t;
        return v;
    }

    template <class T> requires std::is_pointer
    Value to_value(T& t) {
        Value v{};
        v.p = t;
        return v;
    }
}