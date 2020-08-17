#include "diagnostics/stats_view.h"


#include "halley/core/api/core_api.h"
#include "halley/core/graphics/camera.h"
#include "halley/core/graphics/render_context.h"
#include "halley/core/graphics/render_target/render_target.h"

using namespace Halley;

StatsView::StatsView(Resources& resources, CoreAPI& coreAPI)
	: resources(resources)
	, coreAPI(coreAPI)
{}

void StatsView::draw(RenderContext& context)
{
	timer.beginSample();
	
	const auto viewPort = Rect4f(context.getDefaultRenderTarget().getViewPort());
	const auto targetSize = Vector2f(1280, 720);
	const auto zoom2d = viewPort.getSize() / targetSize;
	const float zoom = std::min(zoom2d.x, zoom2d.y);

	auto camera = Camera(viewPort.getSize() / zoom * 0.5f).setZoom(zoom);
	context.with(camera).bind([&](Painter& painter) {
		paint(painter);
		painter.flush();
	});

	timer.endSample();
	drawing = true;
}

void StatsView::update()
{
	if (!drawing) {
		timer.beginSample();
		timer.endSample();
	}
	drawing = false;
}

void StatsView::setWorld(const World* world)
{
	this->world = world;
}

String StatsView::formatTime(int64_t ns) const
{
	const int64_t us = (ns + 500) / 1000;
	return toString(us / 1000) + "." + toString(us % 1000, 10, 3);
}
