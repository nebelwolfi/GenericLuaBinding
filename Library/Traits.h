#pragma once

#include <tuple>
#include <type_traits>

namespace LuaBinding {
    class State;

    template <typename T>
    struct is_boolean : std::is_same<bool, T> {};
    template <typename T>
    inline constexpr bool is_boolean_v = is_boolean<T>::value;
    template< class T >
    struct is_void : std::is_same<void, typename std::remove_cv<T>::type> {};

    template<typename T>
    concept is_constructible_from_nullptr = requires() {
        { T(nullptr) };
    };

    template<typename T>
    concept is_default_constructible = requires() {
        { T() };
    };

    template <std::size_t I = 0, int J = 0, typename ... Ts>
    void assign_tup(lua_State* L, State* S, std::tuple<Ts...> &tup) {
        if constexpr (I == sizeof...(Ts)) {
            return;
        } else {
            using Type = typename std::tuple_element<I, std::tuple<Ts...>>::type;

            if constexpr (std::is_same_v<Type, lua_State*>) {
                std::get<I>(tup) = L;
            }
            else if constexpr (std::is_same_v<Type, State*>) {
                std::get<I>(tup) = S;
            }
            else if constexpr (std::is_same_v<Type, Object>) {
                std::get<I>(tup) = Object(L, I + 1 + J);
            }
            else if constexpr (!detail::is_pushable<Type>) {
                if (!StackClass<Type>::is(L, I + 1 + J))
                    if (lua_isnoneornil(L, I + 1 + J))
                        if constexpr (std::is_convertible_v<Type, void*>)
                        {
                            std::get<I>(tup) = (Type)nullptr;
                        }
                        else
                        {
                            luaL_typeerror(L, I + 1, StackClass<Type>::type_name(L));
                        }
                    else
                    {
                        luaL_typeerror(L, I + 1, StackClass<Type>::type_name(L));
                    }
                else
                    std::get<I>(tup) = (Type)StackClass<Type>::get(L, I + 1 + J);
            }
            else if constexpr (detail::is_pushable<Type>) {
                if (!Stack<Type>::is(L, I + 1 + J))
                    if (lua_isnoneornil(L, I + 1 + J))
                        if constexpr (is_constructible_from_nullptr<Type>)
                            std::get<I>(tup) = Type(nullptr);
                        else if constexpr (is_default_constructible<Type>)
                            std::get<I>(tup) = Type();
                        else
                            luaL_typeerror(L, I + 1, Stack<Type>::type_name(L));
                    else
                        luaL_typeerror(L, I + 1, Stack<Type>::type_name(L));
                else
                    std::get<I>(tup) = (Type)Stack<Type>::get(L, I + 1 + J);
            }
            else static_assert(sizeof(Type) > 0, "How");
            assign_tup<I + 1, J>(L, S, tup);
        }
    }

    template <class R, class... Params>
    class Traits {
        using ParamList = std::tuple<std::decay_t<Params>...>;
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
                return detail::push(L, result);
            }
        }
    };

    namespace detail {
        template <class T, class Tuple, std::size_t... I>
        constexpr T* make_from_tuple_impl(void* mem, Tuple&& t, std::index_sequence<I...> )
        {
            return new (mem) T(std::get<I>(std::forward<Tuple>(t))...);
        }
    }

    template <class T, class Tuple>
    constexpr T* make_from_tuple(void* mem, Tuple&& t )
    {
        return detail::make_from_tuple_impl<T>(mem,
                                               std::forward<Tuple>(t),
                                               std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<Tuple>>>{});
    }

    template <class T, class... Params>
    class TraitsCtor {
        using ParamList = std::tuple<std::decay_t<Params>...>;
    public:
        static int f(lua_State* L) {
            auto state = (State*)lua_touserdata(L, lua_upvalueindex(1));
            ParamList params;
            assign_tup<0, 1>(L, state, params);
            make_from_tuple<T>(lua_newuserdata (L, sizeof(T)), params);
            lua_pushvalue(L, 1);
            lua_setmetatable(L, -2);
            return 1;
        }
    };

    template <class R, class... Params>
    class TraitsSTDFunction {
        using ParamList = std::tuple<std::decay_t<Params>...>;
    public:
        static int f(lua_State* L) {
            auto state = (State*)lua_touserdata(L, lua_upvalueindex(1));
            auto fnptr = reinterpret_cast <std::function<R(Params...)>*> (lua_touserdata(L, lua_upvalueindex(2)));
            ParamList params;
            assign_tup(L, state, params);
            if constexpr (std::is_void_v<R>) {
                std::apply(*fnptr, params);
                return 0;
            } else {
                R result = std::apply(*fnptr, params);
                return detail::push(L, result);
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
        using ParamList = std::tuple<T*, std::decay_t<Params>...>;
    public:
        static int f(lua_State* L) {
            lua_rawgeti(L, LUA_REGISTRYINDEX, 4);
            auto state = (State*)lua_touserdata(L, -1);
            lua_pop(L, 1);
            auto fnptr = *static_cast <R(T::**)(Params...)> (lua_touserdata(L, lua_upvalueindex(1)));
            ParamList params;
            assign_tup(L, state, params);
            if constexpr (std::is_void_v<R>) {
                std::apply(fnptr, params);
                return 0;
            } else {
                return detail::push(L, std::apply(fnptr, params));
            }
        }
    };

    template <class R, class T, class... Params>
    class TraitsFunClass {
        using ParamList = std::tuple<std::decay_t<Params>...>;
    public:
        static int f(lua_State* L) {
            lua_rawgeti(L, LUA_REGISTRYINDEX, 4);
            auto state = (State*)lua_touserdata(L, -1);
            lua_pop(L, 1);
            auto fnptr = *static_cast <std::function<R(Params...)>*> (lua_touserdata(L, lua_upvalueindex(1)));
            ParamList params;
            assign_tup(L, state, params);
            if constexpr (std::is_void_v<R>) {
                std::apply(fnptr, params);
                return 0;
            } else {
                R result = std::apply(fnptr, params);
                return detail::push(L, result);
            }
        }
    };

    template <class R, class T, class... Params>
    class TraitsNClass {
        using ParamList = std::tuple<std::decay_t<Params>...>;
    public:
        static int f(lua_State* L) {
            lua_rawgeti(L, LUA_REGISTRYINDEX, 4);
            auto state = (State*)lua_touserdata(L, -1);
            lua_pop(L, 1);
            auto fnptr = *static_cast <R(**)(Params...)> (lua_touserdata(L, lua_upvalueindex(1)));
            ParamList params;
            assign_tup(L, state, params);
            if constexpr (std::is_void_v<R>) {
                std::apply(fnptr, params);
                return 0;
            } else {
                R result = std::apply(fnptr, params);
                return detail::push(L, result);
            }
        }
    };
    template <class T>
    class TraitsClassNCFunc {
    public:
        static int f(lua_State* L) {
            lua_rawgeti(L, LUA_REGISTRYINDEX, 4);
            auto state = (State*)lua_touserdata(L, -1);
            lua_pop(L, 1);
            auto fnptr = reinterpret_cast <int(*)(State*)> (lua_touserdata(L, lua_upvalueindex(1)));
            return fnptr(state);
        }
    };
    template <class T>
    class TraitsClassFunNCFunc {
    public:
        static int f(lua_State* L) {
            lua_rawgeti(L, LUA_REGISTRYINDEX, 4);
            auto state = (State*)lua_touserdata(L, -1);
            lua_pop(L, 1);
            auto fnptr = *reinterpret_cast <std::function<int(State*)>*> (lua_touserdata(L, lua_upvalueindex(1)));
            return fnptr(state);
        }
    };
    template <class T>
    class TraitsClassCFunc {
    public:
        static int f(lua_State* L) {
            auto t = StackClass<T*>::get(L, 1);
            lua_rawgeti(L, LUA_REGISTRYINDEX, 4);
            auto state = (State*)lua_touserdata(L, -1);
            lua_pop(L, 1);
            auto fnptr = reinterpret_cast <int(T::**)(State*)> (lua_touserdata(L, lua_upvalueindex(1)));
            return (t->**fnptr)(state);
        }
    };
    template <class T>
    class TraitsClassFunCFunc {
    public:
        static int f(lua_State* L) {
            auto t = StackClass<T*>::get(L, 1);
            lua_rawgeti(L, LUA_REGISTRYINDEX, 4);
            auto state = (State*)lua_touserdata(L, -1);
            lua_pop(L, 1);
            auto fnptr = reinterpret_cast <std::function<int(T*, State*)>*> (lua_touserdata(L, lua_upvalueindex(1)));
            return fnptr(t, state);
        }
    };
    template <class T>
    class TraitsClassLCFunc {
        using func = int(T::*)(lua_State*);
    public:
        static int f(lua_State* L) {
            auto t = StackClass<T*>::get(L, 1);
            auto fnptr = (func*)lua_touserdata(L, lua_upvalueindex(1));
            return (t->**fnptr)(L);
        }
    };
    template <class T>
    class TraitsClassFunLCFunc {
        using func = int(T::*)(lua_State*);
    public:
        static int f(lua_State* L) {
            auto t = StackClass<T*>::get(L, 1);
            auto fnptr = reinterpret_cast <std::function<int(T*, lua_State*)>*> (lua_touserdata(L, lua_upvalueindex(1)));
            return fnptr(t, L);
        }
    };
    template <class T>
    class TraitsClassFunNLCFunc {
    public:
        static int f(lua_State* L) {
            auto fnptr = *reinterpret_cast <std::function<int(lua_State*)>*> (lua_touserdata(L, lua_upvalueindex(1)));
            return fnptr(L);
        }
    };

    template <class R, class T>
    class TraitsClassProperty {
        using prop = R(T::*);
    public:
        static int set(lua_State* L) {
            auto t = StackClass<T*>::get(L, 1);
            auto p = (prop*)lua_touserdata(L, lua_upvalueindex(1));
            t->**p = detail::get<R>(L, 3);
            return 0;
        }
        static int get(lua_State* L) {
            auto t = StackClass<T*>::get(L, 1);
            auto p = (prop*)lua_touserdata(L, lua_upvalueindex(1));
            return detail::push(L, t->**p);
        }
    };

    template <class R, class T>
    class TraitsClassPropertyNFn {
        using set_t = void(*)(R);
        using get_t = R(*)();
    public:
        static int set(lua_State* L) {
            auto p = *(set_t*)lua_touserdata(L, lua_upvalueindex(1));
            p(detail::get<R>(L, 3));
            return 0;
        }
        static int get(lua_State* L) {
            auto p = *(get_t*)lua_touserdata(L, lua_upvalueindex(1));
            return detail::push(L, p());
        }
    };

    template <class R, class T>
    class TraitsClassPropertyFn {
        using set_t = void(T::*)(R);
        using get_t = R(T::*)();
    public:
        static int set(lua_State* L) {
            auto t = StackClass<T*>::get(L, 1);
            auto p = (set_t*)lua_touserdata(L, lua_upvalueindex(1));
            (t->**p)(detail::get<R>(L, 3));
            return 0;
        }
        static int get(lua_State* L) {
            auto t = StackClass<T*>::get(L, 1);
            auto p = (get_t*)lua_touserdata(L, lua_upvalueindex(1));
            return detail::push(L, (t->**p)());
        }
    };

    template <class R, class T>
    class TraitsClassPropertyNFunFn {
        using set_t = std::function<void(R)>;
        using get_t = std::function<R()>;
    public:
        static int set(lua_State* L) {
            auto p = *(set_t*)lua_touserdata(L, lua_upvalueindex(1));
            p(detail::get<R>(L, 3));
            return 0;
        }
        static int get(lua_State* L) {
            auto p = *(get_t*)lua_touserdata(L, lua_upvalueindex(1));
            return detail::push(L, p());
        }
    };

    template <class R, class T>
    class TraitsClassPropertyFunFn {
        using set_t = std::function<void(T*, R)>;
        using get_t = std::function<R(T*)>;
    public:
        static int set(lua_State* L) {
            auto t = StackClass<T*>::get(L, 1);
            auto p = *(set_t*)lua_touserdata(L, lua_upvalueindex(1));
            p(t, detail::get<R>(L, 3));
            return 0;
        }
        static int get(lua_State* L) {
            auto t = StackClass<T*>::get(L, 1);
            auto p = *(get_t*)lua_touserdata(L, lua_upvalueindex(1));
            detail::push(L, p(t));
            return 1;
        }
    };

    template <class R, class T>
    class TraitsClassPropertyNFunCFn {
        using set_t = std::function<int(State*)>;
        using get_t = std::function<int(State*)>;
    public:
        static int set(lua_State* L) {
            lua_rawgeti(L, LUA_REGISTRYINDEX, 4);
            auto state = (State*)lua_touserdata(L, -1);
            lua_pop(L, 1);
            auto fnptr = *reinterpret_cast <std::function<int(State*)>*> (lua_touserdata(L, lua_upvalueindex(1)));
            return fnptr(state);
        }
        static int get(lua_State* L) {
            lua_rawgeti(L, LUA_REGISTRYINDEX, 4);
            auto state = (State*)lua_touserdata(L, -1);
            lua_pop(L, 1);
            auto fnptr = *reinterpret_cast <std::function<int(State*)>*> (lua_touserdata(L, lua_upvalueindex(1)));
            return fnptr(state);
        }
    };

    template <class R, class T>
    class TraitsClassPropertyNFunLCFn {
        using set_t = std::function<int(lua_State*)>;
        using get_t = std::function<int(lua_State*)>;
    public:
        static int set(lua_State* L) {
            auto p = *(set_t*)lua_touserdata(L, lua_upvalueindex(1));
            auto fnptr = *reinterpret_cast <std::function<int(lua_State*)>*> (lua_touserdata(L, lua_upvalueindex(1)));
            return fnptr(L);
        }
        static int get(lua_State* L) {
            auto p = *(get_t*)lua_touserdata(L, lua_upvalueindex(1));
            auto fnptr = *reinterpret_cast <std::function<int(lua_State*)>*> (lua_touserdata(L, lua_upvalueindex(1)));
            return fnptr(L);
        }
    };

    template <class R, class T>
    class TraitsClassPropertyFunLCFn {
        using set_t = std::function<int(T*, lua_State*)>;
        using get_t = std::function<int(T*, lua_State*)>;
    public:
        static int set(lua_State* L) {
            auto t = StackClass<T*>::get(L, 1);
            auto p = *(set_t*)lua_touserdata(L, lua_upvalueindex(1));
            auto fnptr = reinterpret_cast <std::function<int(T*, lua_State*)>*> (lua_touserdata(L, lua_upvalueindex(1)));
            return fnptr(t, L);
        }
        static int get(lua_State* L) {
            auto t = StackClass<T*>::get(L, 1);
            auto p = *(get_t*)lua_touserdata(L, lua_upvalueindex(1));
            auto fnptr = reinterpret_cast <std::function<int(T*, lua_State*)>*> (lua_touserdata(L, lua_upvalueindex(1)));
            return fnptr(t, L);
        }
    };

    template <class R, class T>
    class TraitsClassPropertyNFunC {
        using set_t = std::function<int(State*)>;
        using get_t = std::function<int(State*)>;
    public:
        static int set(lua_State* L) {
            lua_rawgeti(L, LUA_REGISTRYINDEX, 4);
            auto state = (State*)lua_touserdata(L, -1);
            lua_pop(L, 1);
            auto p = *(set_t*)lua_touserdata(L, lua_upvalueindex(1));
            auto fnptr = *reinterpret_cast <std::function<int(State*)>*> (lua_touserdata(L, lua_upvalueindex(1)));
            return fnptr(state);
        }
        static int get(lua_State* L) {
            lua_rawgeti(L, LUA_REGISTRYINDEX, 4);
            auto state = (State*)lua_touserdata(L, -1);
            lua_pop(L, 1);
            auto p = *(get_t*)lua_touserdata(L, lua_upvalueindex(1));
            auto fnptr = *reinterpret_cast <std::function<int(State*)>*> (lua_touserdata(L, lua_upvalueindex(1)));
            return fnptr(state);
        }
    };

    template <class R, class T>
    class TraitsClassPropertyFunC {
        using set_t = std::function<int(T*, State*)>;
        using get_t = std::function<int(T*, State*)>;
    public:
        static int set(lua_State* L) {
            auto t = StackClass<T*>::get(L, 1);
            lua_rawgeti(L, LUA_REGISTRYINDEX, 4);
            auto state = (State*)lua_touserdata(L, -1);
            lua_pop(L, 1);
            auto fnptr = reinterpret_cast <std::function<int(T*, State*)>*> (lua_touserdata(L, lua_upvalueindex(1)));
            return fnptr(t, state);
        }
        static int get(lua_State* L) {
            auto t = StackClass<T*>::get(L, 1);
            lua_rawgeti(L, LUA_REGISTRYINDEX, 4);
            auto state = (State*)lua_touserdata(L, -1);
            lua_pop(L, 1);
            auto fnptr = reinterpret_cast <std::function<int(T*, State*)>*> (lua_touserdata(L, lua_upvalueindex(1)));
            return fnptr(t, state);
        }
    };

    template <class R, class T>
    class TraitsClassPropertyNCFun {
        using set_t = int(*)(State*);
        using get_t = int(*)(State*);
    public:
        static int set(lua_State* L) {
            lua_rawgeti(L, LUA_REGISTRYINDEX, 4);
            auto state = (State*)lua_touserdata(L, -1);
            lua_pop(L, 1);
            auto fnptr = *reinterpret_cast <set_t*> (lua_touserdata(L, lua_upvalueindex(1)));
            return fnptr(state);
        }
        static int get(lua_State* L) {
            lua_rawgeti(L, LUA_REGISTRYINDEX, 4);
            auto state = (State*)lua_touserdata(L, -1);
            lua_pop(L, 1);
            auto fnptr = *reinterpret_cast <get_t*> (lua_touserdata(L, lua_upvalueindex(1)));
            return fnptr(state);
        }
    };

    template <class R, class T>
    class TraitsClassPropertyCFun {
        using set_t = int(T::*)(State*);
        using get_t = int(T::*)(State*);
    public:
        static int set(lua_State* L) {
            auto t = StackClass<T*>::get(L, 1);
            lua_rawgeti(L, LUA_REGISTRYINDEX, 4);
            auto state = (State*)lua_touserdata(L, -1);
            lua_pop(L, 1);
            auto fnptr = reinterpret_cast <set_t*> (lua_touserdata(L, lua_upvalueindex(1)));
            return fnptr(t, state);
        }
        static int get(lua_State* L) {
            auto t = StackClass<T*>::get(L, 1);
            lua_rawgeti(L, LUA_REGISTRYINDEX, 4);
            auto state = (State*)lua_touserdata(L, -1);
            lua_pop(L, 1);
            auto fnptr = reinterpret_cast <get_t*> (lua_touserdata(L, lua_upvalueindex(1)));
            return fnptr(t, state);
        }
    };
}
