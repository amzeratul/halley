#pragma once

#include <gsl/gsl>

namespace Halley {
    // Note: it's important that this class has the same layout and binary structure as a plain pointer
    template <typename T>
    class MaybeRef {
    public:
        MaybeRef() : pointer(nullptr) {}
        MaybeRef(T* pointer) : pointer(pointer) {}
        MaybeRef(T& ref) : pointer(&ref) {}

        bool hasValue() const
        {
            return pointer != nullptr;
        }

        T& get()
        {
            Expects(pointer != nullptr);
            return *pointer;
        }

        const T& get() const
        {
            Expects(pointer != nullptr);
            return *pointer;
        }

    private:
        T* pointer;
    };
}
