#pragma once

#include <memory>

namespace Halley {

    template <typename T>
    class ValuePtr {
    public:
        ValuePtr()
            : data(std::make_unique<T>())
        {}

        ValuePtr(const T& v)
            : data(std::make_unique<T>(v))
        {}

        ValuePtr(T&& v)
            : data(std::make_unique<T>(std::move(v)))
        {}

        ValuePtr(const ValuePtr& other)
            : data(std::make_unique<T>(*other.data))
        {
        }

        ValuePtr(ValuePtr&& other)
            : data(std::move(other.data))
        {
        }

        ~ValuePtr() = default;

        ValuePtr& operator=(const ValuePtr& other)
        {
            if (this != &other) [[likely]] {
	            *data = *other.data;
            }
            return *this;
        }

        ValuePtr& operator=(ValuePtr&& other)
        {
            if (this != &other) [[likely]] {
	            data = std::move(other.data);
            }
            return *this;
        }

        const T* operator->() const
        {
            return data.get();
        }

        const T& operator*() const
        {
            return *data;
        }

        T* operator->()
        {
            return data.get();
        }

        T& operator*()
        {
            return *data;
        }

    	bool operator==(const ValuePtr& other) const
        {
            return *data == *other.data;
        }

        bool operator!=(const ValuePtr& other) const
        {
            return *data != *other.data;
        }

        bool operator<=>(const ValuePtr& other) const
        {
            return *data <=> *other.data;
        }

    private:
        std::unique_ptr<T> data;
    };

}