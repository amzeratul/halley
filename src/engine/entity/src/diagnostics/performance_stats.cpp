#include "diagnostics/performance_stats.h"
#include "world.h"
#include "halley/core/api/core_api.h"
#include "halley/core/graphics/render_context.h"
#include "halley/core/resources/resources.h"
#include "halley/time/halleytime.h"
#include "halley/time/stopwatch.h"

using namespace Halley;

PerformanceStatsView::PerformanceStatsView(Resources& resources, CoreAPI& coreAPI)
	: StatsView(resources, coreAPI)
	, text(resources.get<Font>("Ubuntu Bold"), "", 16, Colour(1, 1, 1), 1.0f, Colour(0.1f, 0.1f, 0.1f))
{}

void PerformanceStatsView::draw(RenderContext& context)
{
	context.bind([&](Painter& painter) {
		int64_t grandTotal = 0;

		TimeLine timelines[] = { TimeLine::FixedUpdate, TimeLine::VariableUpdate, TimeLine::Render };

		for (auto timeline : timelines) {
			const int64_t total = coreAPI.getTime(CoreAPITimer::Engine, timeline, StopwatchAveraging::Mode::Average);
			grandTotal += total;
		}

		const int64_t vsyncTime = coreAPI.getTime(CoreAPITimer::Vsync, TimeLine::Render, StopwatchAveraging::Mode::Average);
		const int64_t nonVsyncTotal = grandTotal - vsyncTime;

		const int curFPS = static_cast<int>(lround(1'000'000'000.0 / grandTotal));
		const int maxFPS = static_cast<int>(lround(1'000'000'000.0 / nonVsyncTotal));

		String str = "Capped: " + formatTime(grandTotal) + " ms [" + toString(curFPS) + " FPS] | Uncapped: " + formatTime(nonVsyncTotal) + " ms [" + toString(maxFPS) + " FPS].\n"
			+ toString(painter.getPrevDrawCalls()) + " draw calls, " + toString(painter.getPrevTriangles()) + " triangles, " + toString(painter.getPrevVertices()) + " vertices.";
		
		text
			.setColour(Colour(1, 1, 1))
			.setText(str)
			.setPosition(Vector2f(20, 20))
			.draw(painter);
		});
}
