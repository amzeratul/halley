# Selene

[![Build Status](https://travis-ci.org/jeremyong/Selene.svg?branch=master)](https://travis-ci.org/jeremyong/Selene)

Simple C++11 friendly header-only bindings to Lua 5.1+.

## Requirements

- Cmake 2.8+
- Lua 5.1+
- C++11 compliant compiler

## Usage

Selene is a headers-only library so you just need to include
"selene.h" to use this project.

To build the tests, do the following:

```
mkdir build
cd build
cmake ..
make
```

This will build a `test_runner` executable that you can run. If you wish to
include Lua from another location, you made pass the `LUA_INCLUDE_DIR` option
to cmake (i.e. `cmake .. -DLUA_INCLUDE_DIR=/path/to/lua/include/dir`).

## Usage

### Establishing Lua Context

```c++
using namespace sel;
State state; // creates a Lua context
State state{true}; // creates a Lua context and loads standard Lua libraries
```

When a `sel::State` object goes out of scope, the Lua context is
automatically destroyed in addition to all objects associated with it
(including C++ objects).

### Accessing elements

```lua
-- test.lua
foo = 4
bar = {}
bar[3] = "hi"
bar["key"] = "there"
```

```c++
sel::State state;
state.Load("/path/to/test.lua");
assert(state["foo"] == 4);
assert(state["bar"][3] == "hi");
assert(state["bar"]["key"] == "there");
```

When the `[]` operator is invoked on a `sel::State` object, a
`sel::Selector` object is returned. The `Selector` is type castable to
all the basic types that Lua can return.

If you access the same element frequently, it is recommended that you
cache the selector for fast access later like so:

```c++
auto bar3 = state["bar"][3]; // bar3 has type sel::Selector
bar3 = 4;
bar3 = 6;
std::cout << int(bar3) << std::endl;
```

### Calling Lua functions from C++

```lua
-- test.lua

function foo()
end

function add(a, b)
  return a + b
end

function sum_and_difference(a, b)
  return (a+b), (a-b);
end

function bar()
  return 4, true, "hi"
end

mytable = {}
function mytable.foo()
    return 4
end
```

```c++
sel::State state;
state.Load("/path/to/test.lua");

// Call function with no arguments or returns
state["foo"]();

// Call function with two arguments that returns an int
// The type parameter can be one of int, lua_Number, std::string,
// bool, or unsigned int
int result = state["add"](5, 2);
assert(result == 7);


// Call function that returns multiple values
int sum, difference;
sel::tie(sum, difference) = state["sum_and_difference"](3, 1);
assert(sum == 4 && difference == 2);

// Call function in table
result = state["mytable"]["foo"]();
assert(result == 4);
```

Note that multi-value returns must have `sel::tie`
on the LHS and not `std::tie`. This will create a `sel::Tuple` as
opposed to an `std::tuple` which has the `operator=` implemented for
the selector type.

### Calling Free-standing C++ functions from Lua

```c++
int my_multiply(int a, int b) {
    return (a*b);
}

sel::State state;

// Register the function to the Lua global "c_multiply"
state["c_multiply"] = &my_multiply;

// Now we can call it (we can also call it from within lua)
int result = state["c_multiply"](5, 2);
assert(result == 10);
```

You can also register functor objects, lambdas, and any fully
qualified `std::function`. See `test/interop_tests.h` for details.

#### Accepting Lua functions as Arguments

To retrieve a Lua function as a callable object in C++, you can use
the `sel::function` type like so:

```lua
-- test.lua

function add(a, b)
    return a+b
end

function pass_add(x, y)
    take_fun_arg(add, x, y)
end
```

```c++
int take_fun_arg(sel::function<int(int, int)> fun, int a, int b) {
    return fun(a, b);
}

sel::State state;
state["take_fun_arg"] = &take_fun_arg;
state.Load("test.lua");
assert(state["pass_add"](3, 5) == 8)
```

The `sel::function` type is pretty much identical to the
`std::function` type excepts it holds a `shared_ptr` to a Lua
reference. Once all instances of a particular `sel::function` go out
of scope, the Lua reference will automatically become unbound. Simply
copying and retaining an instance of a `sel::function` will allow it
to be callable later. You can also return a `sel::function` which will
then be callable in C++ or Lua.

### Running arbitrary code

```c++
sel::State state;
state("x = 5");
```

After running this snippet, `x` will have value 5 in the Lua runtime.
Snippets run in this way cannot return anything to the caller at this time.

### Registering Classes

```c++
struct Bar {
    int x;
    Bar(int x_) : x(x_) {}
    int AddThis(int y) { return x + y; }
};

sel::State state;
state["Bar"].SetClass<Bar, int>("add_this", &Bar::AddThis);
```

```lua
bar = Bar.new(5)
-- bar now refers to a new instance of Bar with its member x set to 5

x = bar:add_this(2)
-- x is now 7

bar = nil
--[[ the bar object will be destroyed the next time a garbage
     collection is run ]]--
```

The signature of the `SetClass` template method looks like the
following:

```c++
template <typename T, typename... CtorArgs, typename... Funs>
void Selector::SetClass(Funs... funs);
```

The template parameters supplied explicitly are first `T`, the class
you wish to register followed by `CtorArgs...`, the types that are
accepted by the class's constructor. In addition to primitive types,
you may also pass pointers or references to other types that have been
or will be registered. Note that constructor overloading
is not supported at this time. The arguments to the `SetClass`
function are a list of member functions you wish to register (callable
from Lua). The format is [function name, function pointer, ...].

After a class is registered, C++ functions and methods can return
pointers or references to Lua, and the class metatable will be
assigned correctly.

#### Registering Class Member Variables

For convenience, if you pass a pointer to a member instead of a member
function, Selene will automatically generate a setter and getter for
the member. The getter name is just the name of the member variable
you supply and the setter has "set_" prepended to that name.

```c++
// Define Bar as above
sel::State state;
state["Bar"].SetClass<Bar, int>("x", &Bar::x);
```

```lua
-- now we can do the following:
bar = Bar.new(4)

print(bar:x()) -- will print '4'

bar:set_x(-4)
print(bar:x()) -- will print '-4'
```

Member variables registered in this way which are declared `const`
will not have a setter generated for them.

### Registering Object Instances

You can also register an explicit object which was instantiated from
C++. However, this object cannot be inherited from and you are in
charge of the object's lifetime.

```c++
struct Foo {
    int x;
    Foo(int x_) : x(x_) {}

    int DoubleAdd(int y) { return 2 * (x + y); }
    void SetX(int x_) { x = x_; }
};

sel::State state;

// Instantiate a foo object with x initially set to 2
Foo foo(2);

// Binds the C++ instance foo to a table also called foo in Lua along
// with the DoubleAdd method and variable x. Binding a member variable
// will create a getter and setter as illustrated below.
// The user is not required to bind all members
state["foo"].SetObj(foo,
                    "double_add", &Foo::DoubleAdd,
                    "x", &Foo::x);

assert(state["foo"]["x"]() == 2);

state["foo"]["set_x"](4);
assert(foo.x == 4);

int result = state["foo"]["double_add"](3);
assert(result == 14);
```

In the above example, the functions `foo.double_add` and `foo.set_x`
will also be accessible from within Lua after registration occurs. As
with class member variables, object instance variables which are
`const` will not have a setter generated for them.

## Writeups

You can read more about this project in the three blogposts that describes it:

- [first part](http://www.jeremyong.com/blog/2014/01/10/interfacing-lua-with-templates-in-c-plus-plus-11/).
- [second part](http://www.jeremyong.com/blog/2014/01/14/interfacing-lua-with-templates-in-c-plus-plus-11-continued)
- [third part](http://www.jeremyong.com/blog/2014/01/21/interfacing-lua-with-templates-in-c-plus-plus-11-conclusion)
- [ref-qualifier usage](http://www.jeremyong.com/blog/2014/02/15/using-ref-qualifiers/)

There have been syntax changes in library usage but the underlying
concepts of variadic template use and generics is the same.

## Roadmap

The following features are planned, although nothing is guaranteed:

- Smarter Lua module loading
- Hooks for module reloading
- VS support
