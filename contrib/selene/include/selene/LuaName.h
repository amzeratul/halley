#pragma once

#include <string>

extern "C" {
#include <lua.h>
}

namespace sel {
/*
 * Used to ensure that globals set in Lua are deleted (set to nil) in
 * the event that the parent object is destroyed. Checks if the Lua
 * context in which it was registered still exists before doing so.
 * Prevents copying but permits moving.
 */
class LuaName {
private:
    lua_State **_l;
    std::string _name;

public:
    LuaName(lua_State *&l, const std::string &name) : _l(&l), _name(name) {}
    LuaName(const LuaName &other) = delete;
    LuaName(LuaName &&other)
        : _l(other._l),
          _name(other._name) {
        *other._l = nullptr;
    }
    ~LuaName() {
        if (_l != nullptr && *_l != nullptr) {
            lua_pushnil(*_l);
            lua_setglobal(*_l, _name.c_str());
        }
    }

    void Register() {
        if (_l != nullptr && *_l != nullptr) {
            lua_setglobal(*_l, _name.c_str());
        }
    }

    std::string GetName() const {return _name;}
    lua_State *GetState() {return *_l;}
};
}
