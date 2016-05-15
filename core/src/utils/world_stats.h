#pragma once
#include "../graphics/text/text_renderer.h"

namespace Halley
{
	class Resources;
	class RenderContext;
	class World;

	class WorldStatsView
	{
	public:
		WorldStatsView(Resources& resources, const World& world);
		void draw(RenderContext& context);

	private:
		const World& world;
		TextRenderer text;
	};
}
