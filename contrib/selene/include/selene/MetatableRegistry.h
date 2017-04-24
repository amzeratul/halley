#pragma once
#include <iostream>
#include <memory>
#include <typeinfo>
#include <unordered_map>

extern "C" {
#include <lua.h>
#include <lauxlib.h>
}

namespace sel {

namespace detail {
struct GetUserdataParameterFromLuaTypeError {
    std::string metatable_name;
    int index;
};
}

namespace MetatableRegistry {
using TypeID = std::reference_wrapper<const std::type_info>;
namespace detail {

static inline void _create_table_in_registry(lua_State *state, const std::string & name) {
    lua_pushlstring(state, name.c_str(), name.size());
    lua_newtable(state);
    lua_settable(state, LUA_REGISTRYINDEX);
}

static inline void _push_names_table(lua_State *state) {
    lua_pushliteral(state, "selene_metatable_names");
    lua_gettable(state, LUA_REGISTRYINDEX);
}

static inline void _push_meta_table(lua_State *state) {
    lua_pushliteral(state, "selene_metatables");
    lua_gettable(state, LUA_REGISTRYINDEX);
}

static inline void _push_typeinfo(lua_State *state, TypeID type) {
    lua_pushlightuserdata(state, const_cast<std::type_info*>(&type.get()));
}

static inline void _get_metatable(lua_State *state, TypeID type) {
    detail::_push_meta_table(state);
    detail::_push_typeinfo(state, type);
    lua_gettable(state, -2);
    lua_remove(state, -2);
}

}

static inline void Create(lua_State *state) {
    detail::_create_table_in_registry(state, "selene_metatable_names");
    detail::_create_table_in_registry(state, "selene_metatables");
}

static inline void PushNewMetatable(lua_State *state, TypeID type, const std::string& name) {
    detail::_push_names_table(state);

    detail::_push_typeinfo(state, type);
    lua_pushlstring(state, name.c_str(), name.size());
    lua_settable(state, -3);

    lua_pop(state, 1);


    luaL_newmetatable(state, name.c_str()); // Actual result.


    detail::_push_meta_table(state);

    detail::_push_typeinfo(state, type);
    lua_pushvalue(state, -3);
    lua_settable(state, -3);

    lua_pop(state, 1);
}

static inline bool SetMetatable(lua_State *state, TypeID type) {
    detail::_get_metatable(state, type);

    if(lua_istable(state, -1)) {
        lua_setmetatable(state, -2);
        return true;
    }

    lua_pop(state, 1);
    return false;
}

static inline bool IsRegisteredType(lua_State *state, TypeID type) {
    detail::_push_names_table(state);
    detail::_push_typeinfo(state, type);
    lua_gettable(state, -2);

    bool registered = lua_isstring(state, -1);
    lua_pop(state, 2);
    return registered;
}

static inline std::string GetTypeName(lua_State *state, TypeID type) {
    std::string name("unregistered type");

    detail::_push_names_table(state);
    detail::_push_typeinfo(state, type);
    lua_gettable(state, -2);

    if(lua_isstring(state, -1)) {
        size_t len = 0;
        char const * str = lua_tolstring(state, -1, &len);
        name.assign(str, len);
    }

    lua_pop(state, 2);
    return name;
}

static inline std::string GetTypeName(lua_State *state, int index) {
    std::string name;

    if(lua_getmetatable(state, index)) {
        lua_pushliteral(state, "__name");
        lua_gettable(state, -2);

        if(lua_isstring(state, -1)) {
            size_t len = 0;
            char const * str = lua_tolstring(state, -1, &len);
            name.assign(str, len);
        }

        lua_pop(state, 2);
    }

    if(name.empty()) {
        name = lua_typename(state, lua_type(state, index));
    }

    return name;
}

static inline bool IsType(lua_State *state, TypeID type, const int index) {
    bool equal = true;

    if(lua_getmetatable(state, index)) {
        detail::_get_metatable(state, type);
        equal = lua_istable(state, -1) && lua_rawequal(state, -1, -2);
        lua_pop(state, 2);
    } else {
        detail::_get_metatable(state, type);
        equal = !lua_istable(state, -1);
        lua_pop(state, 1);
    }

    return equal;
}

static inline void CheckType(lua_State *state, TypeID type, const int index) {
    if(!IsType(state, type, index)) {
        throw sel::detail::GetUserdataParameterFromLuaTypeError{
            GetTypeName(state, type),
            index
        };
    }
}

}

}
