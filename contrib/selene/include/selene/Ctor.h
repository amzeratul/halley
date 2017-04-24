#pragma once

#include "BaseFun.h"

namespace sel {

template <typename T, typename... Args>
class Ctor : public BaseFun {
private:
    using _ctor_type = std::function<void(lua_State *, Args...)>;
    _ctor_type _ctor;

public:
    Ctor(lua_State *l,
         const std::string &metatable_name)
         : _ctor([metatable_name](lua_State *state, Args... args) {
             void *addr = lua_newuserdata(state, sizeof(T));
             new(addr) T(args...);
             luaL_setmetatable(state, metatable_name.c_str());
           }) {
        lua_pushlightuserdata(l, (void *)static_cast<BaseFun *>(this));
        lua_pushcclosure(l, &detail::_lua_dispatcher, 1);
        lua_setfield(l, -2, "new");
    }

    int Apply(lua_State *l) {
        std::tuple<Args...> args = detail::_get_args<Args...>(l);
        auto pack = std::tuple_cat(std::make_tuple(l), args);
        detail::_lift(_ctor, pack);
        // The constructor will leave a single userdata entry on the stack
        return 1;
    }
};
}
