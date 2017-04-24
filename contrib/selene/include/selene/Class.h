#pragma once

#include "ClassFun.h"
#include "Ctor.h"
#include "Dtor.h"
#include "MetatableRegistry.h"
#include <map>
#include <memory>
#include "util.h"
#include <vector>
#include <stack>

namespace sel {

struct BaseClass {
    virtual ~BaseClass() {}
};


template <typename T,
          typename A,
          typename... Members>
class Class : public BaseClass {
private:
    std::string _name;
    std::string _metatable_name;
    std::unique_ptr<A> _ctor;
    std::unique_ptr<Dtor<T>> _dtor;
    using Funs = std::vector<std::unique_ptr<BaseFun>>;
    Funs _funs;

    void _register_ctor(lua_State *state) {
        _ctor.reset(new A(state, _metatable_name.c_str()));
    }

    void _register_dtor(lua_State *state) {
        _dtor.reset(new Dtor<T>(state, _metatable_name.c_str()));
    }

    template <typename M>
    void _register_member(lua_State *state,
                          const char *member_name,
                          M T::*member) {
        _register_member(state, member_name, member,
                         typename std::is_const<M>::type{});
    }

    template <typename M>
    void _register_member(lua_State *state,
                          const char *member_name,
                          M T::*member,
                          std::false_type) {
        std::function<M(T*)> lambda_get = [member](T *t) {
            return t->*member;
        };
        _funs.emplace_back(
            sel::make_unique<ClassFun<1, T, M>>(
                state, std::string{member_name},
                _metatable_name.c_str(), lambda_get));

        std::function<void(T*, M)> lambda_set = [member](T *t, M value) {
            (t->*member) = value;
        };
        _funs.emplace_back(
            sel::make_unique<ClassFun<0, T, void, M>>(
                state, std::string("set_") + member_name,
                _metatable_name.c_str(), lambda_set));
    }

    template <typename M>
    void _register_member(lua_State *state,
                          const char *member_name,
                          M T::*member,
                          std::true_type) {
        std::function<M(T*)> lambda_get = [member](T *t) {
            return t->*member;
        };
        _funs.emplace_back(
            sel::make_unique<ClassFun<1, T, M>>(
                state, std::string{member_name},
                _metatable_name.c_str(), lambda_get));
    }

    template <typename Ret, typename... Args>
    void _register_member(lua_State *state,
                          const char *fun_name,
                          Ret(T::*fun)(Args&&...)) {
        std::function<Ret(T*, Args&&...)> lambda = [fun](T *t, Args&&... args) -> Ret {
            return (t->*fun)(std::forward<Args>(args)...);
        };
        constexpr int arity = detail::_arity<Ret>::value;
        _funs.emplace_back(
            sel::make_unique<ClassFun<arity, T, Ret, Args...>>(
                state, std::string(fun_name),
                _metatable_name.c_str(), lambda));
    }

    template <typename Ret, typename... Args>
    void _register_member(lua_State *state,
                          const char *fun_name,
                          Ret(T::*fun)(Args...)) {
        std::function<Ret(T*, Args...)> lambda = [fun](T *t, Args... args) {
            return (t->*fun)(args...);
        };
        constexpr int arity = detail::_arity<Ret>::value;
        _funs.emplace_back(
            sel::make_unique<ClassFun<arity, T, Ret, Args...>>(
                state, std::string(fun_name),
                _metatable_name.c_str(), lambda));
    }

    template <typename Ret, typename... Args>
    void _register_member(lua_State *state,
                          const char *fun_name,
                          Ret(T::*fun)(Args...) const) {
        std::function<Ret(const T*, Args...)> lambda =
            [fun](const T *t, Args... args) {
                return (t->*fun)(args...);
            };
        constexpr int arity = detail::_arity<Ret>::value;
        _funs.emplace_back(
            sel::make_unique<ClassFun<arity, const T, Ret, Args...>>(
                state, std::string(fun_name),
                _metatable_name.c_str(), lambda));
    }

    void _register_members(lua_State *state) {}

    template <typename M, typename... Ms>
    void _register_members(lua_State *state,
                           const char *name,
                           M member,
                           Ms... members) {
        _register_member(state, name, member);
        _register_members(state, members...);
    }

public:
    Class(lua_State *state,
          const std::string &name,
          Members... members) : _name(name) {
        _metatable_name = _name + "_lib";
        MetatableRegistry::PushNewMetatable(state, typeid(T), _metatable_name);
        _register_dtor(state);
        _register_ctor(state);
        _register_members(state, members...);
        lua_pushvalue(state, -1);
        lua_setfield(state, -1, "__index");
    }
    ~Class() = default;
    Class(const Class &) = delete;
    Class& operator=(const Class &) = delete;
    Class(Class &&other) = default;
    Class& operator=(Class &&other) = default;
};
}
