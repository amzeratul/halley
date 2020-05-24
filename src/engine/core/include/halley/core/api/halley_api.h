#pragma once
#include "halley_api_internal.h"
#include "halley/core/resources/resources.h"

namespace Halley
{
	namespace HalleyAPIFlags {
		enum Flags
		{
			System = 0, // System is mandatory, so it's flag 0
			Video = 1,
			Audio = 2,
			Input = 4,
			Network = 8,
			Platform = 16,
			Movie = 32
		};
	}

	class HalleyAPI
	{
		// Beware of member order
		CoreAPIInternal* coreInternal;
		std::shared_ptr<SystemAPIInternal> systemInternal;
		std::shared_ptr<VideoAPIInternal> videoInternal;
		std::shared_ptr<InputAPIInternal> inputInternal;
		std::shared_ptr<AudioAPIInternal> audioInternal;
		std::shared_ptr<AudioOutputAPIInternal> audioOutputInternal;
		std::shared_ptr<PlatformAPIInternal> platformInternal;
		std::shared_ptr<NetworkAPIInternal> networkInternal;
		std::shared_ptr<MovieAPIInternal> movieInternal;

	public:
		CoreAPI* core;
		SystemAPI* system;
		VideoAPI* video;
		InputAPI* input;
		AudioAPI* audio;
		PlatformAPI* platform;
		NetworkAPI* network;
		MovieAPI* movie;

		std::unique_ptr<HalleyAPI> clone() const;
		void replaceCoreAPI(CoreAPIInternal* core);

	private:
		friend class Core;

		void init();
		void deInit();
		void assign();
		void setAPI(PluginType pluginType, HalleyAPIInternal* api);
		static std::unique_ptr<HalleyAPI> create(CoreAPIInternal* core, int flags);

		HalleyAPI& operator=(const HalleyAPI& other);
	};
}
