#pragma once
#include "stats_view.h"

namespace Halley
{
	class WorldStatsView : public StatsView
	{
	public:
		WorldStatsView(Resources& resources, CoreAPI& coreApi);

		void paint(Painter& painter) override;

	private:
		TextRenderer text;
	};
}
