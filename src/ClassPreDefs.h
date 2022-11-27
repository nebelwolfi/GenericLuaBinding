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
}