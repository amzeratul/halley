#include "diagnostics/stats_view.h"

using namespace Halley;

StatsView::StatsView(Resources& resources, CoreAPI& coreAPI)
	: resources(resources)
	, coreAPI(coreAPI)
{}

void StatsView::setWorld(const World* world)
{
	this->world = world;
}

String StatsView::formatTime(int64_t ns) const
{
	const int64_t us = (ns + 500) / 1000;
	return toString(us / 1000) + "." + toString(us % 1000, 10, 3);
}
