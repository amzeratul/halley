#pragma once

#include "vector.h"

namespace Halley {
	template <typename T, typename P = int>
	class OverrideSet {
	public:
		using Key = uint32_t;

		Key add(T value, P priority = {})
		{
			const auto key = nextKey++;
			entries.emplace_back(Entry{ std::move(value), key, priority});
			std::sort(entries.begin(), entries.end());
			return key;
		}

		void remove(Key key)
		{
			std_ex::erase_if(entries, [&](const Entry& o) { return o.key == key; });
		}

		bool hasValue() const
		{
			return !entries.empty();
		}

		const T& getValue() const
		{
			return entries.back().value;
		}

		const T& getValueOr(const T& fallback) const
		{
			return entries.empty() ? fallback : entries.back().value;
		}

		const T* tryGetValue() const
		{
			return entries.empty() ? nullptr : &entries.back().value;
		}

	private:
		struct Entry {
			T value;
			Key key;
			P priority;

			bool operator<(const Entry& other) const
			{
				return priority < other.priority;
			}
		};

		Vector<Entry> entries;
		Key nextKey = 0;
	};
}
