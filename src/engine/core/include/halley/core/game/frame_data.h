#pragma once

#include <type_traits>

namespace Halley {
	class Core;

	class FrameData {
		friend class Core;

	public:
		virtual ~FrameData() = default;

		virtual void clear() {}

		template <typename T>
		static T& getCurrent()
		{
			static_assert(std::is_base_of_v<FrameData, T>);
			assert(threadInstance != nullptr);
			return static_cast<T&>(*threadInstance);
		}

	private:
		static thread_local FrameData* threadInstance;
	};
}
