#pragma once

namespace Halley {
    template <typename T>
    class ConvertibleTo {
    public:
        T value;

        ConvertibleTo()
            : value(T{})
        {}

        template<typename U>
        ConvertibleTo(U u)
            : value(static_cast<T>(u))
        {}
    };
}