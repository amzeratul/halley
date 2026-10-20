#pragma once

#include "type_traits.h"
#include "../data_structures/vector.h"
#include "../data_structures/hash_map.h"

namespace Halley
{
    
	namespace Detail {
		template<class T>
		using GetSizeBytes = decltype(std::declval<T>().getSizeBytes());
	}

	template <typename T>
	struct MemoryUsageCalculator {
		static size_t getSizeBytes(const T& v)
		{
			if constexpr (is_detected_v<Detail::GetSizeBytes, T>) {
                return v.getSizeBytes();
            } else {
                return sizeof(T);
            }
		}
	};

	template <typename T>
	struct MemoryUsageCalculator<std::optional<T>> {
		static size_t getSizeBytes(const std::optional<T>& v)
		{
			if (v) {
				return MemoryUsageCalculator<T>::getSizeBytes(*v);
			}
			return 0;
		}
	};

	template <typename T>
	struct MemoryUsageCalculator<std::unique_ptr<T>> {
		static size_t getSizeBytes(const std::unique_ptr<T>& v)
		{
			if (v) {
				return sizeof(T) + MemoryUsageCalculator<T>::getSizeBytes(*v);
			}
			return 0;
		}
	};

	template <typename T>
	struct MemoryUsageCalculator<std::shared_ptr<T>> {
		static size_t getSizeBytes(const std::shared_ptr<T>& v)
		{
			if (v) {
				return sizeof(T) + MemoryUsageCalculator<T>::getSizeBytes(*v);
			}
			return 0;
		}
	};

	template <typename T>
	struct MemoryUsageCalculator<Vector<T>> {
		static size_t getSizeBytes(const Vector<T>& values)
		{
			if constexpr (std::is_trivially_copyable_v<T>) {
				return values.size_bytes();
			} else {
				size_t result = values.size_bytes();
				for (const auto& e: values) {
					result += MemoryUsageCalculator<T>::getSizeBytes(e);
				}
				return result;
			}
		}
	};

	template <typename K, typename V>
	struct MemoryUsageCalculator<HashMap<K, V>> {
		static size_t getSizeBytes(const HashMap<K, V>& values)
		{
			size_t result = values.size_bytes();
			for (const auto& [k, v]: values) {
				result += MemoryUsageCalculator<K>::getSizeBytes(k);
				result += MemoryUsageCalculator<V>::getSizeBytes(v);
			}
			return result;
		}
	};

	template <typename T>
	static size_t getSizeBytesOf(const T& v)
	{
		return MemoryUsageCalculator<T>::getSizeBytes(v);
	}

	class MemoryUsageCounter {
	public:
		size_t total = 0;

		template <typename T>
		MemoryUsageCounter& operator<< (const T& other)
		{
			total += getSizeBytesOf(other);
			return *this;
		}
	};
}
