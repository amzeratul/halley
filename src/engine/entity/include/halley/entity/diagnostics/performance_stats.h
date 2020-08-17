#pragma once

#include "stats_view.h"

namespace Halley
{
	class PerformanceStatsView : public StatsView
	{
	public:
		PerformanceStatsView(Resources& resources, CoreAPI& coreAPI);

		void draw(RenderContext& context) override;

	private:
		TextRenderer text;
	};
}
