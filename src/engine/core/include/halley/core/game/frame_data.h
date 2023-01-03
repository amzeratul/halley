#pragma once

#include <type_traits>

namespace Halley {
	class IFrameData {
	public:
		virtual ~IFrameData() = default;

		static void setThreadFrameData(IFrameData* value)
		{
			threadInstance = value;
		}

		virtual void doStartFrame(bool multithreaded, IFrameData* previous) {}

	protected:
		static thread_local IFrameData* threadInstance;
	};

	template <typename T>
	class FrameData : public IFrameData {
	public:
		virtual void startFrame(bool multithreaded, T* previous) {}

		static T& getCurrent()
		{
			assert(hasCurrent());
			return static_cast<T&>(*threadInstance);
		}

		static bool hasCurrent()
		{
			return threadInstance != nullptr;
		}

	protected:
		void doStartFrame(bool multithreaded, IFrameData* previous) override
		{
			startFrame(multithreaded, static_cast<T*>(previous));
		}
	};

	class EmptyFrameData : public FrameData<EmptyFrameData> {
	public:
	};
}
