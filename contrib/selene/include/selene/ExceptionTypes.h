#pragma once

#include <exception>
#include <string>
#include <utility>

namespace sel {

class SeleneException : public std::exception {};

class TypeError : public SeleneException {
    std::string _message;
public:
    explicit TypeError(std::string expected)
      : _message(std::move(expected)
                 + " expected, got no object.") {}
    explicit TypeError(std::string expected, std::string const & actual)
      : _message(std::move(expected)
                 + " expected, got " + actual + '.') {}
    char const * what() const noexcept override {
        return _message.c_str();
    }
};

class CopyUnregisteredType : public SeleneException {
public:
    using TypeID = std::reference_wrapper<const std::type_info>;
    explicit CopyUnregisteredType(TypeID type) : _type(type) {}

    TypeID getType() const
    {
        return _type;
    }
    char const * what() const noexcept override {
        return "Tried to copy an object of an unregistered type. "
               "Please register classes before passing instances by value.";
    }
private:
    TypeID _type;
};
}
