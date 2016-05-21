#pragma once
#include "halley/graphics/text/text_renderer.h"

namespace Halley
{
	class CoreAPI;
	class Resources;
	class RenderContext;
	class World;

	class WorldStatsView
	{
	public:
		WorldStatsView(CoreAPI& coreApi, const World& world);
		void draw(RenderContext& context);

	private:
		String formatTime(long long ns) const;

		const CoreAPI& coreAPI;
		const World& world;
		TextRenderer text;
	};
}
