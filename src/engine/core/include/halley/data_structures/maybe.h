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

	namespace Detail {
		template<typename T, bool = std::is_enum_v<T>>
		struct underlying_type {
			using type = T;
		};

		template<typename T>
		struct underlying_type<T, true> : std::underlying_type<T> {
		};
	}

	template <typename T>
	class OptionalLite {
		using StorageType = std::conditional_t<std::is_enum_v<T>, typename Detail::underlying_type<T>::type, T>;

	public:
		constexpr OptionalLite()
			: val(getDefaultValue())
		{}

		constexpr OptionalLite(T value)
			: val(StorageType(value))
		{}

		constexpr OptionalLite(std::nullptr_t)
			: val(getDefaultValue())
		{}

		constexpr OptionalLite(std::nullopt_t)
			: val(getDefaultValue())
		{}

		constexpr OptionalLite(std::optional<T> value)
			: val(value ? StorageType(*value) : getDefaultValue())
		{
		}

		[[maybe_unused]] constexpr OptionalLite& operator=(T v)
		{
			val = static_cast<StorageType>(v);
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
			HalleyAssertDev(has_value());
			return reinterpret_cast<const T&>(val);
		}

		[[nodiscard]] constexpr T& value()
		{
			HalleyAssertDev(has_value());
			return reinterpret_cast<T&>(val);
		}

		[[nodiscard]] constexpr T value_or(T def) const
		{
			if (has_value()) {
				return value();
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
			return has_value() && value() == other;
		}

		[[nodiscard]] constexpr bool operator!=(const OptionalLite& other) const
		{
			return val != other.val;
		}

		[[nodiscard]] constexpr bool operator!=(const T& other) const
		{
			return !has_value() || value() != other;
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

		[[nodiscard]] constexpr std::optional<T> to_optional() const
		{
			return has_value() ? std::optional<T>(value()) : std::nullopt;
		}

		void reset()
		{
			val = getDefaultValue();
		}

	private:
		StorageType val;

		constexpr static StorageType getDefaultValue()
		{
			static_assert(std::is_integral_v<StorageType> || std::is_floating_point_v<StorageType>);
			
			if constexpr (std::is_integral_v<StorageType>) {
				if constexpr (std::numeric_limits<StorageType>::is_signed) {
					return std::numeric_limits<StorageType>::min();
				} else {
					return std::numeric_limits<StorageType>::max();
				}
			} else if constexpr (std::is_floating_point_v<StorageType>) {
				return std::numeric_limits<StorageType>::quiet_NaN();
			} else {
				return StorageType();
			}
		}

		constexpr static bool isDefaultValue(StorageType value)
		{
			if constexpr (std::is_floating_point_v<StorageType>) {
				return std::isnan(value);
			} else {
				return value == getDefaultValue();
			}
		}
	};
}
