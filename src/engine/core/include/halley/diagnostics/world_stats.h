#pragma once
#include "stats_view.h"

namespace Halley
{
	class WorldStatsView : public StatsView
	{
	public:
		WorldStatsView(Resources& resources, const HalleyAPI& api);

		void paint(Painter& painter) override;

	private:
		TextRenderer text;
	};
}
