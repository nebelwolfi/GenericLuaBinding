//------------------------------------------------------------------------------
/*
  https://github.com/vinniefalco/LuaBridge

  Copyright 2019, Dmitry Tarakanov
  Copyright 2012, Vinnie Falco <vinnie.falco@gmail.com>

  License: The MIT License (http://www.opensource.org/licenses/mit-license.php)

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/
//==============================================================================

#pragma once

#include <functional>

namespace LuaBinding::function_conversion {
    namespace function_traits {
        namespace type_lists {

            typedef void None;

            template <typename Head, typename Tail = None>
            struct TypeList
            {
                typedef Tail TailType;
            };

            template <class List>
            struct TypeListSize
            {
                static const size_t value = TypeListSize <typename List::TailType>::value + 1;
            };

            template <>
            struct TypeListSize <None>
            {
                static const size_t value = 0;
            };

            template <class... Params>
            struct MakeTypeList;

            template <class Param, class... Params>
            struct MakeTypeList <Param, Params...>
            {
                using Result = TypeList <Param, typename MakeTypeList <Params...>::Result>;
            };

            template <>
            struct MakeTypeList <>
            {
                using Result = None;
            };

            template <typename List>
            struct TypeListValues
            {
                static std::string const tostring (bool)
                {
                    return "";
                }
            };

            template <typename Head, typename Tail>
            struct TypeListValues <TypeList <Head, Tail> >
            {
                Head hd;
                TypeListValues <Tail> tl;

                TypeListValues (Head hd_, TypeListValues <Tail> const& tl_)
                        : hd (hd_), tl (tl_)
                {
                }

                static std::string tostring (bool comma = false)
                {
                    std::string s;

                    if (comma)
                        s = ", ";

                    s = s + typeid (Head).name ();

                    return s + TypeListValues <Tail>::tostring (true);
                }
            };

            template <typename Head, typename Tail>
            struct TypeListValues <TypeList <Head&, Tail> >
            {
                Head hd;
                TypeListValues <Tail> tl;

                TypeListValues (Head& hd_, TypeListValues <Tail> const& tl_)
                        : hd (hd_), tl (tl_)
                {
                }

                static std::string const tostring (bool comma = false)
                {
                    std::string s;

                    if (comma)
                        s = ", ";

                    s = s + typeid (Head).name () + "&";

                    return s + TypeListValues <Tail>::tostring (true);
                }
            };

            template <typename Head, typename Tail>
            struct TypeListValues <TypeList <Head const&, Tail> >
            {
                Head hd;
                TypeListValues <Tail> tl;

                TypeListValues (Head const& hd_, const TypeListValues <Tail>& tl_)
                        : hd (hd_), tl (tl_)
                {
                }

                static std::string const tostring (bool comma = false)
                {
                    std::string s;

                    if (comma)
                        s = ", ";

                    s = s + typeid (Head).name () + " const&";

                    return s + TypeListValues <Tail>::tostring (true);
                }
            };

            template <typename List, int Start = 1>
            struct ArgList
            {
            };

            template <int Start>
            struct ArgList <None, Start> : public TypeListValues <None>
            {
                ArgList (lua_State*)
                {
                }
            };

            template <typename Head, typename Tail, int Start>
            struct ArgList <TypeList <Head, Tail>, Start>
                    : public TypeListValues <TypeList <Head, Tail> >
            {
                ArgList (lua_State* L)
                        : TypeListValues <TypeList <Head, Tail> > (Stack <Head>::get (L, Start),
                                                                   ArgList <Tail, Start + 1> (L))
                {
                }
            };

        }

        template <class ReturnType, size_t NUM_PARAMS>
        struct Caller;

        template <class ReturnType>
        struct Caller <ReturnType, 0>
        {
            template <class Fn, class Params>
            static ReturnType f (Fn& fn, type_lists::TypeListValues <Params>& params)
            {
                return fn ();
            }

            template <class T, class MemFn, class Params>
            static ReturnType f (T* obj, MemFn& fn, type_lists::TypeListValues <Params>&)
            {
                return (obj->*fn) ();
            }
        };

        template <class ReturnType>
        struct Caller <ReturnType, 1>
        {
            template <class Fn, class Params>
            static ReturnType f (Fn& fn, type_lists::TypeListValues <Params>& tvl)
            {
                return fn (tvl.hd);
            }

            template <class T, class MemFn, class Params>
            static ReturnType f (T* obj, MemFn& fn, type_lists::TypeListValues <Params>& tvl)
            {
                return (obj->*fn) (tvl.hd);
            }
        };

        template <class ReturnType>
        struct Caller <ReturnType, 2>
        {
            template <class Fn, class Params>
            static ReturnType f (Fn& fn, type_lists::TypeListValues <Params>& tvl)
            {
                return fn (tvl.hd, tvl.tl.hd);
            }

            template <class T, class MemFn, class Params>
            static ReturnType f (T* obj, MemFn& fn, type_lists::TypeListValues <Params>& tvl)
            {
                return (obj->*fn) (tvl.hd, tvl.tl.hd);
            }
        };

        template <class ReturnType>
        struct Caller <ReturnType, 3>
        {
            template <class Fn, class Params>
            static ReturnType f (Fn& fn, type_lists::TypeListValues <Params>& tvl)
            {
                return fn (tvl.hd, tvl.tl.hd, tvl.tl.tl.hd);
            }

            template <class T, class MemFn, class Params>
            static ReturnType f (T* obj, MemFn& fn, type_lists::TypeListValues <Params>& tvl)
            {
                return (obj->*fn) (tvl.hd, tvl.tl.hd, tvl.tl.tl.hd);
            }
        };

        template <class ReturnType>
        struct Caller <ReturnType, 4>
        {
            template <class Fn, class Params>
            static ReturnType f (Fn& fn, type_lists::TypeListValues <Params>& tvl)
            {
                return fn (tvl.hd, tvl.tl.hd, tvl.tl.tl.hd, tvl.tl.tl.tl.hd);
            }

            template <class T, class MemFn, class Params>
            static ReturnType f (T* obj, MemFn& fn, type_lists::TypeListValues <Params>& tvl)
            {
                return (obj->*fn) (tvl.hd, tvl.tl.hd, tvl.tl.tl.hd, tvl.tl.tl.tl.hd);
            }
        };

        template <class ReturnType>
        struct Caller <ReturnType, 5>
        {
            template <class Fn, class Params>
            static ReturnType f (Fn& fn, type_lists::TypeListValues <Params>& tvl)
            {
                return fn (tvl.hd, tvl.tl.hd, tvl.tl.tl.hd, tvl.tl.tl.tl.hd, tvl.tl.tl.tl.tl.hd);
            }

            template <class T, class MemFn, class Params>
            static ReturnType f (T* obj, MemFn& fn, type_lists::TypeListValues <Params>& tvl)
            {
                return (obj->*fn) (tvl.hd, tvl.tl.hd, tvl.tl.tl.hd, tvl.tl.tl.tl.hd, tvl.tl.tl.tl.tl.hd);
            }
        };

        template <class ReturnType>
        struct Caller <ReturnType, 6>
        {
            template <class Fn, class Params>
            static ReturnType f (Fn& fn, type_lists::TypeListValues <Params>& tvl)
            {
                return fn (tvl.hd, tvl.tl.hd, tvl.tl.tl.hd, tvl.tl.tl.tl.hd, tvl.tl.tl.tl.tl.hd,
                           tvl.tl.tl.tl.tl.tl.hd);
            }

            template <class T, class MemFn, class Params>
            static ReturnType f (T* obj, MemFn& fn, type_lists::TypeListValues <Params>& tvl)
            {
                return (obj->*fn) (tvl.hd, tvl.tl.hd, tvl.tl.tl.hd, tvl.tl.tl.tl.hd, tvl.tl.tl.tl.tl.hd,
                                   tvl.tl.tl.tl.tl.tl.hd);
            }
        };

        template <class ReturnType>
        struct Caller <ReturnType, 7>
        {
            template <class Fn, class Params>
            static ReturnType f (Fn& fn, type_lists::TypeListValues <Params>& tvl)
            {
                return fn (tvl.hd, tvl.tl.hd, tvl.tl.tl.hd, tvl.tl.tl.tl.hd, tvl.tl.tl.tl.tl.hd,
                           tvl.tl.tl.tl.tl.tl.hd, tvl.tl.tl.tl.tl.tl.tl.hd);
            }

            template <class T, class MemFn, class Params>
            static ReturnType f (T* obj, MemFn& fn, type_lists::TypeListValues <Params>& tvl)
            {
                return (obj->*fn) (tvl.hd, tvl.tl.hd, tvl.tl.tl.hd, tvl.tl.tl.tl.hd, tvl.tl.tl.tl.tl.hd,
                                   tvl.tl.tl.tl.tl.tl.hd, tvl.tl.tl.tl.tl.tl.tl.hd);
            }
        };

        template <class ReturnType>
        struct Caller <ReturnType, 8>
        {
            template <class Fn, class Params>
            static ReturnType f (Fn& fn, type_lists::TypeListValues <Params>& tvl)
            {
                return fn (tvl.hd, tvl.tl.hd, tvl.tl.tl.hd, tvl.tl.tl.tl.hd, tvl.tl.tl.tl.tl.hd,
                           tvl.tl.tl.tl.tl.tl.hd, tvl.tl.tl.tl.tl.tl.tl.hd, tvl.tl.tl.tl.tl.tl.tl.tl.hd);
            }

            template <class T, class MemFn, class Params>
            static ReturnType f (T* obj, MemFn& fn, type_lists::TypeListValues <Params>& tvl)
            {
                return (obj->*fn) (tvl.hd, tvl.tl.hd, tvl.tl.tl.hd, tvl.tl.tl.tl.hd, tvl.tl.tl.tl.tl.hd,
                                   tvl.tl.tl.tl.tl.tl.hd, tvl.tl.tl.tl.tl.tl.tl.hd, tvl.tl.tl.tl.tl.tl.tl.tl.hd);
            }
        };

        template <class ReturnType>
        struct Caller <ReturnType, 9>
        {
            template <class Fn, class Params>
            static ReturnType f (Fn& fn, type_lists::TypeListValues <Params>& tvl)
            {
                return fn (tvl.hd, tvl.tl.hd, tvl.tl.tl.hd, tvl.tl.tl.tl.hd, tvl.tl.tl.tl.tl.hd,
                           tvl.tl.tl.tl.tl.tl.hd, tvl.tl.tl.tl.tl.tl.tl.hd, tvl.tl.tl.tl.tl.tl.tl.tl.hd,
                           tvl.tl.tl.tl.tl.tl.tl.tl.tl.hd);
            }

            template <class T, class MemFn, class Params>
            static ReturnType f (T* obj, MemFn& fn, type_lists::TypeListValues <Params>& tvl)
            {
                return (obj->*fn) (tvl.hd, tvl.tl.hd, tvl.tl.tl.hd, tvl.tl.tl.tl.hd, tvl.tl.tl.tl.tl.hd,
                                   tvl.tl.tl.tl.tl.tl.hd, tvl.tl.tl.tl.tl.tl.tl.hd, tvl.tl.tl.tl.tl.tl.tl.tl.hd,
                                   tvl.tl.tl.tl.tl.tl.tl.tl.tl.hd);
            }
        };

        template <class ReturnType>
        struct Caller <ReturnType, 10>
        {
            template <class Fn, class Params>
            static ReturnType f(Fn& fn, type_lists::TypeListValues <Params>& tvl)
            {
                return fn(tvl.hd, tvl.tl.hd, tvl.tl.tl.hd, tvl.tl.tl.tl.hd, tvl.tl.tl.tl.tl.hd,
                          tvl.tl.tl.tl.tl.tl.hd, tvl.tl.tl.tl.tl.tl.tl.hd, tvl.tl.tl.tl.tl.tl.tl.tl.hd,
                          tvl.tl.tl.tl.tl.tl.tl.tl.tl.hd, tvl.tl.tl.tl.tl.tl.tl.tl.tl.tl.hd);
            }

            template <class T, class MemFn, class Params>
            static ReturnType f(T* obj, MemFn& fn, type_lists::TypeListValues <Params>& tvl)
            {
                return (obj->*fn) (tvl.hd, tvl.tl.hd, tvl.tl.tl.hd, tvl.tl.tl.tl.hd, tvl.tl.tl.tl.tl.hd,
                                   tvl.tl.tl.tl.tl.tl.hd, tvl.tl.tl.tl.tl.tl.tl.hd, tvl.tl.tl.tl.tl.tl.tl.tl.hd,
                                   tvl.tl.tl.tl.tl.tl.tl.tl.tl.hd, tvl.tl.tl.tl.tl.tl.tl.tl.tl.tl.hd);
            }
        };

        template <class ReturnType>
        struct Caller <ReturnType, 11>
        {
            template <class Fn, class Params>
            static ReturnType f(Fn& fn, type_lists::TypeListValues <Params>& tvl)
            {
                return fn(tvl.hd, tvl.tl.hd, tvl.tl.tl.hd, tvl.tl.tl.tl.hd, tvl.tl.tl.tl.tl.hd,
                          tvl.tl.tl.tl.tl.tl.hd, tvl.tl.tl.tl.tl.tl.tl.hd, tvl.tl.tl.tl.tl.tl.tl.tl.hd,
                          tvl.tl.tl.tl.tl.tl.tl.tl.tl.hd, tvl.tl.tl.tl.tl.tl.tl.tl.tl.tl.hd,
                          tvl.tl.tl.tl.tl.tl.tl.tl.tl.tl.tl.hd);
            }

            template <class T, class MemFn, class Params>
            static ReturnType f(T* obj, MemFn& fn, type_lists::TypeListValues <Params>& tvl)
            {
                return (obj->*fn) (tvl.hd, tvl.tl.hd, tvl.tl.tl.hd, tvl.tl.tl.tl.hd, tvl.tl.tl.tl.tl.hd,
                                   tvl.tl.tl.tl.tl.tl.hd, tvl.tl.tl.tl.tl.tl.tl.hd, tvl.tl.tl.tl.tl.tl.tl.tl.hd,
                                   tvl.tl.tl.tl.tl.tl.tl.tl.tl.hd, tvl.tl.tl.tl.tl.tl.tl.tl.tl.tl.hd,
                                   tvl.tl.tl.tl.tl.tl.tl.tl.tl.tl.tl.hd);
            }
        };

        template <class ReturnType>
        struct Caller <ReturnType, 12>
        {
            template <class Fn, class Params>
            static ReturnType f(Fn& fn, type_lists::TypeListValues <Params>& tvl)
            {
                return fn(tvl.hd, tvl.tl.hd, tvl.tl.tl.hd, tvl.tl.tl.tl.hd, tvl.tl.tl.tl.tl.hd,
                          tvl.tl.tl.tl.tl.tl.hd, tvl.tl.tl.tl.tl.tl.tl.hd, tvl.tl.tl.tl.tl.tl.tl.tl.hd,
                          tvl.tl.tl.tl.tl.tl.tl.tl.tl.hd, tvl.tl.tl.tl.tl.tl.tl.tl.tl.tl.hd,
                          tvl.tl.tl.tl.tl.tl.tl.tl.tl.tl.tl.hd,
                          tvl.tl.tl.tl.tl.tl.tl.tl.tl.tl.tl.tl.hd);
            }

            template <class T, class MemFn, class Params>
            static ReturnType f(T* obj, MemFn& fn, type_lists::TypeListValues <Params>& tvl)
            {
                return (obj->*fn) (tvl.hd, tvl.tl.hd, tvl.tl.tl.hd, tvl.tl.tl.tl.hd, tvl.tl.tl.tl.tl.hd,
                                   tvl.tl.tl.tl.tl.tl.hd, tvl.tl.tl.tl.tl.tl.tl.hd, tvl.tl.tl.tl.tl.tl.tl.tl.hd,
                                   tvl.tl.tl.tl.tl.tl.tl.tl.tl.hd, tvl.tl.tl.tl.tl.tl.tl.tl.tl.tl.hd,
                                   tvl.tl.tl.tl.tl.tl.tl.tl.tl.tl.tl.hd,
                                   tvl.tl.tl.tl.tl.tl.tl.tl.tl.tl.tl.tl.hd);
            }
        };

        template <class ReturnType>
        struct Caller <ReturnType, 13>
        {
            template <class Fn, class Params>
            static ReturnType f(Fn& fn, type_lists::TypeListValues <Params>& tvl)
            {
                return fn(tvl.hd, tvl.tl.hd, tvl.tl.tl.hd, tvl.tl.tl.tl.hd, tvl.tl.tl.tl.tl.hd,
                          tvl.tl.tl.tl.tl.tl.hd, tvl.tl.tl.tl.tl.tl.tl.hd, tvl.tl.tl.tl.tl.tl.tl.tl.hd,
                          tvl.tl.tl.tl.tl.tl.tl.tl.tl.hd, tvl.tl.tl.tl.tl.tl.tl.tl.tl.tl.hd,
                          tvl.tl.tl.tl.tl.tl.tl.tl.tl.tl.tl.hd,
                          tvl.tl.tl.tl.tl.tl.tl.tl.tl.tl.tl.tl.hd,
                          tvl.tl.tl.tl.tl.tl.tl.tl.tl.tl.tl.tl.tl.hd);
            }

            template <class T, class MemFn, class Params>
            static ReturnType f(T* obj, MemFn& fn, type_lists::TypeListValues <Params>& tvl)
            {
                return (obj->*fn) (tvl.hd, tvl.tl.hd, tvl.tl.tl.hd, tvl.tl.tl.tl.hd, tvl.tl.tl.tl.tl.hd,
                                   tvl.tl.tl.tl.tl.tl.hd, tvl.tl.tl.tl.tl.tl.tl.hd, tvl.tl.tl.tl.tl.tl.tl.tl.hd,
                                   tvl.tl.tl.tl.tl.tl.tl.tl.tl.hd, tvl.tl.tl.tl.tl.tl.tl.tl.tl.tl.hd,
                                   tvl.tl.tl.tl.tl.tl.tl.tl.tl.tl.tl.hd,
                                   tvl.tl.tl.tl.tl.tl.tl.tl.tl.tl.tl.tl.hd,
                                   tvl.tl.tl.tl.tl.tl.tl.tl.tl.tl.tl.tl.tl.hd);
            }
        };

        template <class ReturnType>
        struct Caller <ReturnType, 14>
        {
            template <class Fn, class Params>
            static ReturnType f(Fn& fn, type_lists::TypeListValues <Params>& tvl)
            {
                return fn(tvl.hd, tvl.tl.hd, tvl.tl.tl.hd, tvl.tl.tl.tl.hd, tvl.tl.tl.tl.tl.hd,
                          tvl.tl.tl.tl.tl.tl.hd, tvl.tl.tl.tl.tl.tl.tl.hd, tvl.tl.tl.tl.tl.tl.tl.tl.hd,
                          tvl.tl.tl.tl.tl.tl.tl.tl.tl.hd, tvl.tl.tl.tl.tl.tl.tl.tl.tl.tl.hd,
                          tvl.tl.tl.tl.tl.tl.tl.tl.tl.tl.tl.hd,
                          tvl.tl.tl.tl.tl.tl.tl.tl.tl.tl.tl.tl.hd,
                          tvl.tl.tl.tl.tl.tl.tl.tl.tl.tl.tl.tl.tl.hd,
                          tvl.tl.tl.tl.tl.tl.tl.tl.tl.tl.tl.tl.tl.tl.hd);
            }

            template <class T, class MemFn, class Params>
            static ReturnType f(T* obj, MemFn& fn, type_lists::TypeListValues <Params>& tvl)
            {
                return (obj->*fn) (tvl.hd, tvl.tl.hd, tvl.tl.tl.hd, tvl.tl.tl.tl.hd, tvl.tl.tl.tl.tl.hd,
                                   tvl.tl.tl.tl.tl.tl.hd, tvl.tl.tl.tl.tl.tl.tl.hd, tvl.tl.tl.tl.tl.tl.tl.tl.hd,
                                   tvl.tl.tl.tl.tl.tl.tl.tl.tl.hd, tvl.tl.tl.tl.tl.tl.tl.tl.tl.tl.hd,
                                   tvl.tl.tl.tl.tl.tl.tl.tl.tl.tl.tl.hd,
                                   tvl.tl.tl.tl.tl.tl.tl.tl.tl.tl.tl.tl.hd,
                                   tvl.tl.tl.tl.tl.tl.tl.tl.tl.tl.tl.tl.tl.hd,
                                   tvl.tl.tl.tl.tl.tl.tl.tl.tl.tl.tl.tl.tl.tl.hd);
            }
        };

        template <class ReturnType, class Fn, class Params>
        ReturnType doCall (Fn& fn, type_lists::TypeListValues <Params>& tvl)
        {
            return Caller <ReturnType, type_lists::TypeListSize <Params>::value>::f (fn, tvl);
        }

        template <class ReturnType, class T, class MemFn, class Params>
        static ReturnType doCall(T* obj, MemFn& fn, type_lists::TypeListValues <Params>& tvl)
        {
            return Caller <ReturnType, type_lists::TypeListSize <Params>::value>::f (obj, fn, tvl);
        }

        template <class MemFn, class D = MemFn>
        struct FuncTraits
        {
        };

        template <class R, class... ParamList>
        struct FuncTraits <R (*) (ParamList...)>
        {
            static bool const isMemberFunction = false;
            using DeclType = R (*) (ParamList...);
            using ReturnType = R;
            using Params = typename type_lists::MakeTypeList <ParamList...>::Result;

            static R call (const DeclType& fp, type_lists::TypeListValues <Params>& tvl)
            {
                return doCall <R> (fp, tvl);
            }
        };

        template <class R, class... ParamList>
        struct FuncTraits <R (__stdcall *) (ParamList...)>
        {
            static bool const isMemberFunction = false;
            using DeclType = R (__stdcall *) (ParamList...);
            using ReturnType = R;
            using Params = typename type_lists::MakeTypeList <ParamList...>::Result;

            static R call (const DeclType& fp, type_lists::TypeListValues <Params>& tvl)
            {
                return doCall <R> (fp, tvl);
            }
        };

        template <class T, class R, class... ParamList>
        struct FuncTraits <R (T::*) (ParamList...)>
        {
            static bool const isMemberFunction = true;
            static bool const isConstMemberFunction = false;
            using DeclType = R (T::*) (ParamList...);
            using ClassType = T;
            using ReturnType = R;
            using Params = typename type_lists::MakeTypeList <ParamList...>::Result;

            static R call (ClassType* obj, const DeclType& fp, type_lists::TypeListValues <Params> tvl)
            {
                return doCall <R> (obj, fp, tvl);
            }
        };

        template <class T, class R, class... ParamList>
        struct FuncTraits <R (T::*) (ParamList...) const>
        {
            static bool const isMemberFunction = true;
            static bool const isConstMemberFunction = true;
            using DeclType = R (T::*) (ParamList...) const;
            using ClassType = T;
            using ReturnType = R;
            using Params = typename type_lists::MakeTypeList <ParamList...>::Result;

            static R call (const ClassType* obj, const DeclType& fp, type_lists::TypeListValues <Params> tvl)
            {
                return doCall <R> (obj, fp, tvl);
            }
        };

        template <class R, class... ParamList>
        struct FuncTraits <std::function <R (ParamList...)>>
        {
            static bool const isMemberFunction = false;
            static bool const isConstMemberFunction = false;
            using DeclType = std::function <R (ParamList...)>;
            using ReturnType = R;
            using Params = typename  type_lists::MakeTypeList <ParamList...>::Result;

            static ReturnType call (DeclType& fn, type_lists::TypeListValues <Params>& tvl)
            {
                return doCall <ReturnType> (fn, tvl);
            }
        };
    }
}
