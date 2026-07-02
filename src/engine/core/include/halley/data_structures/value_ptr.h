#pragma once

#include <memory>

namespace Halley {

    template <typename T>
    class ValuePtr {
    public:
        ValuePtr()
            : data(std::make_unique<T>())
        {}

        ValuePtr(std::nullopt_t)
            : data()
        {}

        ValuePtr(const T& v)
            : data(std::make_unique<T>(v))
        {}

        ValuePtr(T&& v)
            : data(std::make_unique<T>(std::move(v)))
        {}

        ValuePtr(const ValuePtr& other)
            : data(other.data ? std::make_unique<T>(*other.data) : std::unique_ptr<T>())
        {
        }

        ValuePtr(ValuePtr&& other) noexcept
	        : data(std::move(other.data))
        {
        }

        ~ValuePtr() = default;

        ValuePtr& operator=(const ValuePtr& other)
        {
            if (this != &other) [[likely]] {
                if (other.data) {
		            *data = *other.data;
                } else {
	                data = {};
                }
            }
            return *this;
        }

        ValuePtr& operator=(ValuePtr&& other) noexcept
        {
            if (this != &other) [[likely]] {
	            data = std::move(other.data);
            }
            return *this;
        }

        ValuePtr& operator=(std::nullopt_t)
        {
            data = {};
            return *this;
        }

        ValuePtr& operator=(const T& other)
        {
            if (!data) {
	            data = std::make_unique<T>(other);
            } else {
	            *data = other;
            }
            return *this;
        }

        ValuePtr& operator=(T&& other)
        {
            if (!data) {
	            data = std::make_unique<T>(std::move(other));
            } else {
	            *data = std::move(other);
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

        operator bool() const
        {
	        return !!data;
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
            return data == other.data && (!data || *data == *other.data);
        }

        bool operator!=(const ValuePtr& other) const
        {
            return data != other.data || (data && *data != *other.data);
        }

        const T* get() const
        {
	        return data.get();
        }

    private:
        std::unique_ptr<T> data;
    };

}
