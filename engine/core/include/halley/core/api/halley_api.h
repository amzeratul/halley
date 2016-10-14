#pragma once
#include "halley_api_internal.h"
#include "halley/core/resources/resources.h"

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
		const std::unique_ptr<AudioAPIInternal> audioInternal;
		const std::unique_ptr<AudioOutputAPIInternal> audioOutputInternal;

	public:
		~HalleyAPI();
		CoreAPI* const core;
		SystemAPI* const system;
		VideoAPI* const video;
		InputAPI* const input;
		AudioAPI* const audio;
		
		template <typename T>
		std::shared_ptr<const T> getResource(String name)
		{
			return core->getResources().of<T>().get(name);
		}

	private:
		friend class Core;

		HalleyAPI(CoreAPIInternal* core, std::unique_ptr<SystemAPIInternal> system, std::unique_ptr<VideoAPIInternal> video, std::unique_ptr<InputAPIInternal> input, std::unique_ptr<AudioAPIInternal> audio, std::unique_ptr<AudioOutputAPIInternal> audioOutput);
		static std::unique_ptr<HalleyAPI> create(CoreAPIInternal* core, int flags);
	};
}
