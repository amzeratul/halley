#pragma once
#include "video_api.h"
#include "system_api.h"
#include "input_api.h"
#include "core_api.h"

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
	public:
		~HalleyAPI();
		CoreAPI* const core;
		const std::unique_ptr<SystemAPI> system;
		const std::unique_ptr<VideoAPI> video;
		const std::unique_ptr<InputAPI> input;

	private:
		friend class CoreRunner;

		HalleyAPI(CoreAPI* core, std::unique_ptr<SystemAPI> system, std::unique_ptr<VideoAPI> video, std::unique_ptr<InputAPI> input);

		static std::unique_ptr<HalleyAPI> create(CoreAPI* core, int flags);
	};
}
