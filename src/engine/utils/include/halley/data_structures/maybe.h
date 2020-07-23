#pragma once

#include <optional>
#include <limits>
#include <type_traits>
#include <cmath>

namespace Halley
{
	template <typename T>
	using Maybe [[deprecated]] = std::optional<T>;

	template <typename T>
	using MaybeRefWrap = std::optional<std::reference_wrapper<T>>;

	template <typename T>
	class OptionalLite {
	public:
		constexpr OptionalLite()
			: val(getDefaultValue())
		{}

		constexpr OptionalLite(T value)
			: val(value)
		{}

		constexpr OptionalLite(std::nullptr_t)
			: val(getDefaultValue())
		{}

		[[maybe_unused]] constexpr OptionalLite& operator=(T v)
		{
			val = v;
			return *this;
		}

		[[maybe_unused]] constexpr OptionalLite& operator=(std::nullptr_t)
		{
			val = getDefaultValue();
			return *this;
		}

		[[maybe_unused]] constexpr OptionalLite& operator=(const OptionalLite& other) noexcept
		{
			val = other.val;
			return *this;
		}

		[[nodiscard]] constexpr const T& value() const
		{
			Expects(has_value());
			return val;
		}

		[[nodiscard]] constexpr T& value()
		{
			Expects(has_value());
			return val;
		}

		[[nodiscard]] constexpr T value_or(T def) const
		{
			if (has_value()) {
				return val;
			} else {
				return def;
			}
		}

		[[nodiscard]] constexpr bool has_value() const
		{
			return !isDefaultValue(val);
		}

		[[nodiscard]] constexpr operator bool() const
		{
			return has_value();
		}

		constexpr operator int() const = delete;

		[[nodiscard]] constexpr const T* operator->() const
		{
			return &value();
		}

		[[nodiscard]] constexpr T* operator->()
		{
			return &value();
		}

		[[nodiscard]] constexpr const T& operator*() const
		{
			return value();
		}

		[[nodiscard]] constexpr T& operator*()
		{
			return value();
		}

		[[nodiscard]] constexpr bool operator==(const OptionalLite& other) const
		{
			return val == other.val;
		}

		[[nodiscard]] constexpr bool operator==(const T& other) const
		{
			return val == other;
		}

		[[nodiscard]] constexpr bool operator!=(const OptionalLite& other) const
		{
			return val != other.val;
		}

		[[nodiscard]] constexpr bool operator!=(const T& other) const
		{
			return val != other;
		}

		[[nodiscard]] constexpr bool operator<(const OptionalLite& other) const
		{
			if (has_value() == other.has_value()) {
				return val < other.val;
			} else {
				return !has_value();
			}
		}

		[[nodiscard]] constexpr bool operator>(const OptionalLite& other) const
		{
			if (has_value() == other.has_value()) {
				return val > other.val;
			} else {
				return has_value();
			}
		}

		[[nodiscard]] constexpr bool operator<=(const OptionalLite& other) const
		{
			if (has_value() == other.has_value()) {
				return val <= other.val;
			} else {
				return !has_value();
			}
		}

		[[nodiscard]] constexpr bool operator>=(const OptionalLite& other) const
		{
			if (has_value() == other.has_value()) {
				return val >= other.val;
			} else {
				return has_value();
			}
		}

	private:
		T val;

		constexpr static T getDefaultValue()
		{
			static_assert(std::is_integral_v<T> || std::is_floating_point_v<T>);
			
			if constexpr (std::is_integral_v<T>) {
				if constexpr (std::numeric_limits<T>::is_signed) {
					return std::numeric_limits<T>::min();
				} else {
					return std::numeric_limits<T>::max();
				}
			} else if constexpr (std::is_floating_point_v<T>) {
				return std::numeric_limits<T>::quiet_NaN();
			} else {
				return T();
			}
		}

		constexpr static bool isDefaultValue(T value)
		{
			if constexpr (std::is_floating_point_v<T>) {
				return std::isnan(value);
			} else {
				return value == getDefaultValue();
			}
		}
	};
}
