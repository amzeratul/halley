#pragma once

#include <type_traits>

namespace Halley {
	class Core;
	class SceneEditor;

	class IFrameData {
		friend class Core;
		friend class SceneEditor;

	public:
		virtual ~IFrameData() = default;

	protected:
		static thread_local IFrameData* threadInstance;

		virtual void doStartFrame(bool multithreaded, IFrameData* previous) {}
	};

	template <typename T>
	class FrameData : public IFrameData {
	public:
		virtual void startFrame(bool multithreaded, T* previous) {}

		static T& getCurrent()
		{
			assert(threadInstance != nullptr);
			return static_cast<T&>(*threadInstance);
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
