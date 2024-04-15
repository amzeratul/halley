#pragma once

#include <cassert>
#include <memory>
#include <iterator>
#include <cstdint>
#include <limits>
#include <algorithm>
#include <stdexcept>
#include <gsl/span>
#include <string_view>

#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

namespace Halley {
	namespace HalleyExceptions {
		[[noreturn]] void throwException(std::string_view msg, int code);
	}

	template <typename T, typename Pointer>
	class VectorIterator {
	public:
	#ifdef __cpp_lib_concepts
	    using iterator_concept = std::contiguous_iterator_tag;
	#endif // __cpp_lib_concepts
	    using iterator_category = std::random_access_iterator_tag;
	    using value_type        = T;
	    using difference_type   = std::ptrdiff_t;
	    using pointer           = Pointer;
	    using reference         = decltype(*Pointer());

		VectorIterator() : v(nullptr) {}
		VectorIterator(pointer v) : v(v) {}

		template<typename OtherPointer>
		VectorIterator(const VectorIterator<T, OtherPointer>& o) : v(o.v) {}
		
		reference operator*() const { return *v; }
		pointer operator->() const { return v; }
		reference operator[](size_t n) const { return v[n]; }
		
		VectorIterator& operator++() { ++v; return *this; }
		VectorIterator& operator--() { --v; return *this; }
		VectorIterator operator++(int) const { return VectorIterator(v + 1); }
		VectorIterator operator--(int) const { return VectorIterator(v - 1); }
		VectorIterator operator+(ptrdiff_t o) const { return VectorIterator(v + o); }
		VectorIterator operator-(ptrdiff_t o) const { return VectorIterator(v - o); }
		VectorIterator operator+=(ptrdiff_t o) { v += o; return *this; }
		VectorIterator operator-=(ptrdiff_t o) { v -= o; return *this; }
		
		template<typename OtherPointer>
		ptrdiff_t operator-(const VectorIterator<T, OtherPointer>& other) const { return v - other.v; }

		template<typename OtherPointer>
		bool operator==(const VectorIterator<T, OtherPointer>& other) const { return v == other.v; }

		template<typename OtherPointer>
		bool operator!=(const VectorIterator<T, OtherPointer>& other) const { return v != other.v; }

		template<typename OtherPointer>
		bool operator<(const VectorIterator<T, OtherPointer>& other) const { return v < other.v; }

		template<typename OtherPointer>
		bool operator>(const VectorIterator<T, OtherPointer>& other) const { return v > other.v; }

		template<typename OtherPointer>
		bool operator<=(const VectorIterator<T, OtherPointer>& other) const { return v <= other.v; }

		template<typename OtherPointer>
		bool operator>=(const VectorIterator<T, OtherPointer>& other) const { return v >= other.v; }

		friend void swap(VectorIterator& a, VectorIterator& b) noexcept { std::swap(a.v, b.v); }

		pointer v;
	};

	template <class T, class = void>
	constexpr bool is_iterator_v = false;

	template <class T>
	constexpr bool is_iterator_v<T, std::void_t<typename std::iterator_traits<T>::iterator_category>> = true;

	namespace VectorDetail {
		template <typename T>
		[[nodiscard]] constexpr T alignUp(T val, T align)
		{
			return val + (align - (val % align)) % align;
		}
	}

	template <typename T, typename SizeType = uint32_t, bool EnableSBO = false, size_t SBOPadding = 0, class Allocator = std::allocator<T>>
	class VectorStd : Allocator {
	public:
		using value_type = T;
		using size_type = SizeType;
		using difference_type = typename std::make_signed<SizeType>::type;
		using reference = T&;
		using const_reference = const T&;
		using pointer = typename std::allocator_traits<Allocator>::pointer;
		using const_pointer = typename std::allocator_traits<Allocator>::const_pointer;

		using iterator = VectorIterator<T, T*>;
		using const_iterator = VectorIterator<T, const T*>;
		using reverse_iterator = std::reverse_iterator<iterator>;
		using const_reverse_iterator = std::reverse_iterator<const_iterator>;

		constexpr static float growth_factor = 2.0f; // When resizing, what number to scale by

		VectorStd() noexcept = default;

		explicit VectorStd(const Allocator& allocator) noexcept
			: Allocator(allocator)
		{
		}

		VectorStd(size_t count, T defaultValue, const Allocator& alloc = Allocator())
			: Allocator(alloc)
		{
			resize(count, std::move(defaultValue));
		}

		explicit VectorStd(size_t count, const Allocator& alloc = Allocator())
			: Allocator(alloc)
		{
			resize(count);
		}

		template <class InputIt, std::enable_if_t<is_iterator_v<InputIt>, int> Test = 0>
		VectorStd(InputIt first, InputIt last, const Allocator& alloc = Allocator())
			: Allocator(alloc)
		{
			assign(std::move(first), std::move(last));
		}
		
		VectorStd(const VectorStd& other)
		{
			change_capacity(other.st_capacity());
			auto* dst = data();
			const auto iterEnd = other.end();
			for (auto iter = other.begin(); iter != iterEnd; ++iter) {
				std::allocator_traits<Allocator>::construct(*this, dst++, *iter);
			}
			set_size(other.st_size());
		}
		
		VectorStd(const VectorStd& other, const Allocator& alloc)
			: Allocator(alloc)
		{
			change_capacity(other.st_capacity());
			auto* dst = data();
			const auto iterEnd = other.end();
			for (auto iter = other.begin(); iter != iterEnd; ++iter) {
				std::allocator_traits<Allocator>::construct(*this, dst++, *iter);
			}
			set_size(other.st_size());
		}
		
		VectorStd(VectorStd&& other) noexcept
			: Allocator(std::move(other))
		{
			move_data_from(other);
		}

		VectorStd(VectorStd&& other, const Allocator& alloc)
			: Allocator(alloc)
		{
			move_data_from(other);
		}

		VectorStd(std::initializer_list<T> list, const Allocator& alloc = Allocator())
			: Allocator(alloc)
		{
			reserve(list.size());
			for (const auto& e: list) {
				push_back(T(e));
			}
		}
		
		~VectorStd() noexcept
		{
			destroy();
		}

		VectorStd& operator=(const VectorStd& other)
		{
			if (this == &other) {
				return *this;
			}

			assign(other.begin(), other.end());
			
			return *this;
		}

		template<typename A, typename S, bool SBO, size_t SBOP>
		VectorStd& operator=(const VectorStd<T, S, SBO, SBOP, A>& other)
		{
			if constexpr (std::is_same_v<decltype(this), decltype(&other)>) {
				if (this == &other) {
					return *this;
				}
			}

			assign(other.begin(), other.end());
			
			return *this;
		}

		VectorStd& operator=(VectorStd&& other) noexcept
		{
			if (this == &other) {
				return *this;
			}

			destroy();
			Allocator::operator=(std::move(other));
			move_data_from(other);
			return *this;
		}

		VectorStd& operator=(std::initializer_list<T> list)
		{
			assign(list.begin(), list.end());
			return *this;
		}

		void assign(size_t count, const T& value)
		{
			clear();
			resize(count, value);
		}

		template <class InputIt, std::enable_if_t<is_iterator_v<InputIt>, int> Test = 0>
		void assign(InputIt begin, InputIt end)
		{
			clear();

			if constexpr (std::is_same_v<typename std::iterator_traits<InputIt>::iterator_category, std::random_access_iterator_tag>) {
				// RandomAccessIterator
				const auto sz = static_cast<size_t>(end - begin);
				reserve(sz);
				auto* dst = data();

				for (auto iter = begin; iter != end; ++iter) {
					std::allocator_traits<Allocator>::construct(as_allocator(), dst, *iter);
					++dst;
				}
				set_size(static_cast<size_type>(sz));
			} else {
				// Non-Random Access Iterator
				for (auto iter = begin; iter != end; ++iter) {
					push_back(*iter);
				}
			}
		}

		void assign(std::initializer_list<T> list)
		{
			assign(list.begin(), list.end());
		}

		[[nodiscard]] Allocator get_allocator() const noexcept
		{
			return *this;
		}

		[[nodiscard]] constexpr pointer data()
		{
			return sbo_active() ? sbo_data() : m_data;
		}

		[[nodiscard]] constexpr const_pointer data() const
		{
			return sbo_active() ? sbo_data() : m_data;
		}

		[[nodiscard]] constexpr size_t size() const
		{
			return st_size();
		}

		[[nodiscard]] constexpr size_t max_size() const
		{
			return std::numeric_limits<size_type>::max() / 2;
		}

		[[nodiscard]] constexpr bool empty() const
		{
			return st_size() == 0;
		}

		[[nodiscard]] constexpr size_t capacity() const
		{
			return st_capacity();
		}

		[[nodiscard]] constexpr reference operator[](size_t index)
		{
			return data()[index];
		}

		[[nodiscard]] constexpr const_reference operator[](size_t index) const
		{
			return data()[index];			
		}

		[[nodiscard]] reference at(size_t index)
		{
			if (index >= size()) {
				HalleyExceptions::throwException("Index out of vector range", 217);
			}
			return data()[index];
		}

		[[nodiscard]] const_reference at(size_t index) const
		{
			if (index >= size()) {
				HalleyExceptions::throwException("Index out of vector range", 217);
			}
			return data()[index];			
		}

		[[nodiscard]] reference front()
		{
			assert(!empty());
			return data()[0];
		}

		[[nodiscard]] reference back()
		{
			assert(!empty());
			return data()[size() - 1];
		}

		[[nodiscard]] const_reference front() const
		{
			assert(!empty());
			return data()[0];			
		}

		[[nodiscard]] const_reference back() const
		{
			assert(!empty());
			return data()[size() - 1];			
		}

		void resize(size_t size)
		{
			do_resize(size, [this] (pointer bytes)
			{
				std::allocator_traits<Allocator>::construct(as_allocator(), bytes, T());
			});
		}

		void resize_no_init(size_t size)
		{
			do_resize(size, [] (pointer bytes) {});
		}

		void resize(size_t size, T defaultValue)
		{
			do_resize(size, [&] (pointer bytes)
			{
				std::allocator_traits<Allocator>::construct(as_allocator(), bytes, defaultValue);
			});
		}

		void reserve(size_t size)
		{
			if (size > capacity()) {
				change_capacity(static_cast<size_type>(std::max(size, static_cast<size_t>(capacity() * growth_factor))));
			}
		}

		void shrink_to_fit()
		{
			change_capacity(st_size());
		}

		void clear() noexcept
		{
			for (size_type i = 0; i < st_size(); ++i) {
				std::allocator_traits<Allocator>::destroy(as_allocator(), data() + i);
			}
			set_size(0);
		}

		iterator insert(const_iterator pos, const T& value)
		{
			return do_insert(pos, [&] (size_t prevSize) {
				push_back(value);
			});
		}

		iterator insert(const_iterator pos, T&& value)
		{
			return do_insert(pos, [&] (size_t prevSize) {
				push_back(std::move(value));
			});
		}

		iterator insert(const_iterator pos, size_t count, const T& value)
		{
			return do_insert(pos, [&](size_t prevSize) {
				reserve(size() + count);
				for (size_t i = 0; i < count; ++i) {
					std::allocator_traits<Allocator>::construct(as_allocator(), data() + (i + prevSize), value);
				}
				set_size(static_cast<size_type>(prevSize + count));
			});
		}

		template <class InputIt, std::enable_if_t<is_iterator_v<InputIt>, int> Test = 0>
		iterator insert(const_iterator pos, InputIt first, InputIt last)
		{
			return do_insert(pos, [&](size_t prevSize) {
				if constexpr (std::is_same_v<typename std::iterator_traits<InputIt>::iterator_category, std::random_access_iterator_tag>) {
					const auto count = last - first;
					reserve(size() + count);
					size_t i = 0;
					for (auto iter = first; iter != last; ++iter) {
						std::allocator_traits<Allocator>::construct(as_allocator(), data() + (i + prevSize), *iter);
						++i;
					}
					set_size(static_cast<size_type>(prevSize + count));
				} else {
					for (auto iter = first; iter != last; ++iter) {
						push_back(*iter);
					}
				}
			});
		}
		
		iterator insert(const_iterator pos, std::initializer_list<T> initializerList)
		{
			return insert(pos, initializerList.begin(), initializerList.end());
		}

		template <class... Args>
		iterator emplace(const_iterator pos, Args&&... args)
		{
			const auto idx = pos - begin();
			emplace_back(std::forward<Args>(args)...);
			std::rotate(begin() + idx, end() - 1, end());
			return begin() + idx;
		}

		iterator erase(const_iterator first, const_iterator last)
		{
			if (first == last) {
				return de_const_iter(last);
			}
			const auto idx = first - begin();
			std::rotate(de_const_iter(first), de_const_iter(last), end());
			resize_down(static_cast<size_type>(size() - (last - first)));
			return begin() + idx;
		}

		iterator erase(const_iterator pos)
		{
			assert(pos != end());
			return erase(pos, pos + 1);
		}

		template <class... Args>
		reference emplace_back(Args&&... args)
		{
			const auto idx = size();
			construct_with_ensure_capacity(st_size() + 1, [&] (pointer data)
			{
				std::allocator_traits<Allocator>::construct(as_allocator(), data + idx, std::forward<Args>(args)...);
			});
			set_size(st_size() + 1);
			return elem(idx);
		}

		void push_back(const T& value)
		{
			construct_with_ensure_capacity(st_size() + 1, [&](pointer data)
			{
				std::allocator_traits<Allocator>::construct(as_allocator(), data + size(), value);
			});
			set_size(st_size() + 1);
		}

		void push_back(T&& value)
		{
			construct_with_ensure_capacity(st_size() + 1, [&](pointer data)
			{
				std::allocator_traits<Allocator>::construct(as_allocator(), data + size(), std::move(value));
			});
			set_size(st_size() + 1);
		}

		void pop_back()
		{
			assert(!empty());
			std::allocator_traits<Allocator>::destroy(as_allocator(), &back());
			set_size(st_size() - 1);
		}

		void swap(VectorStd& other) noexcept
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
			return iterator(&elem(size()));
		}

		[[nodiscard]] const_iterator begin() const
		{
			return const_iterator(&elem(0));
		}

		[[nodiscard]] const_iterator end() const
		{
			return const_iterator(&elem(size()));
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

		[[nodiscard]] constexpr static bool sbo_enabled()
		{
			return sbo_max_objects() > 0;
		}

		[[nodiscard]] constexpr bool sbo_active() const
		{
			if constexpr (sbo_enabled()) {
				return m_size.small.sbo_enabled;
			} else {
				return false;
			}
		}

		[[nodiscard]] constexpr static size_type sbo_max_objects()
		{
			if constexpr (EnableSBO) {
				constexpr auto size_bytes = sizeof(VectorStd);
				constexpr auto sbo_align_enabled = alignof(VectorStd) >= alignof(T);
				constexpr auto first_sbo_offset = sbo_start_offset_bytes();
				const auto result = sbo_align_enabled && first_sbo_offset < size_bytes ? (size_bytes - first_sbo_offset) / sizeof(T) : 0ull;
				static_assert(result <= 127); // More than 127 wouldn't fit in the 7-bit size field
				return static_cast<size_type>(result);
			} else {
				return 0;
			}
		}

		[[nodiscard]] gsl::span<const T> span() const
		{
			return gsl::span<const T>(data(), size());
		}

		[[nodiscard]] gsl::span<T> span()
		{
			return gsl::span<T>(data(), size());
		}

		[[nodiscard]] gsl::span<const T> const_span() const
		{
			return gsl::span<const T>(data(), size());
		}

		[[nodiscard]] gsl::span<const gsl::byte> byte_span() const
		{
			return gsl::as_bytes(span());
		}

		[[nodiscard]] gsl::span<gsl::byte> byte_span()
		{
			return gsl::as_writable_bytes(span());
		}

		[[nodiscard]] gsl::span<const gsl::byte> const_byte_span() const
		{
			return gsl::as_bytes(span());
		}

	private:
		struct SBO {
			union {
				struct {
					size_type sbo_enabled : 1;
					size_type size : sizeof(size_type) * 8 - 1;
				} big;
				struct {
					uint8_t sbo_enabled : 1;
					uint8_t size : 7;
					uint8_t padding[sizeof(size_type) - 1];
				} small;
			};

			SBO() noexcept
			{
				big.sbo_enabled = false;
				big.size = 0;
			}
		} m_size;
		size_type m_capacity = 0;
		union {
			pointer m_data = nullptr;
			uint8_t sbo_padding[SBOPadding + sizeof(pointer)];
		};

		static_assert(sizeof(SBO) == sizeof(size_type));
		static_assert(alignof(SBO) == alignof(size_type));

		void change_capacity(size_type newCapacity)
		{
			change_capacity(newCapacity, [](pointer) {});
		}

		template <typename F>
		void change_capacity(size_type newCapacity, const F& construct)
		{
			const auto size = st_size();
			const auto capacity = st_capacity();
			assert(newCapacity >= size);
			if (newCapacity != capacity) {
				// Allocate new memory
				const bool canUseSBO = sbo_max_objects() >= newCapacity;
				pointer newData = canUseSBO ? sbo_data() : (newCapacity > 0 ? std::allocator_traits<Allocator>::allocate(as_allocator(), newCapacity) : nullptr);
				construct(newData);

				// Move old objects
				auto* oldData = data();
				if constexpr (std::is_trivially_copyable_v<T>) {
					if (size > 0) {
						memcpy(newData, oldData, size * sizeof(T));
					}
				} else {
					for (size_type i = 0; i < size; ++i) {
						std::allocator_traits<Allocator>::construct(as_allocator(), newData + i, std::move(oldData[i]));
						std::allocator_traits<Allocator>::destroy(as_allocator(), oldData + i);
					}
				}

				// Deallocate old memory
				if (!sbo_active() && oldData && capacity > 0) {
					std::allocator_traits<Allocator>::deallocate(as_allocator(), oldData, capacity);
				}

				if (sbo_active() != canUseSBO) {
					if constexpr (sbo_enabled()) {
						m_size.small.sbo_enabled = canUseSBO;
					}
					set_size(size);
				}

				if (!canUseSBO) {
					m_data = newData;
					m_capacity = newCapacity;
				}
			}
		}

		template<typename F>
		void do_resize(size_t nSize, const F& construct)
		{
			const auto newSize = static_cast<size_type>(nSize);
			if (newSize > st_size()) {
				if (newSize > capacity()) {
					change_capacity(newSize);
				}
				auto* d = data();
				for (size_type i = st_size(); i < newSize; ++i) {
					construct(d + i);
				}
				set_size(newSize);
			} else if (newSize < st_size()) {
				resize_down(newSize);
			}
		}

		void resize_down(size_type newSize)
		{
			assert(newSize <= st_size());
			auto* d = data();
			for (size_type i = newSize; i < st_size(); ++i) {
				std::allocator_traits<Allocator>::destroy(as_allocator(), d + i);
			}
			set_size(newSize);
		}

		void set_size(size_type sz)
		{
			if (sbo_active()) {
				m_size.small.size = static_cast<uint8_t>(sz);
			} else {
				m_size.big.size = sz;
			}
		}

		template <typename F>
		void construct_with_ensure_capacity(size_type minCapacity, const F& construct)
		{
			if (capacity() >= minCapacity) {
				construct(data());
			} else {
				change_capacity(std::max(minCapacity, static_cast<size_type>(capacity() * growth_factor)), construct);
			}
		}

		template <typename F>
		iterator do_insert(const_iterator pos, F f)
		{
			const auto prevSize = size();
			const auto idx = pos - begin();

			f(prevSize);

			if (pos != end()) {
				std::rotate(begin() + idx, begin() + prevSize, end());
				return begin() + idx;
			} else {
				return begin() + prevSize;
			}
		}

		void move_data_from(VectorStd& other)
		{
			if (other.sbo_active()) {
				// Using SBO, move elements
				m_size = other.m_size;

				auto* dst = data();
				auto iter = other.begin();
				const auto iterEnd = other.end();
				for (; iter != iterEnd; ++iter) {
					std::allocator_traits<Allocator>::construct(*this, dst++, std::move(*iter));
				}
				
				other.clear();
			} else {
				// No SBO, steal data
				m_size = other.m_size;
				m_capacity = other.m_capacity;
				m_data = other.m_data;

				other.m_data = nullptr;
				other.m_size = {};
				other.m_capacity = 0;
			}
		}

		void destroy()
		{
			clear();
			change_capacity(0);
		}

		[[nodiscard]] reference elem(size_t pos)
		{
			return data()[pos];
		}

		[[nodiscard]] const_reference elem(size_t pos) const
		{
			return data()[pos];
		}

		[[nodiscard]] iterator de_const_iter(const_iterator iter)
		{
			return iterator(begin() + (iter - begin()));
		}

		[[nodiscard]] Allocator& as_allocator() noexcept
		{
			return *this;
		}

		[[nodiscard]] constexpr size_type st_size() const
		{
			return sbo_active() ? m_size.small.size : m_size.big.size;
		}

		[[nodiscard]] constexpr size_type st_capacity() const
		{
			return sbo_active() ? sbo_max_objects() : m_capacity;
		}

		[[nodiscard]] constexpr pointer sbo_data()
		{
			return reinterpret_cast<pointer>(reinterpret_cast<char*>(this) + sbo_start_offset_bytes());
		}

		[[nodiscard]] constexpr const_pointer sbo_data() const
		{
			return reinterpret_cast<const_pointer>(reinterpret_cast<const char*>(this) + sbo_start_offset_bytes());
		}

		[[nodiscard]] constexpr static std::size_t sbo_start_offset_bytes()
		{
			return VectorDetail::alignUp<size_t>(1ull, alignof(T));
		}
	};

	template<typename T, typename SizeType, bool SBO0, bool SBO1, size_t SBOP0, size_t SBOP1, class A0, class A1>
	bool operator==(const VectorStd<T, SizeType, SBO0, SBOP0, A0>& a, const VectorStd<T, SizeType, SBO1, SBOP1, A1>& b)
	{
		return std::equal(a.begin(), a.end(), b.begin(), b.end());
	}

	template<typename T, typename SizeType, bool SBO0, bool SBO1, size_t SBOP0, size_t SBOP1, class A0, class A1>
	bool operator!=(const VectorStd<T, SizeType, SBO0, SBOP0, A0>& a, const VectorStd<T, SizeType, SBO1, SBOP1, A1>& b)
	{
		return !std::equal(a.begin(), a.end(), b.begin(), b.end());
	}

	template<typename T, typename SizeType, bool SBO0, bool SBO1, size_t SBOP0, size_t SBOP1, class A0, class A1>
	bool operator<(const VectorStd<T, SizeType, SBO0, SBOP0, A0>& a, const VectorStd<T, SizeType, SBO1, SBOP1, A1>& b)
	{
		return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end(), [](const T& a, const T& b) { return a < b; });
	}

	template<typename T, typename SizeType, bool SBO0, bool SBO1, size_t SBOP0, size_t SBOP1, class A0, class A1>
	bool operator>(const VectorStd<T, SizeType, SBO0, SBOP0, A0>& a, const VectorStd<T, SizeType, SBO1, SBOP1, A1>& b)
	{
		return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end(), [](const T& a, const T& b) { return a > b; });
	}

	template<typename T, typename SizeType, bool SBO0, bool SBO1, size_t SBOP0, size_t SBOP1, class A0, class A1>
	bool operator<=(const VectorStd<T, SizeType, SBO0, SBOP0, A0>& a, const VectorStd<T, SizeType, SBO1, SBOP1, A1>& b)
	{
		return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end(), [](const T& a, const T& b) { return a <= b; });
	}

	template<typename T, typename SizeType, bool SBO0, bool SBO1, size_t SBOP0, size_t SBOP1, class A0, class A1>
	bool operator>=(const VectorStd<T, SizeType, SBO0, SBOP0, A0>& a, const VectorStd<T, SizeType, SBO1, SBOP1, A1>& b)
	{
		return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end(), [](const T& a, const T& b) { return a >= b; });
	}


	template<typename T, typename SizeType, bool EnableSBO, size_t SBOPadding, class Allocator>
	void swap(VectorStd<T, SizeType, EnableSBO, SBOPadding, Allocator>& a, VectorStd<T, SizeType, EnableSBO, SBOPadding, Allocator>& b) noexcept
	{
		a.swap(b);
	}


	// Default versions
	template<typename T, typename Allocator = std::allocator<T>, int Padding = 0>
	using VectorSize32 = VectorStd<T, uint32_t, true, Padding, Allocator>;
}
