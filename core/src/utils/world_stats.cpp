#include "world_stats.h"
#include "../graphics/text/text_renderer.h"
#include "../graphics/render_context.h"
#include "../graphics/text/font.h"
#include "../resources/resources.h"
#include "../api/core_api.h"

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
		for (auto timeline : timelines) {
			Vector2f pos = Vector2f(40 + (i++) * 400.0f, 40);
			text.setColour(Colour(0.2f, 1.0f, 0.3f)).setText(String(timelineLabels[int(timeline)]) + ": ").draw(painter, pos);
			text.setColour(Colour(1, 1, 1));
			pos.y += 20;

			long long engineTotal = coreAPI.getAverageTime(timeline);
			long long worldTotal = world.getAverageTime(timeline);
			long long sysTotal = 0;

			for (auto& system : world.getSystems(timeline)) {
				String name = system->getName();
				long long ns = system->getNanoSecondsTakenAvg();
				sysTotal += ns;

				text.setText(name).draw(painter, pos + Vector2f(10, 0));
				text.setText(formatTime(ns)).draw(painter, pos + Vector2f(300, 0));
				pos.y += 20;
			}

			text.setColour(Colour(0.7f, 0.7f, 0.7f));
			text.setText("World Overhead").draw(painter, pos + Vector2f(10, 0));
			text.setText(formatTime(worldTotal - sysTotal)).draw(painter, pos + Vector2f(300, 0));
			pos.y += 20;
			text.setText("Engine Overhead").draw(painter, pos + Vector2f(10, 0));
			text.setText(formatTime(engineTotal - worldTotal)).draw(painter, pos + Vector2f(300, 0));
			pos.y += 20;
			text.setColour(Colour(0.7f, 0.8f, 0.7f));
			text.setText("Total").draw(painter, pos + Vector2f(10, 0));
			text.setText(formatTime(engineTotal)).draw(painter, pos + Vector2f(300, 0));
			pos.y += 20;
		}
	});
}

String WorldStatsView::formatTime(long long ns) const
{
	long long us = (ns + 500) / 1000;
	std::stringstream ss;
	ss << std::setw(3) << std::setfill('0') << (us / 1000) << ' ' << std::setw(3) << std::setfill('0') << (us % 1000);
	return ss.str();
}
