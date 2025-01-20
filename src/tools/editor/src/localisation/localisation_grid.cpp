#include "localisation_grid.h"

using namespace Halley;

namespace {
	constexpr float cellBorder = 3;
	constexpr float lineHeight = 16 + 2 * cellBorder;
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
	const Rect4f relClip = (clip ? *clip : getRect()) - getPosition();

	const auto n = static_cast<int>(curData ? curData->entries.size() : 0);
	const auto firstLine = clamp(static_cast<int>(std::floor(relClip.getTop() / lineHeight)) - 1, 0, n - 1);
	const auto lastLine = clamp(static_cast<int>(std::ceil(relClip.getBottom() / lineHeight)) - 1, 0, n - 1);

	Vector<float> columns;
	const float width = getSize().x - 1;
	const float dynamicWidth = std::floor((width - 100) / (curData && origData ? 2 : 1));
	columns.push_back(35);
	columns.push_back(200);
	columns.push_back(dynamicWidth);
	if (origData) {
		columns.push_back(dynamicWidth);
	}

	// Entries
	auto p2 = painter.withClip(relClip.grow(0, -lineHeight - 1, 0, 0) + getPosition());
	for (int i = firstLine; i <= lastLine; ++i) {
		drawLine(p2, i, columns);
	}

	// Draw grid
	const auto gridCol = factory.getColourScheme()->getColour("ui_text").withAlpha(0.1f);
	p2.draw([firstLine, lastLine, pos = getPosition(), columns, size = getSize(), gridCol](Painter& painter)
	{
		// Backgrounds
		for (int i = firstLine; i <= lastLine + 1; ++i) {
			if (i % 2 == 0) {
				const auto linePos = pos + Vector2f(0, static_cast<float>(i + 1) * lineHeight);
				painter.drawPolygon(Polygon(Rect4f(linePos, size.x, lineHeight)), Colour4f(0, 0, 0).withAlpha(0.1f));
			}
		}

		// Horizontal lines
		for (int i = firstLine; i <= lastLine + 1; ++i) {
			const auto linePos = pos + Vector2f(0, static_cast<float>(i + 1) * lineHeight);
			painter.drawLine(LineSegment(linePos, linePos + Vector2f(size.x, 0)), 1, gridCol);
		}

		// Vertical lines
		{
			const auto linePos0 = pos + Vector2f(0, static_cast<float>(firstLine + 1) * lineHeight);
			const auto linePos1 = pos + Vector2f(0, static_cast<float>(lastLine + 2) * lineHeight);
			float curPos = 0;
			for (int i = -1; i < static_cast<int>(columns.size()); ++i) {
				curPos += i >= 0 ? columns[i] : 0;
				auto hPos = Vector2f(std::min(curPos, size.x - 1), 0.0f);
				painter.drawLine(LineSegment(linePos0 + hPos, linePos1 + hPos), 1, gridCol);
			}
		}
	});

	// Draw header
	const auto headerCol = Colour4f::fromHexString("#BD40B0");
	painter.draw([pos = getPosition(), columns, size = getSize(), headerCol, relClip](Painter& painter)
	{
		const auto linePos0 = relClip.getTopLeft() + pos;
		const auto linePos1 = linePos0 + Vector2f(0, lineHeight);

		// Background
		painter.drawPolygon(Polygon(Rect4f(linePos0, size.x, lineHeight)), headerCol.withAlpha(0.25f));

		// Horizontal lines
		painter.drawLine(LineSegment(linePos0, linePos0 + Vector2f(size.x, 0)), 1, headerCol.withAlpha(0.5f));
		painter.drawLine(LineSegment(linePos1, linePos1 + Vector2f(size.x, 0)), 1, headerCol.withAlpha(0.5f));

		// Vertical lines
		{
			float curPos = 0;
			for (int i = -1; i < static_cast<int>(columns.size()); ++i) {
				curPos += i >= 0 ? columns[i] : 0;
				auto hPos = Vector2f(std::min(curPos, size.x - 1), 0.0f);
				painter.drawLine(LineSegment(linePos0 + hPos, linePos1 + hPos), 1, headerCol.withAlpha(0.5f));
			}
		}
	});

	// Header text
	drawLine(painter, relClip.getTopLeft() + getPosition(), columns, Vector<String>{ "#", "Key", "Original", "Translated" }.const_span().subspan(0, columns.size()));
}

void LocalisationGrid::drawLine(UIPainter& painter, int idx, const Vector<float>& columns) const
{
	const auto basePos = getPosition() + Vector2f(0, static_cast<float>(idx + 1) * lineHeight);

	if (curData) {
		const auto& curEntry = curData->entries.at(idx);

		Vector<String> strs;
		strs.push_back(toString(idx + 1));
		strs.push_back(curEntry.key);
		if (origData) {
			const auto& origEntry = origData->entries.at(idx);
			strs.push_back(origEntry.values.at(0).value);
		}
		strs.push_back(curEntry.values.at(0).value);

		drawLine(painter, basePos, columns, strs.const_span());
	}
}

void LocalisationGrid::drawLine(UIPainter& painter, Vector2f pos, gsl::span<const float> columns, gsl::span<const String> strings) const
{
	float curPos = 0;

	auto drawColumn = [&] (float width, const String& str)
	{
		auto t = text.clone()
			.setPosition(pos + Vector2f(curPos + cellBorder, cellBorder))
			.setText(str)
			.setClip(Rect4f(0, 0, width - 2 * cellBorder, lineHeight - 2 * cellBorder));

		painter.draw(std::move(t));

		curPos += width;
	};

	for (size_t i = 0; i < strings.size(); ++i) {
		drawColumn(columns[i], strings[i]);
	}
}

void LocalisationGrid::setData(const LocalisationDataChunk* origData, LocalisationDataChunk* curData)
{
	this->origData = origData;
	this->curData = curData;

	setMinSize(Vector2f(0, lineHeight * (static_cast<float>(curData ? static_cast<int>(curData->entries.size()) : 0) + 2)));
}

