#pragma once

#include <array>
#include <functional>
#include <cassert>
#include "halley/support/exception.h"

namespace Halley
{
	/*
	template <typename T>
	class Maybe
	{
	public:
		Maybe()
			: defined(false)
		{
		}

		Maybe(const T& v)
			: defined(true)
		{
			new(getData()) T(v);
		}

		Maybe(T&& v)
			: defined(true)
		{
			new(getData()) T(v);
		}

		~Maybe()
		{
			reset();
		}

		Maybe& operator=(const T& o) {
			assert(getData() != &o);

			reset();
			new(getData()) T(o);
			defined = true;
			return *this;
		}

		Maybe& operator=(T&& o) {
			assert(getData() != &o);

			reset();
			new(getData()) T(o);
			defined = true;
			return *this;
		}

		Maybe& operator=(const Maybe& o) {
			assert(this != &o);

			reset();
			if (o.defined) {
				new(getData()) T();
				defined = true;
			} else {
				defined = false;
			}
			return *this;
		}

		void reset()
		{
			if (defined) {
				getData()->~T();
				defined = false;
			}
		}

		using SuccessType = std::function<void(T&)>;
		using FailType = std::function<void()>;
		void match(SuccessType success, FailType fail)
		{
			if (defined) {
				success(*getData());
			} else {
				fail();
			}
		}

		void match(SuccessType success)
		{
			if (defined) {
				success(*getData());
			}
		}

		operator bool() const {
			return defined;
		}

		T& get()
		{
			if (defined) {
				return *getData();
			} else {
				throw Exception("Data not defined.");
			}
		}
		
		const T& get() const
		{
			if (defined) {
				return *getData();
			}
			else {
				throw Exception("Data not defined.");
			}
		}
		
	private:
		alignas(alignof(T)) std::array<char, sizeof(T)> data;
		bool defined;

		T* getData()
		{
			return reinterpret_cast<T*>(&data[0]);
		}

		const T* getData() const
		{
			return reinterpret_cast<const T*>(&data[0]);
		}
	};
	*/

	template <typename T>
	class Maybe
	{
	public:
		Maybe()
			: defined(false)
		{
		}

		Maybe(const T& v)
			: data(v)
			, defined(true)
		{
		}

		Maybe(T&& v)
			: data(v)
			, defined(true)
		{
		}

		~Maybe()
		{
			reset();
		}

		Maybe& operator=(const T& o) {
			data = o;
			defined = true;
			return *this;
		}

		Maybe& operator=(T&& o) {
			data = o;
			defined = true;
			return *this;
		}

		Maybe& operator=(const Maybe& o) {
			if (o.defined) {
				data = o.data;
				defined = true;
			} else {
				reset();
			}
			return *this;
		}

		void reset()
		{
			if (defined) {
				data = T();
				defined = false;
			}
		}

		using SuccessType = std::function<void(T&)>;
		using FailType = std::function<void()>;
		void match(SuccessType success, FailType fail)
		{
			if (defined) {
				success(data);
			} else {
				fail();
			}
		}

		void match(SuccessType success)
		{
			if (defined) {
				success(data);
			}
		}

		operator bool() const {
			return defined;
		}

		T& get()
		{
			if (defined) {
				return data;
			} else {
				throw Exception("Data not defined.");
			}
		}

		const T& get() const
		{
			if (defined) {
				return data;
			} else {
				throw Exception("Data not defined.");
			}
		}

	private:
		T data;
		bool defined;
	};
}