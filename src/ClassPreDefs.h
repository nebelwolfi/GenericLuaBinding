#pragma once

namespace LuaBinding {
    template<typename C, typename ...Functions>
    class OverloadedFunction;
    template<typename T>
    class Class;
    class DynClass;
    class Object;
    class ObjectRef;
    class Environment;
    class State;
    class IndexProxy;
    template<typename T>
    class Stack;
    template<typename T>
    class StackClass;
    template<typename T>
    class helper;



    template <class R, class... Params>
    class Traits;
    template <class T, class... Params>
    class TraitsCtor;
    template <class R, class... Params>
    class TraitsSTDFunction;
    template <class T>
    class TraitsCFunc;
    template <class R, class T, class... Params>
    class TraitsClass;
    template <class R, class T, class... Params>
    class TraitsFunClass;
    template <class R, class... Params>
    class TraitsNClass;
    template <class T>
    class TraitsClassNCFunc;
    template <class T>
    class TraitsClassFunNCFunc;
    template <class T>
    class TraitsClassCFunc;
    template <class T>
    class TraitsClassFunCFunc;
    template <class T>
    class TraitsClassLCFunc;
    template <class T>
    class TraitsClassFunLCFunc;
    template <class T>
    class TraitsClassFunNLCFunc;
    template <class R, class T>
    class TraitsClassProperty;
    template <class R, class T>
    class TraitsClassPropertyNFn;
    template <class R, class T>
    class TraitsClassPropertyFn;
    template <class R, class T>
    class TraitsClassPropertyNFunFn;
    template <class R, class T>
    class TraitsClassPropertyFunFn;
    template <class R, class T>
    class TraitsClassPropertyNFunCFn;
    template <class R, class T>
    class TraitsClassPropertyNFunLCFn;
    template <class R, class T>
    class TraitsClassPropertyFunLCFn;
    template <class R, class T>
    class TraitsClassPropertyNFunC;
    template <class R, class T>
    class TraitsClassPropertyFunC;
    template <class R, class T>
    class TraitsClassPropertyNCFun;
    template <class R, class T>
    class TraitsClassPropertyCFun;
}