#pragma once

#include <type_traits>

namespace Halley {
	class Core;
	class SceneEditor;
	class AssetPreviewGenerator;

	class IFrameData {
		friend class Core;
		friend class SceneEditor;
		friend class AssetPreviewGenerator;

	public:
		virtual ~IFrameData() = default;

		static void setThreadFrameData(IFrameData* value)
		{
			threadInstance = value;
		}

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
