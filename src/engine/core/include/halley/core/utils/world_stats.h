#pragma once
#include "halley/core/graphics/text/text_renderer.h"
#include <cstdint>

namespace Halley
{
	class CoreAPI;
	class Resources;
	class RenderContext;
	class World;

	class WorldStatsView
	{
	public:
		WorldStatsView(CoreAPI& coreApi);

		void draw(RenderContext& context);
		void setWorld(const World* world);

	private:
		String formatTime(int64_t ns) const;

		const CoreAPI& coreAPI;
		const World* world = nullptr;
		TextRenderer text;
	};
}
