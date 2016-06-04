#include "utils/world_stats.h"
#include "halley/core/graphics/render_context.h"
#include "graphics/text/font.h"
#include "resources/resources.h"
#include "halley/core/api/core_api.h"
#include <halley/entity/world.h>
#include <halley/entity/system.h>

using namespace Halley;

WorldStatsView::WorldStatsView(CoreAPI& coreAPI, const World& world)
	: coreAPI(coreAPI)
	, world(world)
	, text(coreAPI.getResources().get<Font>("ubuntub.yaml"), "", 16, Colour(1, 1, 1), 1.5f, Colour(0.1f, 0.1f, 0.1f))
{
}

void WorldStatsView::draw(RenderContext& context)
{
	context.bind([&] (Painter& painter) {
		TimeLine timelines[] = { TimeLine::FixedUpdate, TimeLine::VariableUpdate, TimeLine::Render };
		String timelineLabels[] = { "Fixed", "Variable", "Render" };
		int i = 0;

		long long grandTotal = 0;

		auto drawStats = [&] (String name, int nEntities, long long time, Vector2f& basePos)
		{
			text.setText(name).setAlignment(0).draw(painter, basePos + Vector2f(10, 0));
			text.setAlignment(1);
			if (nEntities > 0) {
				text.setText(String::integerToString(nEntities)).draw(painter, basePos + Vector2f(280, 0));
			}
			text.setText(formatTime(time)).draw(painter, basePos + Vector2f(350, 0));
			text.setAlignment(0);
			basePos.y += 20;
		};

		for (auto timeline : timelines) {
			Vector2f pos = Vector2f(40 + (i++) * 400.0f, 80);
			text.setColour(Colour(0.2f, 1.0f, 0.3f)).setText(String(timelineLabels[int(timeline)]) + ": ").draw(painter, pos);
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
		text.setColour(Colour(1, 1, 1)).setText("Total elapsed: " + formatTime(grandTotal) + " ms [" + String::integerToString(maxFPS) + " FPS maximum].").draw(painter, Vector2f(40, 40));
	});
}

String WorldStatsView::formatTime(long long ns) const
{
	long long us = (ns + 500) / 1000;
	std::stringstream ss;
	ss << (us / 1000) << '.' << std::setw(3) << std::setfill('0') << (us % 1000);
	return ss.str();
}
