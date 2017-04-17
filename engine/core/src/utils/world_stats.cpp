#include "utils/world_stats.h"
#include "halley/core/graphics/render_context.h"
#include "graphics/text/font.h"
#include "resources/resources.h"
#include "halley/core/api/core_api.h"
#include <halley/entity/world.h>
#include <halley/entity/system.h>
#include "halley/text/string_converter.h"

using namespace Halley;

WorldStatsView::WorldStatsView(CoreAPI& coreAPI, const World& world)
	: coreAPI(coreAPI)
	, world(world)
	, text(coreAPI.getResources().get<Font>("Ubuntu Bold"), "", 16, Colour(1, 1, 1), 1.0f, Colour(0.1f, 0.1f, 0.1f))
{
}

void WorldStatsView::draw(RenderContext& context)
{
	context.bind([&] (Painter& painter) {
		TimeLine timelines[] = { TimeLine::FixedUpdate, TimeLine::VariableUpdate, TimeLine::Render };
		String timelineLabels[] = { "Fixed", "Variable", "Render" };
		int i = 0;
		float width = (float(context.getCamera().getActiveViewPort().getWidth()) - 40.0f) / 3.0f;

		long long grandTotal = 0;

		auto drawStats = [&] (String name, int nEntities, long long time, Vector2f& basePos)
		{
			text.setText(name).setAlignment(0).setPosition(basePos + Vector2f(10, 0)).draw(painter);
			text.setAlignment(1);
			if (nEntities > 0) {
				text.setText(toString(nEntities)).setPosition(basePos + Vector2f(width - 120, 0)).draw(painter);
			}
			text.setText(formatTime(time)).setPosition(basePos + Vector2f(width - 50, 0)).draw(painter);
			text.setAlignment(0);
			basePos.y += 20;
		};

		for (auto timeline : timelines) {
			Vector2f pos = Vector2f(20 + (i++) * width, 60);
			text.setColour(Colour(0.2f, 1.0f, 0.3f)).setText(String(timelineLabels[int(timeline)]) + ": ").setPosition(pos).draw(painter);
			text.setColour(Colour(1, 1, 1));
			pos.y += 20;

			long long engineTotal = coreAPI.getAverageTime(timeline);
			long long worldTotal = world.getAverageTime(timeline);
			long long sysTotal = 0;
			grandTotal += engineTotal;

			for (auto& system : world.getSystems(timeline)) {
				String name = system->getName();
				long long ns = system->getNanoSecondsTakenAvg();
				sysTotal += ns;

				drawStats(name, int(system->getEntityCount()), ns, pos);
			}

			text.setColour(Colour(0.8f, 0.8f, 0.8f));
			drawStats("[World]", 0, worldTotal - sysTotal, pos);
			drawStats("[Engine]", 0, engineTotal - worldTotal, pos);
			text.setColour(Colour(0.8f, 1.0f, 0.8f));
			drawStats("Total", int(world.numEntities()), engineTotal, pos);
		}

		int maxFPS = int(1'000'000'000.0 / grandTotal + 0.5f);
		text
			.setColour(Colour(1, 1, 1))
			.setText("Total elapsed: " + formatTime(grandTotal) + " ms [" + toString(maxFPS) + " FPS maximum].\n" + toString(painter.getPrevDrawCalls()) + " draw calls, " + toString(painter.getPrevTriangles()) + " triangles, " + toString(painter.getPrevVertices()) + " vertices.")
			.setPosition(Vector2f(20, 20))
			.draw(painter);
	});
}

String WorldStatsView::formatTime(long long ns) const
{
	long long us = (ns + 500) / 1000;
	std::stringstream ss;
	ss << (us / 1000) << '.' << std::setw(3) << std::setfill('0') << (us % 1000);
	return ss.str();
}
