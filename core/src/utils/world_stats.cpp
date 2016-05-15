#include "world_stats.h"
#include "../graphics/text/text_renderer.h"
#include "../graphics/render_context.h"
#include "../graphics/text/font.h"
#include "../resources/resources.h"

void Halley::WorldStatsView::draw(RenderContext& context)
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

			for (auto& system : world.getSystems(timeline)) {
				String name = system->getName();
				int ns = system->getNanoSecondsTakenAvg();
				int us = (ns + 500) / 1000;
				std::stringstream ss;
				ss << std::setw(3) << std::setfill('0') << (us / 1000) << ' ' << std::setw(3) << std::setfill('0') << (us % 1000);

				text.setText(name).draw(painter, pos + Vector2f(10, 0));
				text.setText(ss.str()).draw(painter, pos + Vector2f(300, 0));
				pos.y += 20;
			}
		}
	});
}

Halley::WorldStatsView::WorldStatsView(Resources& resources, const World& world)
	: world(world)
	, text(resources.get<Font>("ubuntub.yaml"), "", 16, Colour(1, 1, 1), 1.5f, Colour(0.1f, 0.1f, 0.1f))
{
}
