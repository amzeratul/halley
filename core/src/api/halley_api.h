#pragma once
#include "video_api.h"
#include "core_api.h"

namespace Halley
{
	namespace HalleyAPIFlags {
		enum Flags
		{
			Core = 1,
			Video = 2,
			Audio = 4,
			Input = 8
		};
	}

	class HalleyAPI
	{
	public:
		~HalleyAPI();
		const std::unique_ptr<CoreAPI> core;
		const std::unique_ptr<VideoAPI> video;

	private:
		friend class CoreRunner;

		HalleyAPI(std::unique_ptr<CoreAPI> core, std::unique_ptr<VideoAPI> video);

		static std::unique_ptr<HalleyAPI> create(int flags);
	};
}
