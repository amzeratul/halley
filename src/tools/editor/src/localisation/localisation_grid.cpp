#include "localisation_grid.h"

using namespace Halley;

namespace {
	constexpr float fontSize = 13;
	constexpr float cellBorder = 3;
	constexpr float lineHeight = fontSize + 2 + 2 * cellBorder;
}

LocalisationGrid::LocalisationGrid(UIFactory& factory)
	: UIWidget("localisation_grid")
	, factory(factory)
{
	const auto col = factory.getColourScheme()->getColour("ui_text");

	text = TextRenderer()
		.setFont(factory.getResources().get<Font>("Ubuntu Regular"))
		.setSize(fontSize)
		.setColour(col);

	setInteractWithMouse(true);
}

void LocalisationGrid::update(Time t, bool moved)
{

}

void LocalisationGrid::draw(UIPainter& painter) const
{
	const auto clip = painter.getClip();
	const Rect4f relClip = (clip ? *clip : getRect()) - getPosition();

	const auto n = static_cast<int>(origData ? origData->entries.size() : 0);
	const auto firstLine = clamp(static_cast<int>(std::floor(relClip.getTop() / lineHeight)) - 1, 0, n - 1);
	const auto lastLine = clamp(static_cast<int>(std::ceil(relClip.getBottom() / lineHeight)) - 1, 0, n - 1);

	Vector<float> columns;
	const float width = getSize().x - 1;
	const float dynamicWidth = std::floor((width - 100) / (translatedData && origData ? 2 : 1));
	columns.push_back(40);
	columns.push_back(250);
	if (origData) {
		columns.push_back(dynamicWidth);
	}
	if (translatedData) {
		columns.push_back(dynamicWidth);
	}

	// Entries
	auto p2 = painter.withClip(relClip.grow(0, -lineHeight - 1, 0, 0) + getPosition());
	for (int i = firstLine; i <= lastLine; ++i) {
		drawLine(p2, i, columns);
	}

	// Draw grid
	const auto gridCol = factory.getColourScheme()->getColour("ui_text");
	p2.draw([firstLine, lastLine, pos = getPosition(), columns, size = getSize(), gridCol, lineUnderMouse = lineUnderMouse, selectedLine = selectedLine](Painter& painter)
	{
		// Backgrounds
		std::optional<Rect4f> drewSelectedLine;
		for (int i = firstLine; i <= lastLine + 1; ++i) {
			if (i % 2 == 0 || lineUnderMouse == i || selectedLine == i) {
				const auto linePos = pos + Vector2f(0, static_cast<float>(i + 1) * lineHeight);
				const auto rect = Rect4f(linePos, size.x, lineHeight);

				auto col = Colour4f(0, 0, 0).withAlpha(0.1f);
				if (selectedLine == i) {
					col = Colour4f(1, 1, 1, 0.2f);
					drewSelectedLine = rect;
				} else if (lineUnderMouse == i) {
					col = Colour4f(1, 1, 1, 0.1f);
				}

				painter.drawPolygon(Polygon(rect), col);
			}
		}

		// Horizontal lines
		for (int i = firstLine; i <= lastLine + 1; ++i) {
			const auto linePos = pos + Vector2f(0, static_cast<float>(i + 1) * lineHeight);
			painter.drawLine(LineSegment(linePos, linePos + Vector2f(size.x, 0)), 1, gridCol.withAlpha(0.1f));
		}

		// Vertical lines
		{
			const auto linePos0 = pos + Vector2f(0, static_cast<float>(firstLine + 1) * lineHeight);
			const auto linePos1 = pos + Vector2f(0, static_cast<float>(lastLine + 2) * lineHeight);
			float curPos = 0;
			for (int i = -1; i < static_cast<int>(columns.size()); ++i) {
				curPos += i >= 0 ? columns[i] : 0;
				auto hPos = Vector2f(std::min(curPos, size.x - 1), 0.0f);
				painter.drawLine(LineSegment(linePos0 + hPos, linePos1 + hPos), 1, gridCol.withAlpha(0.1f));
			}
		}

		// Selection
		if (drewSelectedLine) {
			painter.drawRect(*drewSelectedLine, 1.0f, gridCol.withAlpha(0.5f));
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

	Vector<String> strs;

	if (origData) {
		const auto& entry = origData->entries.at(idx);
		strs.push_back(toString(idx + 1));
		strs.push_back(entry.key);
		strs.push_back(entry.value);
	}

	if (translatedData) {
		const auto& entry = translatedData->entries.at(idx);
		strs.push_back(entry.value);
	}

	drawLine(painter, basePos, columns, strs.const_span());
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

void LocalisationGrid::setData(LocalisationDataChunk* origData, LocalisationDataChunk* translatedData)
{
	if (origData && translatedData) {
		translatedData->alignWith(*origData);
	}

	this->origData = origData;
	this->translatedData = translatedData;

	setMinSize(Vector2f(0, lineHeight * (static_cast<float>(origData ? static_cast<int>(origData->entries.size()) : 0) + 2)));
	setSelectedLine(0);
}

int LocalisationGrid::getSelectedLine() const
{
	return selectedLine.value_or(-1);
}

void LocalisationGrid::setSelectedLine(int line)
{
	if (!origData || line < 0 || line >= static_cast<int>(origData->entries.size())) {
		selectedLine = {};
	} else {
		selectedLine = line;
	}
}

const String& LocalisationGrid::getSelectedKey() const
{
	if (origData && selectedLine) {
		return origData->entries.at(*selectedLine).key;
	} else {
		return String::emptyString();
	}
}

void LocalisationGrid::onMouseOver(Vector2f mousePos)
{
	const auto line = static_cast<int>((mousePos.y - getPosition().y) / lineHeight) - 1;
	if (!origData || line < 0 || line >= static_cast<int>(origData->entries.size())) {
		lineUnderMouse = {};
	} else {
		lineUnderMouse = line;
	}
}

void LocalisationGrid::onMouseLeft(Vector2f mousePos)
{
	lineUnderMouse = std::nullopt;
}

void LocalisationGrid::pressMouse(Vector2f mousePos, int button, KeyMods keyMods)
{
	if (button == 0) {
		if (selectedLine != lineUnderMouse) {
			selectedLine = lineUnderMouse;
			auto id = selectedLine ? origData->entries[*selectedLine].key : "";
			sendEvent(UIEvent(UIEventType::ListSelectionChanged, getId(), id, selectedLine.value_or(-1)));
		}
	}
}

void LocalisationGrid::releaseMouse(Vector2f mousePos, int button)
{
}

