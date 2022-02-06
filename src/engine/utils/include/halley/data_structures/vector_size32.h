#pragma once

#include <cassert>
#include <memory>
#include <iterator>
#include <cstdint>
#include <limits>
#include <algorithm>

namespace Halley {
	template <typename T, class Allocator = std::allocator<T>>
	class VectorSize32 {
	public:
		using value_type = T;
		using size_type = uint32_t;
		using difference_type = int32_t;
		using reference = T&;
		using const_reference = const T&;
		using pointer = typename std::allocator_traits<Allocator>::pointer;
		using const_pointer = typename std::allocator_traits<Allocator>::const_pointer;
		constexpr static float growth_factor = 2.0f;

		class const_iterator;
		
		class iterator {
			friend class const_iterator;
		
		public:
		#ifdef __cpp_lib_concepts
		    using iterator_concept = std::contiguous_iterator_tag;
		#endif // __cpp_lib_concepts
		    using iterator_category = std::random_access_iterator_tag;
		    using value_type        = typename VectorSize32::value_type;
		    using difference_type   = typename VectorSize32::difference_type;
		    using pointer           = typename VectorSize32::pointer;
		    using reference         = value_type&;

			iterator() : v(nullptr) {}
			iterator(pointer v) : v(v) {}
			
			reference operator*() const { return *v; }
			iterator& operator++() { ++v; return *this; }
			iterator& operator--() { --v; return *this; }
			iterator operator++(int) const { return iterator(v + 1); }
			iterator operator--(int) const { return iterator(v - 1); }
			iterator operator+(size_t o) const { return iterator(v + o); }
			iterator operator-(size_t o) const { return iterator(v - o); }
			ptrdiff_t operator-(const iterator& other) const { return v - other.v; }

			bool operator==(const iterator& other) { return v == other.v; }
			bool operator!=(const iterator& other) { return v != other.v; }
			bool operator<(const iterator& other) { return v < other.v; }
			bool operator>(const iterator& other) { return v > other.v; }
			bool operator<=(const iterator& other) { return v <= other.v; }
			bool operator>=(const iterator& other) { return v >= other.v; }

			friend void swap(iterator& a, iterator& b) noexcept { std::swap(a.v, b.v); }

		private:
			pointer v;
		};

		class const_iterator {
			friend class VectorSize32;
		
		public:
		#ifdef __cpp_lib_concepts
		    using iterator_concept = std::contiguous_iterator_tag;
		#endif // __cpp_lib_concepts
		    using iterator_category = std::random_access_iterator_tag;
		    using value_type        = typename VectorSize32::value_type;
		    using difference_type   = typename VectorSize32::difference_type;
		    using pointer           = typename VectorSize32::const_pointer;
		    using reference         = const value_type&;

			const_iterator() : v(nullptr) {}
			const_iterator(const_pointer v) : v(v) {}
			const_iterator(iterator v) : v(v.v) {}
			
			const_reference operator*() const { return *v; }
			const_iterator& operator++() { ++v; return *this; }
			const_iterator& operator--() { --v; return *this; }
			const_iterator operator++(int) const { return const_iterator(v + 1); }
			const_iterator operator--(int) const { return const_iterator(v - 1); }
			const_iterator operator+(size_t o) const { return const_iterator(v + o); }
			const_iterator operator-(size_t o) const { return const_iterator(v - o); }
			ptrdiff_t operator-(const const_iterator& o) const { return v - o.v; }

			bool operator==(const const_iterator& other) { return v == other.v; }
			bool operator!=(const const_iterator& other) { return v != other.v; }
			bool operator<(const const_iterator& other) { return v < other.v; }
			bool operator>(const const_iterator& other) { return v > other.v; }
			bool operator<=(const const_iterator& other) { return v <= other.v; }
			bool operator>=(const const_iterator& other) { return v >= other.v; }

			friend void swap(const_iterator& a, const_iterator& b) noexcept { std::swap(a.v, b.v); }

		private:
			const_pointer v;			
		};

		using reverse_iterator = std::reverse_iterator<iterator>;
		using const_reverse_iterator = std::reverse_iterator<const_iterator>;

		VectorSize32()
			: m_data(nullptr)
			, m_size(0)
			, m_capacity(0)
		{
		}
		
		VectorSize32(const VectorSize32& other)
		{
			change_capacity(other.m_capacity);
			m_size = other.m_size;
			for (uint32_t i = 0; i < m_size; ++i) {
				new (pointer_at(i)) T(other[i]);
			}
		}
		
		VectorSize32(VectorSize32&& other) noexcept
			: m_data(other.m_data)
			, m_size(other.m_size)
			, m_capacity(other.m_capacity)
		{
			other.m_data = nullptr;
			other.m_size = 0;
			other.m_capacity = 0;
		}
		
		~VectorSize32()
		{
			clear();
			change_capacity(0);
		}

		VectorSize32& operator=(const VectorSize32& other)
		{
			if (this == &other) {
				return *this;
			}
			
			change_capacity(other.m_capacity);
			resize(other.m_size);
			for (uint32_t i = 0; i < m_size; ++i) {
				(*this)[i] = other[i];
			}
			
			return *this;
		}

		VectorSize32& operator=(VectorSize32&& other) noexcept
		{
			m_data = other.m_data;
			m_size = other.m_size;
			m_capacity = other.m_capacity;
			other.m_data = nullptr;
			other.m_size = 0;
			other.m_capacity = 0;
			return *this;
		}

		[[nodiscard]] pointer data()
		{
			return reinterpret_cast<pointer>(m_data);
		}

		[[nodiscard]] const_pointer data() const
		{
			return reinterpret_cast<const_pointer>(m_data);
		}

		[[nodiscard]] size_t size() const
		{
			return m_size;
		}

		[[nodiscard]] size_t max_size() const
		{
			return std::numeric_limits<size_type>::max();
		}

		[[nodiscard]] bool empty() const
		{
			return m_size == 0;
		}

		[[nodiscard]] size_t capacity() const
		{
			return m_capacity;
		}

		[[nodiscard]] reference operator[](size_t index)
		{
			return data()[index];
		}

		[[nodiscard]] const_reference operator[](size_t index) const
		{
			return data()[index];			
		}

		[[nodiscard]] reference at(size_t index)
		{
			if (index >= m_size) {
				[[unlikely]] throw std::out_of_range("Index out of vector range");
			}
			return data()[index];
		}

		[[nodiscard]] const_reference at(size_t index) const
		{
			if (index >= m_size) {
				[[unlikely]] throw std::out_of_range("Index out of vector range");
			}
			return data()[index];			
		}

		[[nodiscard]] reference front()
		{
			assert(m_size > 0);
			return (*this)[0];
		}

		[[nodiscard]] reference back()
		{
			assert(m_size > 0);
			return (*this)[m_size - 1];
		}

		[[nodiscard]] const_reference front() const
		{
			assert(m_size > 0);
			return (*this)[0];			
		}

		[[nodiscard]] const_reference back() const
		{
			assert(m_size > 0);
			return (*this)[m_size - 1];			
		}

		void resize(size_t size, T defaultValue = T())
		{
			const auto newSize = static_cast<uint32_t>(size);
			if (newSize > m_size) {
				if (newSize > m_capacity) {
					change_capacity(newSize);
				}
				for (size_type i = m_size; i < newSize; ++i) {
					new (pointer_at(i)) T(defaultValue);
				}
				m_size = newSize;
			} else if (newSize < m_size) {
				for (size_type i = newSize; i < m_size; ++i) {
					elem(i).~T();
				}
				m_size = newSize;
			}
		}

		void reserve(size_t size)
		{
			change_capacity(std::max(static_cast<size_type>(size), m_capacity));
		}

		void shrink_to_fit()
		{
			change_capacity(m_size);
		}

		void clear()
		{
			resize(0);
		}

		iterator insert(const_iterator pos, const T& value)
		{
			const auto idx = pos - begin();
			push_back(value);
			std::rotate(pos, end() - 1, end());
			return begin() + idx;
		}

		iterator insert(const_iterator pos, T&& value)
		{
			const auto idx = pos - begin();
			push_back(std::move(value));
			std::rotate(de_const_iter(pos), end() - 1, end());
			return begin() + idx;
		}

		template <class... Args>
		iterator emplace(const_iterator pos, Args&&... args)
		{
			const auto idx = pos - begin();
			emplace_back(std::forward(args));
			std::rotate(de_const_iter(pos), end() - 1, end());
			return begin() + idx;
		}

		iterator erase(const_iterator first, const_iterator last)
		{
			const auto idx = first - begin();
			std::rotate(de_const_iter(first), de_const_iter(last), end());
			resize(m_size - (last - first));
			return begin() + idx;
		}

		iterator erase(const_iterator pos)
		{
			return erase(pos, pos + 1);
		}

		template <class... Args>
		reference emplace_back(Args&&... args)
		{
			const auto idx = m_size;
			construct_with_ensure_capacity(m_size + 1, [&] (std::byte* data)
			{
				new(pointer_at(idx, data)) T(args);			
			});
			++m_size;
			return elem(idx);
		}

		void push_back(const T& value)
		{
			construct_with_ensure_capacity(m_size + 1, [&](std::byte* data)
			{
				new(pointer_at(m_size, data)) T(value);
			});
			++m_size;
		}

		void push_back(T&& value)
		{
			construct_with_ensure_capacity(m_size + 1, [&](std::byte* data)
			{
				new(pointer_at(m_size, data)) T(std::move(value));
			});
			++m_size;
		}

		void pop_back()
		{
			assert(m_size > 0);
			elem(m_size - 1).~T();
			--m_size;
		}

		void swap(VectorSize32& other) noexcept
		{
			std::swap(m_data, other.m_data);
			std::swap(m_size, other.m_size);
			std::swap(m_capacity, other.m_capacity);
		}

		[[nodiscard]] iterator begin()
		{
			return iterator(&elem(0));
		}

		[[nodiscard]] iterator end()
		{
			return iterator(&elem(m_size));
		}

		[[nodiscard]] const_iterator begin() const
		{
			return const_iterator(&elem(0));
		}

		[[nodiscard]] const_iterator end() const
		{
			return const_iterator(&elem(m_size));
		}

		[[nodiscard]] reverse_iterator rbegin()
		{
			return reverse_iterator(end())++;
		}

		[[nodiscard]] reverse_iterator rend()
		{
			return reverse_iterator(begin())++;
		}

		[[nodiscard]] const_reverse_iterator rbegin() const
		{
			return const_reverse_iterator(end())++;
		}

		[[nodiscard]] const_reverse_iterator rend() const
		{
			return const_reverse_iterator(begin())++;
		} 

	private:
		std::byte* m_data;
		size_type m_size;
		size_type m_capacity;

		void change_capacity(size_type newCapacity)
		{
			change_capacity(newCapacity, [](std::byte*) {});
		}

		template <typename F>
		void change_capacity(size_type newCapacity, const F& construct)
		{
			assert(newCapacity >= m_size);
			if (newCapacity != m_capacity) {
				std::byte* newData = newCapacity > 0 ? new std::byte[newCapacity * sizeof(T)] : nullptr;

				construct(newData);
				
				for (size_type i = 0; i < m_size; ++i) {
					new (pointer_at(i, newData)) T(std::move(elem(i)));
					elem(i).~T();
				}
				delete[] m_data;
				m_data = newData;
			}
		}

		template <typename F>
		void construct_with_ensure_capacity(size_type minCapacity, const F& construct)
		{
			if (m_capacity >= minCapacity) {
				construct(m_data);
			} else {
				change_capacity(std::max(minCapacity, static_cast<size_type>(m_capacity * growth_factor)), construct);
			}
		}

		[[nodiscard]] std::byte* pointer_at(size_type pos, std::byte* bytes)
		{
			return bytes + pos * sizeof(T);
		}

		[[nodiscard]] std::byte* pointer_at(size_type pos)
		{
			return m_data + pos * sizeof(T);
		}

		[[nodiscard]] reference elem(size_type pos)
		{
			return data()[pos];
		}

		[[nodiscard]] const_reference elem(size_type pos) const
		{
			return data()[pos];
		}

		[[nodiscard]] iterator de_const_iter(const_iterator iter)
		{
			return iterator(const_cast<pointer>(iter.v));
		}
	};

	template <typename T, class Allocator>
	bool operator==(const VectorSize32<T, Allocator>& a, const VectorSize32<T, Allocator>& b)
	{
		if (a.size() != b.size()) {
			return false;
		}
		for (size_t i = 0; i < a.size(); ++i) {
			if (!(a[i] == b[i])) {
				return false;
			}
		}
		return true;
	}

	template <typename T, class Allocator>
	bool operator!=(const VectorSize32<T, Allocator>& a, const VectorSize32<T, Allocator>& b)
	{
		if (a.size() != b.size()) {
			return true;
		}
		for (size_t i = 0; i < a.size(); ++i) {
			if (a[i] != b[i]) {
				return true;
			}
		}
		return false;
	}

	template<typename T, class Allocator>
	void swap(VectorSize32<T, Allocator>& a, VectorSize32<T, Allocator>& b) noexcept
	{
		a.swap(b);
	}
}
