#pragma once

#include "vector.h"

namespace Halley {
	template <typename T>
	class CircularBuffer {
	public:
		CircularBuffer(size_t _size)
			: data(_size)
			, capacity(_size)
			, offset(0)
			, size(0)
		{}

		size_t getCapacity() const { return capacity; }
		size_t getSize() const { return size; }

		T getAverage() const {
			T accum = T();
			for (size_t i = 0; i < size; i++) {
				accum += (*this)[i];
			}
			return accum / size;
		}

		T getAverageOr(const T& def) const {
			if (size > 0) {
				return getAverage();
			}
			else {
				return def;
			}
		}

		T& operator[](size_t i) {
			return data[(i + offset) % capacity];
		}

		const T& operator[](size_t i) const {
			return data[(i + offset) % capacity];
		}

		void add(const T& elem) {
			offset = (offset + capacity - 1) % capacity; // offset moves back, so latest element is always at index 0
			(*this)[0] = elem;
			size = std::min(size + 1, capacity);
		}

	private:
		Vector<T> data;
		size_t capacity;
		size_t offset;
		size_t size;
	};
}
