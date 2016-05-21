#pragma once
#include "halley_api_internal.h"

namespace Halley
{
	namespace HalleyAPIFlags {
		enum Flags
		{
			Video = 1,
			Audio = 2,
			Input = 4
		};
	}

	class HalleyAPI
	{
		// Beware of member order
		CoreAPIInternal* coreInternal;
		const std::unique_ptr<SystemAPIInternal> systemInternal;
		const std::unique_ptr<VideoAPIInternal> videoInternal;
		const std::unique_ptr<InputAPIInternal> inputInternal;

	public:
		~HalleyAPI();
		CoreAPI* const core;
		SystemAPI* const system;
		VideoAPI* const video;
		InputAPI* const input;
		
		template <typename T>
		std::shared_ptr<T> getResource(String name)
		{
			return core->getResources().of<T>().get(name);
		}

	private:
		friend class CoreRunner;

		HalleyAPI(CoreAPIInternal* core, std::unique_ptr<SystemAPIInternal> system, std::unique_ptr<VideoAPIInternal> video, std::unique_ptr<InputAPIInternal> input);
		static std::unique_ptr<HalleyAPI> create(CoreAPIInternal* core, int flags);
	};
}
