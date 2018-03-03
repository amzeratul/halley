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
		std::unique_ptr<SystemAPIInternal> systemInternal;
		std::unique_ptr<VideoAPIInternal> videoInternal;
		std::unique_ptr<InputAPIInternal> inputInternal;
		std::unique_ptr<AudioAPIInternal> audioInternal;
		std::unique_ptr<AudioOutputAPIInternal> audioOutputInternal;
		std::unique_ptr<PlatformAPIInternal> platformInternal;
		std::unique_ptr<NetworkAPIInternal> networkInternal;
		std::unique_ptr<MovieAPIInternal> movieInternal;

	public:
		~HalleyAPI();
		CoreAPI* core;
		SystemAPI* system;
		VideoAPI* video;
		InputAPI* input;
		AudioAPI* audio;
		PlatformAPI* platform;
		NetworkAPI* network;
		MovieAPI* movie;
		
		template <typename T>
		std::shared_ptr<const T> getResource(String name) const
		{
			return core->getResources().of<T>().get(name);
		}

	private:
		friend class Core;

		void init();
		void deInit();
		void assign();
		static std::unique_ptr<HalleyAPI> create(CoreAPIInternal* core, int flags);
	};
}
