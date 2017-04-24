#pragma once

#include "BaseFun.h"
#include <string>

namespace sel {

template <int N, typename T, typename Ret, typename... Args>
class ClassFun : public BaseFun {
private:
    using _fun_type = std::function<Ret(T*, Args...)>;
    _fun_type _fun;
    std::string _name;
    std::string _metatable_name;

    T *_get(lua_State *state) {
        T *ret = (T *)luaL_checkudata(state, 1, _metatable_name.c_str());
        lua_remove(state, 1);
        return ret;
    }

public:
    ClassFun(lua_State *l,
             const std::string &name,
             const std::string &metatable_name,
             Ret(*fun)(Args...))
        : ClassFun(l, name, _fun_type{fun}) {}

    ClassFun(lua_State *l,
             const std::string &name,
             const std::string &metatable_name,
             _fun_type fun)
        : _fun(fun), _name(name), _metatable_name(metatable_name) {
        lua_pushlightuserdata(l, (void *)static_cast<BaseFun *>(this));
        lua_pushcclosure(l, &detail::_lua_dispatcher, 1);
        lua_setfield(l, -2, name.c_str());
    }

    int Apply(lua_State *l) {
        std::tuple<T*> t = std::make_tuple(_get(l));
        std::tuple<Args...> args = detail::_get_args<Args...>(l);
        std::tuple<T*, Args...> pack = std::tuple_cat(t, args);
        detail::_push(l, detail::_lift(_fun, pack));
        return N;
    }
};

template <typename T, typename... Args>
class ClassFun<0, T, void, Args...> : public BaseFun {
private:
    using _fun_type = std::function<void(T*, Args...)>;
    _fun_type _fun;
    std::string _name;
    std::string _metatable_name;

    T *_get(lua_State *state) {
        T *ret = (T *)luaL_checkudata(state, 1, _metatable_name.c_str());
        lua_remove(state, 1);
        return ret;
    }

public:
    ClassFun(lua_State *l,
             const std::string &name,
             const std::string &metatable_name,
             void(*fun)(Args...))
        : ClassFun(l, name, metatable_name, _fun_type{fun}) {}

    ClassFun(lua_State *l,
             const std::string &name,
             const std::string &metatable_name,
             _fun_type fun)
        : _fun(fun), _name(name), _metatable_name(metatable_name) {
        lua_pushlightuserdata(l, (void *)static_cast<BaseFun *>(this));
        lua_pushcclosure(l, &detail::_lua_dispatcher, 1);
        lua_setfield(l, -2, name.c_str());
    }

    int Apply(lua_State *l) {
        std::tuple<T*> t = std::make_tuple(_get(l));
        std::tuple<Args...> args = detail::_get_args<Args...>(l);
        std::tuple<T*, Args...> pack = std::tuple_cat(t, args);
        detail::_lift(_fun, pack);
        return 0;
    }
};
}
