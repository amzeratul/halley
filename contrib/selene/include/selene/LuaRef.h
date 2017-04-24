#pragma once

#include <memory>
#include <vector>
#include "primitives.h"
#include "ResourceHandler.h"

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

namespace sel {
namespace detail {
class LuaRefDeleter {
private:
    lua_State *_state;
public:
    LuaRefDeleter(lua_State *state) : _state{state} {}
    void operator()(int *ref) const {
        luaL_unref(_state, LUA_REGISTRYINDEX, *ref);
        delete ref;
    }
};
}
class LuaRef {
private:
    std::shared_ptr<int> _ref;
public:
    LuaRef(lua_State *state, int ref)
        : _ref(new int{ref}, detail::LuaRefDeleter{state}) {}

    LuaRef(lua_State *state)
        : LuaRef(state, LUA_REFNIL)
        {}

    void Push(lua_State *state) const {
        lua_rawgeti(state, LUA_REGISTRYINDEX, *_ref);
    }
};

template <typename T>
LuaRef make_Ref(lua_State * state, T&& t) {
    detail::_push(state, std::forward<T>(t));
    return LuaRef(state, luaL_ref(state, LUA_REGISTRYINDEX));
}

namespace detail {
    inline void append_ref_recursive(lua_State *, std::vector<LuaRef> &) {}

    template <typename Head, typename... Tail>
    void append_ref_recursive(lua_State * state, std::vector<LuaRef> & refs, Head&& head, Tail&&... tail) {
        refs.push_back(make_Ref(state, std::forward<Head>(head)));

        append_ref_recursive(state, refs, std::forward<Tail>(tail)...);
    }
}

template <typename... Args>
std::vector<LuaRef> make_Refs(lua_State * state, Args&&... args) {
    std::vector<LuaRef> refs;
    refs.reserve(sizeof...(Args));

    detail::append_ref_recursive(state, refs, std::forward<Args>(args)...);
    return refs;
}
}
