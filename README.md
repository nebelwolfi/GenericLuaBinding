<h1 align="center">Generic Header-Only Lua Binding</h1>

<div align="center">

[![Status](https://img.shields.io/badge/status-active-success.svg)]()
[![GitHub Issues](https://img.shields.io/github/issues/nebelwolfi/GenericLuaBinding.svg)](https://github.com/nebelwolfi/GenericLuaBinding/issues)
[![GitHub Pull Requests](https://img.shields.io/github/issues-pr/nebelwolfi/GenericLuaBinding.svg)](https://github.com/nebelwolfi/GenericLuaBinding/pulls)
[![License](https://img.shields.io/badge/license-MIT-blue.svg)](/LICENSE)

</div>

---

## 📝 Table of Contents

- [Compatability](#compatability)
- [Quick and Dirty](#quick_and_dirty)
- [Full Documentation](#full_documentation)
- [Authors](#authors)

### 🎈 Compatability <a name="compatability"></a>

Tested against LuaJIT 2.1 x86 and Lua 5.4 x86, will happily fix compats with other versions - simply open an issue

## 🔧 Quick and Dirty <a name="quick_and_dirty"></a>
```cpp
#include <lua.hpp>
#include <LuaBinding.h>
```
#### standalone example
```cpp
int main() {
  auto S = std::make_unique<LuaBinding::State>();

  S->addClass<Sprite>("Sprite")
    .ctor<std::string>()
    .prop_fun("size", &LuaSprite::GetSize)
    .prop_fun("width", [](LuaSprite* self){ return self->GetSize().x; })
    .prop("info", &LuaSprite::info)
    .fun("draw", &LuaSprite::Draw);

  auto env = S->addEnv();
  env.set("var", 1);

  try {
    // execute
    S->exec(R"(
      local s = Sprite("icon.png")
      print(s.width)
      s:draw()
    )");
    // or execute with env
    S->exec(env, R"(
      print(var)
    )");
  } catch (std::exception& e) {
    printf("Lua Error: %s", e.what());
  }
}
```
#### lua module example
```cpp
void my_print(LuaBinding::State S) {
  std::string out;
  // iterate over args from 1 -> argn
  for (auto o : S) {
    out += o.as<std::string>();
    out += "\t";
  }
  printf("%s", out.c_str());
}

int printf(lua_State *L) {
  auto S = LuaBinding::State(L);
  S["string"]["format"].push(1);
  LuaBinding::pcall(L, S.n() - 1, 1);
  return print(L);
}

int luaopen_mylib(lua_State *L) {
  LuaBinding::State S(L);

  // create global table and get ref
  auto CLib = S.table("CLib");
  // set prop
  CLib.set("name", "CLib");

  // set func
  CLib.fun("print", my_print);
  // can also do it like this
  S.cfun("CLib.printf", printf);

  // push it to stack as return value
  S.at("CLib").push();
  return 1;
}
```

## ✈️ Full Documentation <a name="full_documentation"></a>

```cpp
State:
  addClass<T>(name) -> Class<T>       // new class
  addEnv() -> Env                     // new env
  n() -> int                          // count of elements in lua stack
  pop(n)                              // pops n values from lua stack
  push(params...)                     // pushs all params to lua stack
  alloc<T>(args...) -> T*             // allocates a new udata: new T(args...)
  fun(f)                              // pushes a func to lua
  fun(name, f)                        // sets a func global
  cfun(f)                             // pushes a cfunc to lua
  cfun(name, f)                       // sets a cfunc global
  overload(name, funcs)               // sets overloaded funcs global
  exec(code, argn, nres) -> int       // executes code
  exec(env, code, argn, nres) -> int  // executes code in an env
  call<T>(params...) -> T             // call func on top of stack
  call<T>(env, params...) -> T        // call func on top of stack in an env
  error(fmt, ...)                     // throws error in lua
  at(index : int) -> Object           // returns Object at stack index
  at(index : string) -> ObjectRef     // returns ObjectRef at global index
  table(index : string) -> ObjectRef  // ^ and creates it as table if it does not exist
  global(name, t)                     // sets t global
  front() -> Object                   // Object at stack index 1
  top() -> Object                     // Object at stack index -1

Class
  ctor<params...>()                   // allows lua to call the constructor
  fun(name, f)                        // binds member func
  cfun(name, f)                       // binds member cfunc
  vfun(name, f)                       // binds member valid func
                                      // object will count as invalid if it returns false
  meta_fun(name, f)                   // binds meta func
  meta_cfun(name, f)                  // binds meta cfunc
  prop(name, p)                       // binds prop
  prop_fun(name, getf [, setf])       // binds property with getter and setter
  prop_cfun(name, getf [, setf])      // binds property with cgetter and csetter
  overload(name, funcs)               // binds overloaded funcs

Object     // on stack
ObjectRef  // stored in registry
Env        // from State::addEnv
IndexProxy // from obj[] or state[]
  tostring() -> const char*           // calls lua_tostring()
  tolstring() -> const char*          // calls tostring(o) in lua and returns the result
  as<T>() -> T                        // retrieves value from lua
  extract<T>() -> T                   // ^ and pops it from stack/registry
  is<T>() -> bool                     // check if obj matches type T
  topointer() -> const void*          // converts the obj to a pointer
  operator[int] -> Object             // gets obj by table index
  operator[const char*] -> Object     // gets obj by table index
  push()                              // pushes this object to top of stack
  pop()                               // pops this object from stack
  call<R>(params...) -> R             // calls obj with return type T
  call<R>(env, params...) -> R        // calls obj with return type T in an env
  call<int n>(params...) -> void      // calls obj with result count n
  call<int n>(env, params...) -> void // calls obj with result count n in an env
  set(index, t)                       // sets table index value
  fun(index, f)                       // sets table index func
  cfun(index, f)                      // sets table index cfunc
  index() -> int                      // stack/registry index
  type() -> int                       // LUA_T...
  valid() -> bool                     // checks if obj is still valid
  valid(t) -> bool                    // checks if obj is still valid and of lua type t
  valid(State) -> bool                // checks if obj is still valid in a State
  valid(L) -> bool                    // checks if obj is still valid in a lua State
  valid(State, t) -> bool             // checks if obj is still valid in a State and of lua type t
  valid(L, t) -> bool                 // checks if obj is still valid in a lua State and of lua type t
  len() -> int                        // table length or udata size
```
Lua C Function formats supported by cfunc calls:
```cpp
int f(lua_State*);
int f(State);
std::function<int(lua_State*)>
std::function<int(State)>
```
And for class functions:
```cpp
int T::f(lua_State*);
int T::f(State);
std::function<int(T*, lua_State*)>
std::function<int(T*, State)>
```

## ✍️ Authors <a name = "authors"></a>

- [@nebelwolfi](https://github.com/nebelwolfi)

See also the list of [contributors](https://github.com/nebelwolfi/GenericLuaBinding/contributors) who participated in this project.
