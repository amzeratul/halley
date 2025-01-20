#include "localisation_grid.h"

using namespace Halley;

namespace {
	constexpr float lineHeight = 18;
}

LocalisationGrid::LocalisationGrid(UIFactory& factory)
	: factory(factory)
{
	const auto col = factory.getColourScheme()->getColour("ui_text");

	text = TextRenderer()
		.setFont(factory.getResources().get<Font>("Ubuntu Regular"))
		.setSize(14)
		.setColour(col);
}

void LocalisationGrid::update(Time t, bool moved)
{

}

void LocalisationGrid::draw(UIPainter& painter) const
{
	const auto clip = painter.getClip();
	auto relClip = clip;
	if (relClip) {
		*relClip -= getPosition();
	}

	const auto n = static_cast<int>(curData->entries.size());
	const auto firstLine = clamp(relClip ? static_cast<int>(std::floor(relClip->getTop() / lineHeight)) : 0, 0, n - 1);
	const auto lastLine = clamp(relClip ? static_cast<int>(std::ceil(relClip->getBottom() / lineHeight)) : n - 1, 0, n - 1);

	Logger::logInfo("Drawing from " + toString(firstLine) + " to " + lastLine);

	for (int i = firstLine; i <= lastLine; ++i) {
		drawLine(painter, i);
	}
}

void LocalisationGrid::drawLine(UIPainter& painter, int idx) const
{
	const auto basePos = getPosition() + Vector2f(0, static_cast<float>(idx) * lineHeight);

	float curPos = 0;

	auto drawColumn = [&] (float width, String str)
	{
		auto t = text.clone()
			.setPosition(basePos + Vector2f(curPos, 0))
			.setText(str)
			.setClip(basePos + Rect4f(curPos, 0, curPos + width, lineHeight));

		painter.draw(std::move(t));

		curPos += width;
	};

	const float width = getSize().x;
	const float dynamicWidth = (width - 100) / (curData && origData ? 2 : 1);

	if (curData) {
		const auto& curEntry = curData->entries.at(idx);
		drawColumn(100, curEntry.key);
		drawColumn(dynamicWidth, curEntry.values.at(0).value);
	}

	if (origData) {
		const auto& origEntry = origData->entries.at(idx);
		drawColumn(dynamicWidth, origEntry.values.at(0).value);
	}
}

void LocalisationGrid::setData(const LocalisationDataChunk* origData, LocalisationDataChunk* curData)
{
	this->origData = origData;
	this->curData = curData;

	setMinSize(Vector2f(0, lineHeight * static_cast<float>(curData ? static_cast<int>(curData->entries.size()) : 0)));
}

