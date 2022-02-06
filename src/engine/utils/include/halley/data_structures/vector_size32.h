#pragma once

#include <cassert>
#include <memory>
#include <iterator>
#include <cstdint>
#include <limits>

namespace Halley {
	template <typename T, typename Allocator = std::allocator<T>>
	class VectorSize32 {
	public:
		using value_type = T;
		using size_type = uint32_t;
		using difference_type = int32_t;
		using reference = T&;
		using const_reference = const T&;
		using pointer = typename std::allocator_traits<Allocator>::pointer;
		using const_pointer = typename std::allocator_traits<Allocator>::const_pointer;

		class iterator {
		public:
			iterator() : v(nullptr) {}
			iterator(pointer v) : v(v) {}
			
			reference operator*() const { return *v; }
			iterator& operator++() const { ++v; return *this; }
			iterator operator++(int) const { auto r = *this; ++v; return r; }
			iterator& operator--() const { --v; return *this; }
			iterator operator--(int) const { auto r = *this; --v; return r; }

		private:
			pointer v;
		};

		class const_iterator {
		public:
			const_iterator() : v(nullptr) {}
			const_iterator(const_pointer v) : v(v) {}
			
			const_reference operator*() const { return *v; }
			const_iterator& operator++() const { ++v; return *this; }
			const_iterator operator++(int) const { auto r = *this; ++v; return r; }
			const_iterator& operator--() const { --v; return *this; }
			const_iterator operator--(int) const { auto r = *this; --v; return r; }

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
			// TODO
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
			delete[] m_data;
		}

		VectorSize32& operator=(const VectorSize32& other)
		{
			// TODO
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

		pointer data()
		{
			return m_data;
		}

		const_pointer data() const
		{
			return m_data;
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
			return m_data[index];
		}

		[[nodiscard]] const_reference operator[](size_t index) const
		{
			return m_data[index];			
		}

		[[nodiscard]] reference at(size_t index)
		{
			if (index >= m_size) {
				[[unlikely]] throw std::out_of_range("Index out of vector range");
			}
			return m_data[index];
		}

		[[nodiscard]] const_reference at(size_t index) const
		{
			if (index >= m_size) {
				[[unlikely]] throw std::out_of_range("Index out of vector range");
			}
			return m_data[index];			
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

		void resize(size_t size)
		{
			// TODO
		}

		void reserve(size_t size)
		{
			// TODO
		}

		void shrink_to_fit()
		{
			// TODO
		}

		void clear()
		{
			// TODO
		}

		iterator insert(const_iterator pos, const T& value)
		{
			// TODO
		}

		iterator insert(const_iterator pos, T&& value)
		{
			// TODO
		}

		iterator erase(const_iterator pos)
		{
			// TODO
		}

		template <class... Args>
		iterator emplace(const_iterator pos, Args&&... args)
		{
			// TODO
		}

		template <class... Args>
		reference emplace_back(Args&&... args)
		{
			// TODO
		}

		void push_back(const T& value)
		{
			// TODO
		}

		void push_back(T&& value)
		{
			// TODO
		}

		void pop_back()
		{
			// TODO
		}

		void swap(VectorSize32& other) noexcept
		{
			std::swap(m_data, other.m_data);
			std::swap(m_size, other.m_size);
			std::swap(m_capacity, other.m_capacity);
		}

		iterator begin()
		{
			return iterator(&data()[0]);
		}

		iterator end()
		{
			return iterator(&data()[m_size]);
		}

		const_iterator begin() const
		{
			return const_iterator(&data()[0]);
		}

		const_iterator end() const
		{
			return const_iterator(&data()[m_size]);
		}

		reverse_iterator rbegin()
		{
			return reverse_iterator(end())++;
		}

		reverse_iterator rend()
		{
			return reverse_iterator(begin())++;
		}

		const_reverse_iterator rbegin() const
		{
			return const_reverse_iterator(end())++;
		}

		const_reverse_iterator rend() const
		{
			return const_reverse_iterator(begin())++;
		} 

	private:
		pointer m_data;
		size_type m_size;
		size_type m_capacity;
	};
}