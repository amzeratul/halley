#pragma once
#include <functional>
#include "primitives.h"
#include <string>

extern "C" {
#include <lua.h>
#include <lauxlib.h>
}

namespace sel {

struct stored_exception {
    std::string what;
    std::exception_ptr exception;
};

inline std::string const * _stored_exception_metatable_name() {
    static std::string const name = "selene_stored_exception";
    return &name;
}

inline int _delete_stored_exception(lua_State * l) {
    void * user_data = lua_touserdata(l, -1);
    static_cast<stored_exception *>(user_data)->~stored_exception();
    return 0;
}

inline int _push_stored_exceptions_what(lua_State * l) {
    void * user_data = lua_touserdata(l, -1);
    std::string const & what = static_cast<stored_exception *>(user_data)->what;
    detail::_push(l, what);
    return 1;
}

inline void _register_stored_exception_metatable(lua_State * l) {
    luaL_newmetatable(l, _stored_exception_metatable_name()->c_str());
    lua_pushcfunction(l, _delete_stored_exception);
    lua_setfield(l, -2, "__gc");
    lua_pushcclosure(l, _push_stored_exceptions_what, 0);
    lua_setfield(l, -2, "__tostring");
}

inline void store_current_exception(lua_State * l, char const * what) {
    void * user_data = lua_newuserdata(l, sizeof(stored_exception));
    new(user_data) stored_exception{what, std::current_exception()};

    luaL_getmetatable(l, _stored_exception_metatable_name()->c_str());
    if(lua_isnil(l, -1)) {
        lua_pop(l, 1);
        _register_stored_exception_metatable(l);
    }

    lua_setmetatable(l, -2);
}

inline stored_exception * test_stored_exception(lua_State *l) {
    if(lua_isuserdata(l, -1)) {
        void * user_data = luaL_testudata(l, -1, _stored_exception_metatable_name()->c_str());
        if(user_data != nullptr) {
            return static_cast<stored_exception *>(user_data);
        }
    }
    return nullptr;
}

inline bool push_stored_exceptions_what(lua_State * l) {
    stored_exception * stored = test_stored_exception(l);
    if(stored != nullptr) {
        detail::_push(l, static_cast<const std::string &>(stored->what));
        return true;
    }
    return false;
}

inline std::exception_ptr extract_stored_exception(lua_State *l) {
    stored_exception * stored = test_stored_exception(l);
    if(stored != nullptr) {
        return stored->exception;
    }
    return nullptr;
}

class ExceptionHandler {
public:
    using function = std::function<void(int,std::string,std::exception_ptr)>;

private:
    function _handler;

public:
    ExceptionHandler() = default;

    explicit ExceptionHandler(function && handler) : _handler(handler) {}

    void Handle(int luaStatusCode, std::string message, std::exception_ptr exception = nullptr) {
        if(_handler) {
            _handler(luaStatusCode, std::move(message), std::move(exception));
        }
    }

    void Handle_top_of_stack(int luaStatusCode, lua_State *L) {
        stored_exception * stored = test_stored_exception(L);
        if(stored) {
            Handle(
                luaStatusCode,
                stored->what,
                stored->exception);
        } else {
            Handle(
                luaStatusCode,
                detail::_get(detail::_id<std::string>(), L, -1));
        }
    }

};

}
