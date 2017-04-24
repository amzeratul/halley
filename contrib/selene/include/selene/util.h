#pragma once

#include "ExceptionHandler.h"
#include <iostream>
#include <utility>

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

namespace sel {
inline std::ostream &operator<<(std::ostream &os, lua_State *l) {
    int top = lua_gettop(l);
    for (int i = 1; i <= top; ++i) {
        int t = lua_type(l, i);
        switch(t) {
        case LUA_TSTRING:
            os << lua_tostring(l, i);
            break;
        case LUA_TBOOLEAN:
            os << (lua_toboolean(l, i) ? "true" : "false");
            break;
        case LUA_TNUMBER:
            os << lua_tonumber(l, i);
            break;
        default:
            os << lua_typename(l, t);
            break;
        }
        os << " ";
    }
    return os;
}

inline void _print() {
    std::cout << std::endl;
}

template <typename T, typename... Ts>
inline void _print(T arg, Ts... args) {
    std::cout << arg << ", ";
    _print(args...);
}

inline bool check(lua_State *L, int code) {
#if LUA_VERSION_NUM >= 502
    if (code == LUA_OK) {
#else
    if (code == 0) {
#endif
        return true;
    } else {
        std::cout << lua_tostring(L, -1) << std::endl;
        return false;
    }
}

inline int Traceback(lua_State *L) {
    // Make nil and values not convertible to string human readable.
    const char* msg = "<not set>";
    if (!lua_isnil(L, -1)) {
        msg = lua_tostring(L, -1);
        if (!msg)
            msg = "<error object>";
    }
    lua_pushstring(L, msg);

    // call debug.traceback
    lua_getglobal(L, "debug");
    lua_getfield(L, -1, "traceback");
    lua_pushvalue(L, -3);
    lua_pushinteger(L, 2);
    lua_call(L, 2, 1);

    return 1;
}

inline int ErrorHandler(lua_State *L) {
    if(test_stored_exception(L) != nullptr) {
        return 1;
    }

    return Traceback(L);
}

inline int SetErrorHandler(lua_State *L) {
    lua_pushcfunction(L, &ErrorHandler);
    return lua_gettop(L);
}

template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args) {
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}
}
