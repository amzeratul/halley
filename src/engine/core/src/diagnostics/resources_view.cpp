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
	const auto size = Vector2f(viewPort.getWidth() / 2.5f, viewPort.getHeight() - border.y * 2);
	const auto origin = viewPort.getTopRight() + Vector2f(-border.x - size.x, border.y);
	const auto rect = Rect4f(origin, origin + size);
	
	whitebox.clone()
		.setPosition(rect.getTopLeft())
		.scaleTo(rect.getSize())
		.setColour(Colour4f(0, 0, 0, 0.5f))
		.draw(painter);

	struct Stats {
		String name;
		ResourceMemoryUsage usage;
		ResourceDesiredLoadState loadState;

		bool operator<(const Stats& other) const
		{
			if (loadState != other.loadState) {
				//return loadState < other.loadState;
			}
			return other.usage < usage;
		}
	};
	Vector<Stats> stats;
	HashMap<ResourceDesiredLoadState, ResourceMemoryUsage> usage;

	resources.ofType(AssetType::Texture).forEachResource([&] (const std::shared_ptr<Resource>& res) {
		auto& resource = dynamic_cast<AsyncResource&>(*res);
		stats += Stats{ resource.getAssetId(), resource.getMemoryUsage(), resource.getDesiredLoadState() };
		usage[stats.back().loadState] += stats.back().usage;
	});

	std::sort(stats.begin(), stats.end());

	const float lineHeight = text.getLineHeight();
	Vector2f startCursorPos = rect.getTopLeft() + Vector2f(5, 5);
	Vector2f cursorPos = startCursorPos;

	for (auto state: { ResourceDesiredLoadState::Load, ResourceDesiredLoadState::Preload, ResourceDesiredLoadState::PreloadLowPriority, ResourceDesiredLoadState::Stale }) {
		text
	        .setPosition(cursorPos)
	        .setText(toString(state) + ": " + String::prettySize(usage[state].getTotal()))
	        .setColour(getColour(state))
			.setAlignment(0.0f)
	        .draw(painter, rect);
		auto extents = text.getExtents();
		cursorPos += Vector2f(extents.x + 10, 0);
	}
	cursorPos = startCursorPos + Vector2f(0, lineHeight * 1.5f);

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

Colour4f ResourcesView::getColour(ResourceDesiredLoadState loadState) const
{
	switch (loadState) {
	case ResourceDesiredLoadState::Load:
		return Colour4f::fromHexString("#00FF80");
	case ResourceDesiredLoadState::Preload:
		return Colour4f::fromHexString("#00FFFF");
	case ResourceDesiredLoadState::PreloadLowPriority:
		return Colour4f::fromHexString("#0080FF");
	case ResourceDesiredLoadState::Stale:
		return Colour4f::fromHexString("#FFFF00");
	case ResourceDesiredLoadState::Unload:
		return Colour4f::fromHexString("#FF0000");
	default:
		return Colour4f::fromHexString("#000000");
	}
}
