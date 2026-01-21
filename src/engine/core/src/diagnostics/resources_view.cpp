#include "halley/diagnostics/resources_view.h"

#include "halley/graphics/painter.h"
#include "halley/resources/resources.h"

using namespace Halley;

ResourcesView::ResourcesView(Resources& resources, const HalleyAPI& api)
	: StatsView(resources, api)
	, whitebox(Sprite().setImage(resources, "whitebox.png"))
{
	text = TextRenderer(resources.get<Font>("Ubuntu Bold"), "", 16, Colour(1, 1, 1), 1.0f, Colour(0.1f, 0.1f, 0.1f));
}

void ResourcesView::update(Time t)
{
	StatsView::update(t);
}

void ResourcesView::paint(Painter& painter)
{
	if (!active) {
		return;
	}
	
	const auto viewPort = Rect4f(painter.getViewPort());
	const auto border = Vector2f(0, 0);
	const auto size = Vector2f(600.0f, viewPort.getHeight() - border.y * 2);
	const auto origin = viewPort.getTopRight() + Vector2f(-border.x - size.x, border.y);
	const auto rect = Rect4f(origin, origin + size);
	
	whitebox.clone()
		.setPosition(rect.getTopLeft())
		.scaleTo(rect.getSize())
		.setColour(Colour4f(0, 0, 0, 0.5f))
		.draw(painter);

	const auto stats = getStats(assetType);

	drawSummary(painter, rect, stats);
	drawStats(painter, rect, stats);
}

Colour4f ResourcesView::getColour(ResourceDesiredLoadState loadState) const
{
	switch (loadState) {
	case ResourceDesiredLoadState::Load:
		return Colour4f::fromHexString("#00FF80");
	case ResourceDesiredLoadState::Preload:
	case ResourceDesiredLoadState::PreloadLowPriority:
		return Colour4f::fromHexString("#00D0FF");
	case ResourceDesiredLoadState::Stale:
		return Colour4f::fromHexString("#FFFF00");
	case ResourceDesiredLoadState::Unload:
		return Colour4f::fromHexString("#FF0000");
	default:
		return Colour4f::fromHexString("#FFFFFF");
	}
}

Vector<ResourcesView::Stats> ResourcesView::getStats(AssetType type) const
{
	Vector<Stats> stats;

	resources.ofType(type).forEachResource([&] (const std::shared_ptr<Resource>& res) {
		auto& resource = dynamic_cast<AsyncResource&>(*res);
		auto state = resource.getDesiredLoadState();
		if (state == ResourceDesiredLoadState::PreloadLowPriority) {
			state = ResourceDesiredLoadState::Preload;
		}
		stats += Stats{ resource.getAssetId(), resource.getMemoryUsage(), state };
	});

	std::sort(stats.begin(), stats.end());

	return stats;
}

void ResourcesView::drawSummary(Painter& painter, Rect4f rect, const Vector<Stats>& stats) const
{
	HashMap<ResourceDesiredLoadState, ResourceMemoryUsage> usagePerState;
	for (const auto& stat: stats) {
		usagePerState[stat.loadState] += stat.usage;
		usagePerState[ResourceDesiredLoadState::Undefined] += stat.usage;
	}

	auto states = std::array<ResourceDesiredLoadState, 4>{
		ResourceDesiredLoadState::Undefined,
		ResourceDesiredLoadState::Load,
		ResourceDesiredLoadState::Preload,
		ResourceDesiredLoadState::Stale
	};
	auto stateNames = std::array<String, 4> {
		"Total",
		"Use",
		"Preloaded",
		"Stale"
	};

	const float lineHeight = text.getLineHeight();
	Vector2f startCursorPos = rect.getTopLeft() + Vector2f(5, 5);
	Vector2f cursorPos = startCursorPos;

	text
        .setPosition(cursorPos)
        .setText("Memory usage for " + toString(assetType))
        .setColour(Colour4f::fromHexString("#FFFFFF"))
		.setAlignment(0.0f)
        .draw(painter, rect);
	cursorPos += Vector2f(0, lineHeight);

	for (int i = 0; i < static_cast<int>(states.size()); ++i) {
		auto state = states[i];
		text
	        .setPosition(cursorPos)
	        .setText(stateNames[i] + ": " + String::prettySize(usagePerState[state].getTotal()))
	        .setColour(getColour(state))
			.setAlignment(0.0f)
	        .draw(painter, rect);
		auto extents = text.getExtents();
		cursorPos += Vector2f(extents.x + 10, 0);
	}
}

void ResourcesView::drawStats(Painter& painter, Rect4f rect, const Vector<Stats>& stats) const
{
	const float lineHeight = text.getLineHeight();
	const Vector2f startCursorPos = rect.getTopLeft() + Vector2f(5, 15 + 2 * lineHeight);
	Vector2f cursorPos = startCursorPos;
	
	for (const auto& stat: stats) {
		const auto colour = getColour(stat.loadState);

		text
            .setPosition(cursorPos + Vector2f(80, 0))
            .setText(String::prettySize(stat.usage.getTotal()))
            .setColour(colour)
			.setAlignment(1.0f)
            .draw(painter, rect);
		
		text
			.setPosition(cursorPos + Vector2f(90, 0))
			.setText(stat.name)
			.setColour(colour)
			.setAlignment(0.0f)
			.draw(painter, rect);

		cursorPos += Vector2f(0, lineHeight);

		if (cursorPos.y > rect.getBottom()) {
			break;
		}
	}
}
