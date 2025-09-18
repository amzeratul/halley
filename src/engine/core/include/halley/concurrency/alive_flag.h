#pragma once
#include <memory>

namespace Halley {
	class NonOwningAliveFlag;

	class AliveFlag {
		friend class NonOwningAliveFlag;

	public:
		AliveFlag();
		~AliveFlag();

		AliveFlag(const AliveFlag& other);
		AliveFlag(AliveFlag&& other) = default;
		AliveFlag& operator=(const AliveFlag& other);
		AliveFlag& operator=(AliveFlag&& other) = default;

		template <typename F>
		void runOnUpdate(F f);

		template <typename E, typename F>
		void runOnExecutor(E& e, F f);

		operator bool() const;

	private:
		std::shared_ptr<bool> flag;
	};

	class NonOwningAliveFlag {
	public:
		NonOwningAliveFlag() = default;
		NonOwningAliveFlag(const AliveFlag& flag);

		operator bool() const;

	private:
		std::shared_ptr<bool> flag;
	};
}

namespace Halley {
	/*
	*/
}
