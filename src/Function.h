#pragma once
#include "Stack.h"

#include <functional>
#include <tuple>

namespace LuaBinding {
    namespace invocable_traits
    {
        enum class Error
        {
            None,
            NotAClass,
            NoCallOperator,
            IsOverloadedTemplated,
            OverloadNotResolved,
            Unknown
        };

        template <Error E>
        void issue_error()
        {
            static_assert(E != Error::NotAClass,
                "passed type is not a class, and thus cannot have an operator()");
            static_assert(E != Error::NoCallOperator,
                "passed type is a class that doesn't have an operator()");
            static_assert(E != Error::IsOverloadedTemplated,
                "passed passed type is a class that has an overloaded or templated operator(), specify argument types in invocable_traits invocation to disambiguate the operator() signature you wish to use");
            static_assert(E != Error::OverloadNotResolved,
                "passed type is a class that doesn't have an operator() that declares the specified argument types, or some const/ref-qualified combination of the specified argument types");
            static_assert(E != Error::Unknown,
                "an unknown error occurred");
        };

        namespace detail
        {
            template <bool, std::size_t i, typename... Args>
            struct invocable_traits_arg_impl
            {
                using type = std::tuple_element_t<i, std::tuple<Args...>>;
            };
            template <std::size_t i, typename... Args>
            struct invocable_traits_arg_impl<false, i, Args...>
            {
                static_assert(i < sizeof...(Args), "Argument index out of bounds (queried callable does not declare this many arguments)");

                // to reduce excessive compiler error output
                using type = void;
            };

            template <
                typename Rd, typename Ri, typename C,
                bool IsConst, bool isVolatile, bool isNoexcept, bool IsVariadic,
                typename... Args>
            struct invocable_traits_class
            {
                static constexpr std::size_t arity = sizeof...(Args);
                static constexpr auto is_const    = IsConst;
                static constexpr auto is_volatile = isVolatile;
                static constexpr auto is_noexcept = isNoexcept;
                static constexpr auto is_variadic = IsVariadic;

                using declared_result_t = Rd;   // return type as declared in function
                using invoke_result_t   = Ri;   // return type of std::invoke() expression
                using class_t           = C;

                template <std::size_t i>
                using arg_t = typename invocable_traits_arg_impl<i < sizeof...(Args), i, Args...>::type;
                using function_type = std::function<Rd(Args...)>;

                static constexpr Error error      = Error::None;
            };

            template <
                typename Rd, typename Ri,
                bool IsConst, bool isVolatile, bool isNoexcept, bool IsVariadic,
                typename... Args>
            struct invocable_traits_free : public invocable_traits_class<Rd, Ri, void, IsConst, isVolatile, isNoexcept, IsVariadic, Args...> {};


            // machinery to extract exact function signature and qualifications
            template <typename, typename...>
            struct invocable_traits_impl;

            // pointers to data members
            template <typename C, typename R>
            struct invocable_traits_impl<R C::*>
                : public invocable_traits_class<R,
                                                std::invoke_result_t<R C::*, C>,
                                                C,
                                                false, false, false, false
                                            > {};

            // pointers to functions
            template <typename R, typename... Args>
            struct invocable_traits_impl<R(*)(Args...)>                 : public invocable_traits_impl<R(Args...)> {};
            template <typename R, typename... Args>
            struct invocable_traits_impl<R(*)(Args...) noexcept>        : public invocable_traits_impl<R(Args...) noexcept> {};
            template <typename R, typename... Args>
            struct invocable_traits_impl<R(*)(Args..., ...)>            : public invocable_traits_impl<R(Args..., ...)> {};
            template <typename R, typename... Args>
            struct invocable_traits_impl<R(*)(Args..., ...) noexcept>   : public invocable_traits_impl<R(Args..., ...) noexcept> {};

            template <typename...>
            struct typelist {};

        #   define IS_NONEMPTY(...) 0 __VA_OPT__(+1)
        #   define MAKE_CONST(...)    __VA_OPT__(const)
        #   define MAKE_VOLATILE(...) __VA_OPT__(volatile)
        #   define MAKE_NOEXCEPT(...) __VA_OPT__(noexcept)
        #   define MAKE_VARIADIC(...) __VA_OPT__(, ...)

            // functions, pointers to member functions and machinery to select a specific overloaded operator()
        #   define INVOCABLE_TRAITS_SPEC(c,vo,e,va)                                             \
            /* functions */                                                                     \
            template <typename R, typename... Args>                                             \
            struct invocable_traits_impl<R(Args... MAKE_VARIADIC(va))                           \
                                        MAKE_CONST(c) MAKE_VOLATILE(vo) MAKE_NOEXCEPT(e)>      \
                : public invocable_traits_free<                                                 \
                    R,                                                                          \
                    std::invoke_result_t<R(Args... MAKE_VARIADIC(va))                           \
                                        MAKE_CONST(c) MAKE_VOLATILE(vo) MAKE_NOEXCEPT(e), Args...>,    \
                    IS_NONEMPTY(c),                                                             \
                    IS_NONEMPTY(vo),                                                            \
                    IS_NONEMPTY(e),                                                             \
                    IS_NONEMPTY(va),                                                            \
                    Args...> {};                                                                \
            /* pointers to member functions */                                                  \
            template <typename C, typename R, typename... Args>                                 \
            struct invocable_traits_impl<R(C::*)(Args... MAKE_VARIADIC(va))                     \
                                        MAKE_CONST(c) MAKE_VOLATILE(vo) MAKE_NOEXCEPT(e)>      \
                : public invocable_traits_class<                                                \
                    R,                                                                          \
                    std::invoke_result_t<R(C::*)(Args...MAKE_VARIADIC(va))                      \
                                        MAKE_CONST(c) MAKE_VOLATILE(vo) MAKE_NOEXCEPT(e), C, Args...>, \
                    C,                                                                          \
                    IS_NONEMPTY(c),                                                             \
                    IS_NONEMPTY(vo),                                                            \
                    IS_NONEMPTY(e),                                                             \
                    IS_NONEMPTY(va),                                                            \
                    Args...> {};                                                                \
            /* machinery to select a specific overloaded operator() */                          \
            template <typename C, typename... OverloadArgs>                                     \
            auto invocable_traits_resolve_overload(std::invoke_result_t<C, OverloadArgs...>         \
                                                (C::*func)(OverloadArgs... MAKE_VARIADIC(va))    \
                                                MAKE_CONST(c) MAKE_VOLATILE(vo) MAKE_NOEXCEPT(e),\
                                                typelist<OverloadArgs...>)\
            { return func; };

            // cover all const, volatile and noexcept permutations
            INVOCABLE_TRAITS_SPEC( ,  , ,  )
            INVOCABLE_TRAITS_SPEC( ,Va, ,  )
            INVOCABLE_TRAITS_SPEC( ,  ,E,  )
            INVOCABLE_TRAITS_SPEC( ,Va,E,  )
            INVOCABLE_TRAITS_SPEC( ,  , ,Vo)
            INVOCABLE_TRAITS_SPEC( ,Va, ,Vo)
            INVOCABLE_TRAITS_SPEC( ,  ,E,Vo)
            INVOCABLE_TRAITS_SPEC( ,Va,E,Vo)
            INVOCABLE_TRAITS_SPEC(C,  , ,  )
            INVOCABLE_TRAITS_SPEC(C,Va, ,  )
            INVOCABLE_TRAITS_SPEC(C,  ,E,  )
            INVOCABLE_TRAITS_SPEC(C,Va,E,  )
            INVOCABLE_TRAITS_SPEC(C,  , ,Vo)
            INVOCABLE_TRAITS_SPEC(C,Va, ,Vo)
            INVOCABLE_TRAITS_SPEC(C,  ,E,Vo)
            INVOCABLE_TRAITS_SPEC(C,Va,E,Vo)
            // clean up
        #   undef INVOCABLE_TRAITS_SPEC
        #   undef IS_NONEMPTY
        #   undef MAKE_CONST
        #   undef MAKE_VOLATILE
        #   undef MAKE_NOEXCEPT
        #   undef MAKE_VARIADIC

            // check if passed type has an operator(), can be true for struct/class, includes lambdas
            // from https://stackoverflow.com/a/70699109/3103767
            // logic: test if &Tester<C>::operator() is a valid expression. It is only valid if
            // C did not have an operator(), because else &Tester<C>::operator() would be ambiguous
            // and thus invalid. To test if C has an operator(), we just check that
            // &Tester<C>::operator() fails, which implies that C has an operator() declared.
            // This trick does not work for final classes, at least detect non-overloaded
            // operator() for those.
            struct Fake { void operator()(); };
            template <typename T> struct Tester : T, Fake { };

            template <typename C>
            concept HasCallOperator = std::is_class_v<C> and (
                requires(C)                 // checks if non-overloaded operator() exists
                {
                    &C::operator();
                } or
                not requires(Tester<C>)     // checks overloaded/templated operator(), but doesn't work on final classes
                {
                    &Tester<C>::operator();
                }
            );
            // check if we can get operator().
            // If it fails (and assuming above HasCallOperator does pass),
            // this is because the operator is overloaded or templated
            // NB: can't simply do requires(C t){ &C::operator(); } because
            // that is too lenient on clang: it also succeeds for
            // overloaded/templated operator() where it shouldn't
            template <typename T>
            concept CanGetCallOperator = requires
            {
                invocable_traits_impl<
                    decltype(
                        &T::operator()
                    )>();
            };
            // check if we can get an operator() that takes the specified arguments types.
            // If it fails (and assuming above HasCallOperator does pass),
            // an operator() with this specified cv- and ref-qualified argument types does
            // not exist.
            template <typename C, typename... OverloadArgs>
            concept HasSpecificCallOperator = requires
            {
                invocable_traits_resolve_overload<C>(
                    &C::operator(),
                    typelist<OverloadArgs...>{}
                );
            };
            namespace try_harder
            {
                // cartesian product of type lists, from https://stackoverflow.com/a/59422700/3103767
                template <typename... Ts>
                typelist<typelist<Ts>...> layered(typelist<Ts...>);

                template <typename... Ts, typename... Us>
                auto operator+(typelist<Ts...>, typelist<Us...>)
                    ->typelist<Ts..., Us...>;

                template <typename T, typename... Us>
                auto operator*(typelist<T>, typelist<Us...>)
                    ->typelist<decltype(T{} + Us{})...>;

                template <typename... Ts, typename TL>
                auto operator^(typelist<Ts...>, TL tl)
                    -> decltype(((typelist<Ts>{} *tl) + ...));

                template <typename... TLs>
                using product_t = decltype((layered(TLs{}) ^ ...));

                // adapter to make cartesian product of a list of typelists
                template <typename... Ts>
                auto list_product(typelist<Ts...>)
                    ->product_t<Ts...>;

                template <typename T>
                using list_product_t = decltype(list_product(T{}));

                // code to turn input argument type T into all possible const/ref-qualified versions
                template <typename T> struct type_maker_impl;
                // T* -> T*, T* const
                template <typename T>
                    requires std::is_pointer_v<T>
                struct type_maker_impl<T>
                {
                    using type = typelist<T, const T>;
                };
                // T -> T, T&, const T&, T&&, const T&& (NB: const on const T is ignored, so that type is not included)
                template <typename T>
                    requires (!std::is_pointer_v<T>)
                struct type_maker_impl<T>
                {
                    using type = typelist<T, T&, const T&, T&&, const T&&>;
                };

                template <typename T>
                struct type_maker : type_maker_impl<std::remove_cvref_t<T>> {};

                template <typename T>
                using type_maker_t = typename type_maker<T>::type;

                template <typename ...Ts>
                struct type_maker_for_typelist
                {
                    using type = typelist<type_maker_t<Ts>...>;
                };

                // code to filter out combinations of qualified input arguments that do not
                // resolve to a declared overload
                template <typename, typename> struct concat;
                template <typename T, typename ...Args>
                struct concat<T, typelist<Args...>>
                {
                    using type = typelist<T, Args...>;
                };

                template <typename...> struct check;
                template <typename C, typename... Args>
                struct check<C, typelist<Args...>>
                {
                    static constexpr bool value = HasSpecificCallOperator<C, Args...>;
                };

                template <typename...> struct filter;
                template <typename C>
                struct filter<C, typelist<>>
                {
                    using type = typelist<>;
                };
                template <typename C, typename Head, typename ...Tail>
                struct filter<C, typelist<Head, Tail...>>
                {
                    using type = std::conditional_t<check<C, Head>::value,
                        typename concat<Head, typename filter<C, typelist<Tail...>>::type>::type,
                        typename filter<C, typelist<Tail...>>::type
                    >;
                };

                // extract first element from a typelist
                template <typename, typename...> struct get_head;
                template <typename Head, typename ...Tail>
                struct get_head<typelist<Head, Tail...>>
                {
                    using type = Head;
                };
            }

            // to reduce excessive compiler error output
            struct invocable_traits_error
            {
                static constexpr std::size_t arity       = 0;
                static constexpr auto        is_const    = false;
                static constexpr auto        is_volatile = false;
                static constexpr auto        is_noexcept = false;
                static constexpr auto        is_variadic = false;
                using declared_result_t                  = void;
                using invoke_result_t                    = void;
                using class_t                            = void;
                template <size_t i>
                using arg_t                              = void;
            };
            struct invocable_traits_error_overload
            {
                static constexpr std::size_t num_matched_overloads = 0;
                static constexpr auto        is_exact_match        = false;
                template <size_t i>
                using matched_overload                             = invocable_traits_error;
            };

            template <typename T, bool hasOverloadArgs>
            constexpr Error get_error()
            {
                if constexpr (!std::is_class_v<T>)
                    return Error::NotAClass;
                else if constexpr (!HasCallOperator<T>)
                    return Error::NoCallOperator;
                else if constexpr (hasOverloadArgs)
                    return Error::OverloadNotResolved;
                else if constexpr (!hasOverloadArgs && !CanGetCallOperator<T>)
                    return Error::IsOverloadedTemplated;

                return Error::Unknown;
            }

            template <typename C, typename Head>
            struct get_overload_info
            {
                using type =
                    invocable_traits_impl<
                        std::decay_t<
                            decltype(
                                invocable_traits_resolve_overload<C>(
                                    &C::operator(),
                                    Head{}
                                )
                            )
                        >
                    > ;
            };
            template <typename C, typename Head>
            using get_overload_info_t = typename get_overload_info<C, Head>::type;

            template <bool, std::size_t i, typename C, typename... Args>
            struct invocable_traits_overload_info_impl
            {
                using type = get_overload_info_t<C, std::tuple_element_t<i, std::tuple<Args...>>>;
            };
            template <std::size_t i, typename C, typename... Args>
            struct invocable_traits_overload_info_impl<false, i, C, Args...>
            {
                static_assert(i < sizeof...(Args), "Argument index out of bounds (queried callable does not have this many matching overloads)");

                // to reduce excessive compiler error output
                using type = void;
            };

            template <typename C, bool B, typename... Args> struct invocable_traits_overload_info;
            template <typename C, bool B, typename... Args>
            struct invocable_traits_overload_info<C, B, typelist<Args...>>
            {
                static constexpr std::size_t num_matched_overloads  = sizeof...(Args);
                static constexpr auto        is_exact_match         = B;

                template <std::size_t i>
                using matched_overload = typename invocable_traits_overload_info_impl<
                    i < sizeof...(Args),
                    i,
                    C,
                    Args...
                >::type;
            };

            // found at least one overload taking a const/ref qualified version of the specified argument types
            // that is different from those provided by the library user
            template <typename C, typename List>
            struct invocable_traits_extract_try_harder :
                invocable_traits_impl<              // instantiate for the first matched overload
                    std::decay_t<
                        decltype(
                            invocable_traits_resolve_overload<C>(
                                &C::operator(),
                                typename try_harder::get_head<List>::type{}
                            )
                        )
                    >
                > ,
                invocable_traits_overload_info<     // but expose all matched overloads
                    C,
                    false,
                    List
                >
            {};

            // failed to find any overload taking the specified argument types or some const/ref qualified version of them
            template <typename T>
            struct invocable_traits_extract_try_harder<T, typelist<>> : // empty list -> no combination of arguments matched an overload
                invocable_traits_error,
                invocable_traits_error_overload
            {
                static constexpr Error error = get_error<T, true>();
            };

            // specific overloaded operator() is available, use it for analysis
            template <typename C, bool, typename... OverloadArgs>
            struct invocable_traits_extract :
                invocable_traits_impl<
                    std::decay_t<
                        decltype(
                            invocable_traits_resolve_overload<C>(
                                &C::operator(),
                                typelist<OverloadArgs...>{}
                            )
                        )
                    >
                >,
                invocable_traits_overload_info<
                    C,
                    true,
                    typelist<typelist<OverloadArgs...>> // expose matched overload through this interface also, for consistency, even though matching procedure was not run
                >
            {};

            // unambiguous operator() is available, use it for analysis
            template <typename C, bool B>
            struct invocable_traits_extract<C, B> :
                invocable_traits_impl<
                    decltype(
                        &C::operator()
                    )
                > {};

            // no specific overloaded operator() taking the specified arguments is available, try harder
            // to see if one can be found that takes some other const/reference qualified version of the
            // input arguments.
            template <typename T, typename... OverloadArgs>
            struct invocable_traits_extract<T, false, OverloadArgs...> :
                invocable_traits_extract_try_harder<
                    T,
                    typename try_harder::filter<T,                          // filter list of all argument combinations: leave only resolvable overloads
                        try_harder::list_product_t<                         // cartesian product of these lists
                            typename try_harder::type_maker_for_typelist<   // produce list with all const/ref combinations of each argument
                                OverloadArgs...
                            >::type
                        >
                    >::type
                > {};

            template <typename T>
            struct invocable_traits_extract<T, false> : invocable_traits_error
            {
                static constexpr Error error = get_error<T, false>();
            };

            // catch all that doesn't match the various function signatures above
            // If T has an operator(), we go with that. Else, issue error message.
            template <typename T>
            struct invocable_traits_impl<T> :
                invocable_traits_extract<
                    T,
                    HasCallOperator<T>&& CanGetCallOperator<T>
                > {};

            // if overload argument types are provided and needed, use them
            template <typename T, bool B, typename... OverloadArgs>
            struct invocable_traits_overload_impl :
                invocable_traits_extract<
                    T, 
                    HasCallOperator<T> && HasSpecificCallOperator<T, OverloadArgs...>,
                    OverloadArgs...
                > {};

            // if they are provided but not needed, ignore them
            template <typename T, typename... OverloadArgs>
            struct invocable_traits_overload_impl<T, false, OverloadArgs...> :
                invocable_traits_impl<
                    T
                > {};
        }

        template <typename T, typename... OverloadArgs>
        struct get :
            detail::invocable_traits_overload_impl<
                std::remove_reference_t<T>,
                detail::HasCallOperator<std::decay_t<T>> && !detail::CanGetCallOperator<std::decay_t<T>>,
                OverloadArgs...
            > {};

        template <typename T, typename... OverloadArgs>
        struct get<std::reference_wrapper<T>, OverloadArgs...> :
            detail::invocable_traits_overload_impl<
                std::remove_reference_t<T>,
                detail::HasCallOperator<std::decay_t<T>> && !detail::CanGetCallOperator<std::decay_t<T>>,
                OverloadArgs...
            > {};

        template <typename T>
        struct get<T> :
            detail::invocable_traits_impl<
                std::decay_t<T>
            > {};

        template <typename T>
        struct get<std::reference_wrapper<T>> :
            detail::invocable_traits_impl<
                std::decay_t<T>
            > {};
    }

    template <typename F>
    using function_type_t = invocable_traits::get<F>::function_type;

    namespace Function {
        template <class R, class... Params>
        void fun(lua_State *L, R(* func)(Params...))
        {
            using FnType = decltype (func);
            new (lua_newuserdata(L, sizeof(func))) FnType(func);
            lua_pushcclosure(L, TraitsNClass<R, Params...>::f, 1);
        }

        template <class T, class R, class... Params>
        void fun(lua_State *L, R(T::* func)(Params...))
        {
            using FnType = decltype (func);
            new (lua_newuserdata(L, sizeof(func))) FnType(func);
            lua_pushcclosure(L, TraitsClass<R, T, Params...>::f, 1);
        }

        template <class T, class R, class... Params>
        void fun(lua_State *L, R(T::* func)(Params...) const)
        {
            using FnType = std::decay_t<decltype (func)>;
            new (lua_newuserdata(L, sizeof(func))) FnType(func);
            lua_pushcclosure(L, TraitsClass<R, T, Params...>::f, 1);
        }

        template <class T, class R, class... Params>
        void fun(lua_State *L, std::function<R(Params...)> func)
        {
            using FnType = decltype (func);
            new (lua_newuserdata(L, sizeof(func))) FnType(func);
            lua_pushcclosure(L, TraitsFunClass<R, T, Params...>::f, 1);
        }

        template <class T, class R, class... Params>
        void fun(lua_State *L, std::function<R(Params...) const> func)
        {
            using FnType = std::decay_t<decltype (func)>;
            new (lua_newuserdata(L, sizeof(func))) FnType(func);
            lua_pushcclosure(L, TraitsFunClass<R, T, Params...>::f, 1);
        }

        template <class T, class R> requires std::is_integral_v<R>
        void cfun(lua_State *L, R(T::*func)(lua_State*))
        {
            using func_t = decltype (func);
            new (lua_newuserdata(L, sizeof(func))) func_t(func);
            lua_pushcclosure(L, TraitsClassLCFunc<T>::f, 1);
        }

        template <class T, class R> requires std::is_integral_v<R>
        void cfun(lua_State *L, std::function<R(T*, lua_State*)> func)
        {
            using FnType = decltype (func);
            new (lua_newuserdata(L, sizeof(func))) FnType(func);
            lua_pushcclosure(L, TraitsClassFunLCFunc<T>::f, 1);
        }

        template <class T, class R> requires std::is_integral_v<R>
        void cfun(lua_State *L, R(*func)(lua_State*))
        {
            lua_pushcclosure(L, func, 0);
        }

        template <class T, class R> requires std::is_integral_v<R>
        void cfun(lua_State *L, std::function<R(lua_State*)> func)
        {
            using FnType = decltype (func);
            new (lua_newuserdata(L, sizeof(func))) FnType(func);
            lua_pushcclosure(L, TraitsClassFunNLCFunc<T>::f, 1);
        }

        template <class T, class R, class U> requires std::is_integral_v<R>
        void cfun(lua_State *L, R(T::*func)(U))
        {
            using func_t = decltype (func);
            new (lua_newuserdata(L, sizeof(func))) func_t(func);
            lua_pushcclosure(L, TraitsClassCFunc<T>::f, 1);
        }

        template <class T, class R, class U> requires std::is_integral_v<R>
        void cfun(lua_State *L, R(*func)(T*, U))
        {
            using func_t = decltype (func);
            new (lua_newuserdata(L, sizeof(func))) func_t(func);
            lua_pushcclosure(L, TraitsClassCFunc<T>::f, 1);
        }

        template <class T, class R, class U> requires std::is_integral_v<R>
        void cfun(lua_State *L, R(*func)(U))
        {
            lua_pushlightuserdata(L, reinterpret_cast<void*>(func));
            lua_pushcclosure(L, TraitsCFunc<T>::f, 1);
        }

        template <class T, class R, class U> requires std::is_integral_v<R>
        void cfun(lua_State *L, std::function<R(U)> func)
        {
            using FnType = decltype (func);
            new (lua_newuserdata(L, sizeof(func))) FnType(func);
            lua_pushcclosure(L, TraitsClassFunNCFunc<T>::f, 1);
        }

        template <class T, class R, class U> requires std::is_integral_v<R>
        void cfun(lua_State *L, std::function<R(T*, U)> func)
        {
            using FnType = decltype (func);
            new (lua_newuserdata(L, sizeof(func))) FnType(func);
            lua_pushcclosure(L, TraitsClassFunCFunc<T>::f, 1);
        }
        
        template <class T, class F>
        void fun(lua_State *L, F& func)
        {
            fun<T>(L, static_cast<function_type_t<std::decay_t<F>>>(func));
        }

        template <class T, class F>
        void cfun(lua_State *L, F& func)
        {
            cfun<T>(L, static_cast<function_type_t<std::decay_t<F>>>(func));
        }
        
        template <class T, class F>
        void fun(lua_State *L, F&& func)
        {
            fun<T>(L, static_cast<function_type_t<std::decay_t<F>>>(func));
        }

        template <class T, class F>
        void cfun(lua_State *L, F&& func)
        {
            cfun<T>(L, static_cast<function_type_t<std::decay_t<F>>>(func));
        }
    }

    template<typename T>
    struct disect_function;

    template<typename R, typename ...Args>
    struct disect_function<R(Args...)>
    {
        static constexpr size_t nargs = sizeof...(Args);
        static constexpr bool isClass = false;

        template <size_t i>
        struct arg
        {
            typedef typename std::tuple_element<i, std::tuple<Args...>>::type type;
        };
    };

    template<typename R, typename ...Args>
    struct disect_function<R(Args...) const>
    {
        static constexpr size_t nargs = sizeof...(Args);
        static constexpr bool isClass = false;

        template <size_t i>
        struct arg
        {
            typedef typename std::tuple_element<i, std::tuple<Args...>>::type type;
        };
    };

    template<typename R, typename ...Args>
    struct disect_function<R(*)(Args...)>
    {
        static constexpr size_t nargs = sizeof...(Args);
        static constexpr bool isClass = false;

        template <size_t i>
        struct arg
        {
            typedef typename std::tuple_element<i, std::tuple<Args...>>::type type;
        };
    };

    template<typename R, typename T, typename ...Args>
    struct disect_function<R(T::*)(Args...)>
    {
        static constexpr size_t nargs = sizeof...(Args);
        static constexpr bool isClass = true;
        static constexpr T* classT = nullptr;

        template <size_t i>
        struct arg
        {
            typedef typename std::tuple_element<i, std::tuple<Args...>>::type type;
        };
    };

    template<typename R, typename T, typename ...Args>
    struct disect_function<R(T::*)(Args...) const>
    {
        static constexpr size_t nargs = sizeof...(Args);
        static constexpr bool isClass = true;
        static constexpr T* classT = nullptr;

        template <size_t i>
        struct arg
        {
            typedef typename std::tuple_element<i, std::tuple<Args...>>::type type;
        };
    };

    template<typename R, typename ...Args>
    struct disect_function<std::function<R(Args...)>>
    {
        static constexpr size_t nargs = sizeof...(Args);
        static constexpr bool isClass = false;

        template <size_t i>
        struct arg
        {
            typedef typename std::tuple_element<i, std::tuple<Args...>>::type type;
        };
    };

    template <typename F, size_t N = 0, size_t J = 1>
    void LoopTupleT(lua_State *L)
    {
        if constexpr (N == 0)
        {
            if constexpr(!std::is_same_v<State, std::decay_t<disect_function<F>::template arg<0>::type>> && !std::is_same_v<Environment, std::decay_t<disect_function<F>::template arg<0>::type>>)
            {
                lua_pushinteger(L, detail::basic_type<std::decay_t<disect_function<F>::template arg<0>::type>>(L));
                lua_rawseti(L, -2, J + (disect_function<F>::isClass ? 1 : 0));
                LoopTupleT<F, 1, J + 1>(L);
            } else if constexpr(std::is_same_v<Object, std::decay_t<disect_function<F>::template arg<0>::type>> || std::is_same_v<ObjectRef, std::decay_t<disect_function<F>::template arg<0>::type>>)
            {
                lua_pushinteger(L, -2);
                lua_rawseti(L, -2, J + (disect_function<F>::isClass ? 1 : 0));
                LoopTupleT<F, 1, J + 1>(L);
            } else
                LoopTupleT<F, 1, J>(L);
        } else if constexpr(N < disect_function<F>::nargs) {
            if constexpr(!std::is_same_v<State, std::decay_t<disect_function<F>::template arg<N-1>::type>> && !std::is_same_v<Environment, std::decay_t<disect_function<F>::template arg<N-1>::type>>)
            {
                lua_pushinteger(L, detail::basic_type<std::decay_t<disect_function<F>::template arg<N-1>::type>>(L));
                lua_rawseti(L, -2, J + (disect_function<F>::isClass ? 1 : 0));
                LoopTupleT<F, N + 1, J + 1>(L);
            } else if constexpr(std::is_same_v<Object, std::decay_t<disect_function<F>::template arg<N-1>::type>> || std::is_same_v<ObjectRef, std::decay_t<disect_function<F>::template arg<N-1>::type>>)
            {
                lua_pushinteger(L, -2);
                lua_rawseti(L, -2, J + (disect_function<F>::isClass ? 1 : 0));
                LoopTupleT<F, N + 1, J + 1>(L);
            } else
                LoopTupleT<F, N + 1, J>(L);
        }
    }

    template<typename C, typename ...Functions>
    class OverloadedFunction {
        lua_State* L = nullptr;
    public:
        OverloadedFunction(lua_State *L, Functions... functions) {
            this->L = L;
            lua_newtable(L);
            auto fun_index = 1;
            auto push_fun = [&fun_index](lua_State* L, auto f) {
                lua_newtable(L);

                lua_newtable(L);
                if constexpr(disect_function<decltype(f)>::isClass) {
                    lua_pushinteger(L, LUA_TUSERDATA);
                    lua_rawseti(L, -2, 1);
                }

                if constexpr(disect_function<decltype(f)>::nargs > 0)
                    LoopTupleT<decltype(f)>(L);

                lua_pushinteger(L, lua_getlen(L, -1));
                lua_setfield(L, -3, "nargs");

                lua_setfield(L, -2, "argl");

                Function::fun<C>(L, f);
                lua_setfield(L, -2, "f");

                lua_pushstring(L, typeid(f).name());
                lua_setfield(L, -2, "typeid");

                lua_rawseti(L, -2, fun_index++);

                return 0;
            };
            (push_fun(L, functions), ...);
            lua_pushcclosure(L, call, 1);
        }
        static int call(lua_State* L) {
            auto argn = lua_gettop(L);

            lua_pushvalue(L, lua_upvalueindex(1));

            lua_pushnil(L);
            while(lua_next(L, -2) != 0) {
                lua_getfield(L, -1, "nargs");
                if (lua_tointeger(L, -1) != argn)
                {
                    lua_pop(L, 2);
                    continue;
                }
                lua_pop(L, 1);

                bool valid = true;

                lua_getfield(L, -1, "argl");
                lua_pushnil(L);
                while(lua_next(L, -2) != 0) {
                    if (lua_tointeger(L, -1) == -2 || lua_tointeger(L, -1) != lua_type(L, lua_tointeger(L, -2)))
                        valid = false;
                    lua_pop(L, 1);
                }
                lua_pop(L, 1);

                if (valid) {
                    lua_getfield(L, -1, "f");
                    lua_insert(L, 1);
                    lua_pop(L, 3);
                    return LuaBinding::pcall(L, argn, LUA_MULTRET);
                }

                lua_pop(L, 1);
            }

            luaL_error(L, "no matching overload with %d args and input values", argn);

            return 0;
        }
    };

    namespace detail {
        namespace _resolve {
            template <typename Sig, typename C>
            inline Sig C::* resolve_v(std::false_type, Sig C::* mem_func_ptr) {
                return mem_func_ptr;
            }

            template <typename Sig, typename C>
            inline Sig C::* resolve_v(std::true_type, Sig C::* mem_variable_ptr) {
                return mem_variable_ptr;
            }
        }

        template <typename... Args, typename R>
        inline auto resolve(R fun_ptr(Args...))->R(*)(Args...) {
            return fun_ptr;
        }

        template <typename Sig>
        inline Sig* resolve(Sig* fun_ptr) {
            return fun_ptr;
        }

        template <typename... Args, typename R, typename C>
        inline auto resolve(R(C::* mem_ptr)(Args...))->R(C::*)(Args...) {
            return mem_ptr;
        }

        template <typename Sig, typename C>
        inline Sig C::* resolve(Sig C::* mem_ptr) {
            return _resolve::resolve_v(std::is_member_object_pointer<Sig C::*>(), mem_ptr);
        }

        template<typename F>
        concept has_call_operator = requires (F) {
            &F::operator();
        };

        template<typename F>
        concept is_cfun_style = std::is_invocable_v<F, State> || std::is_invocable_v<F, lua_State*>;
        
        template <typename T>
        concept is_pushable_ref_fun = requires (T &t) {
            { Function::fun(nullptr, t) };
        };

        template <typename T>
        concept is_pushable_forward_fun = requires (T &&t) {
            { Function::fun(nullptr, t) };
        };

        template <typename T>
        concept is_pushable_ref_cfun = requires (T &t) {
            { Function::cfun(nullptr, t) };
        };

        template <typename T>
        concept is_pushable_forward_cfun = requires (T &&t) {
            { Function::cfun(nullptr, t) };
        };

        template <typename T>
        concept is_pushable_fun =
            is_pushable_ref_fun<std::decay_t<T>> || is_pushable_forward_fun<std::decay_t<T>>;

        template <typename T>
        concept is_pushable_cfun =
            (is_pushable_ref_cfun<std::decay_t<T>> || is_pushable_forward_cfun<std::decay_t<T>>) && is_cfun_style<std::decay_t<T>>;
    }
}