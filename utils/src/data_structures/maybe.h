#pragma once

#include <array>
#include <functional>

namespace Halley
{
	template <typename T>
	class Maybe
	{
	public:
		Maybe()
			: defined(false)
		{}

		Maybe(const T& v)
			: defined(true)
		{
			*getData() = v;
		}

		Maybe(T&& v)
			: defined(true)
		{
			*getData() = v;
		}

		~Maybe()
		{
			reset();
		}

		Maybe& operator=(const T& o) {
			reset();
			*getData() = o;
			defined = true;
			return *this;
		}

		Maybe& operator=(T&& o) {
			reset();
			*getData() = o;
			defined = true;
			return *this;
		}

		Maybe& operator=(const Maybe& o) {
			reset();
			if (o.defined) {
				*getData() = *o.getData();
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
		
	private:
		std::array<char, sizeof(T)> data;
		bool defined;

		T* getData()
		{
			return reinterpret_cast<T*>(data.data());
		}

		const T* getData() const
		{
			return reinterpret_cast<const T*>(data.data());
		}
	};
}