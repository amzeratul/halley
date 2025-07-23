#pragma once

#include "vector.h"
#include "halley/concurrency/alive_flag.h"
#include "halley/utils/algorithm.h"

namespace Halley {

	class IListenerSet {
	public:
		using KeyType = uint32_t;

		virtual ~IListenerSet() = default;
		virtual void remove(KeyType key) = 0;
	};

	class ListenerSetToken {
	public:
		using KeyType = uint32_t;

		ListenerSetToken(KeyType key, IListenerSet& listenerSet, NonOwningAliveFlag aliveFlag)
			: key(key)
			, listenerSet(&listenerSet)
			, aliveFlag(std::move(aliveFlag))
		{}

		ListenerSetToken() = default;

		ListenerSetToken(const ListenerSetToken& other) = delete;
		ListenerSetToken& operator=(const ListenerSetToken& other) = delete;

		ListenerSetToken(ListenerSetToken&& other) noexcept
		{
			moveFrom(other);
		}

		ListenerSetToken& operator=(ListenerSetToken&& other) noexcept
		{
			moveFrom(other);
			return *this;
		}

		~ListenerSetToken() noexcept
		{
			unbind();
		}

		void unbind() noexcept
		{
			if (listenerSet) {
				if (aliveFlag) {
					listenerSet->remove(key);
				}
				listenerSet = nullptr;
				key = 0;
			}
		}

	private:
		KeyType key = 0;
		IListenerSet* listenerSet = nullptr;
		NonOwningAliveFlag aliveFlag;

		void moveFrom(ListenerSetToken& other) noexcept
		{
			key = other.key;
			listenerSet = other.listenerSet;
			aliveFlag = std::move(other.aliveFlag);
			other.key = 0;
			other.listenerSet = nullptr;
			other.aliveFlag = {};
		}
	};

	template <typename T>
	class ListenerSet : private IListenerSet {
	public:
		using Callback = std::function<void(T)>;

		[[nodiscard]] ListenerSetToken add(Callback callback)
		{
			const auto key = nextKey++;
			entries.emplace_back(Entry{ std::move(callback), key });
			return ListenerSetToken(key, *this, NonOwningAliveFlag(aliveFlag));
		}

		void notify(const T& value) const
		{
			for (auto& e: entries) {
				e.value(value);
			}
		}

	protected:
		void remove(KeyType key) override
		{
			std_ex::erase_if(entries, [&](const Entry& o) { return o.key == key; });
		}

	private:
		struct Entry {
			Callback value;
			KeyType key;
		};

		Vector<Entry> entries;
		KeyType nextKey = 0;
		AliveFlag aliveFlag;
	};
}
