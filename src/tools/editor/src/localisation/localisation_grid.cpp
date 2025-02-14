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
	textCol = factory.getColourScheme()->getColour("ui_text");
	outdatedCol = factory.getColourScheme()->getColour("ui_logWarningText");

	text = TextRenderer()
		.setFont(factory.getResources().get<Font>("Ubuntu Regular"))
		.setSize(fontSize)
		.setColour(textCol);

	setInteractWithMouse(true);
}

void LocalisationGrid::update(Time t, bool moved)
{
	if (scrollCooldown >= 0) {
		scrollCooldown -= t;
	}
}

namespace {
	std::optional<Colour4f> blendColours(std::optional<Colour4f> bottom, std::optional<Colour4f> top)
	{
		if (bottom && top) {
			return top->over(*bottom);
		} else if (bottom) {
			return bottom;
		} else if (top) {
			return top;
		} else {
			return {};
		}
	}

	std::optional<Colour4f> blendColours(std::optional<Colour4f> col0, std::optional<Colour4f> col1, std::optional<Colour4f> col2)
	{
		return blendColours(blendColours(col0, col1), col2);
	}
}

void LocalisationGrid::draw(UIPainter& painter) const
{
	const auto clip = painter.getClip();
	drawClip = clip;
	const Rect4f relClip = (clip ? *clip : getRect()) - getPosition();

	const auto n = static_cast<int>(origData ? origData->getNumEntries() : 0);
	const auto firstLine = clamp(static_cast<int>(std::floor(relClip.getTop() / lineHeight)) - 1, 0, n - 1);
	const auto lastLine = clamp(static_cast<int>(std::ceil(relClip.getBottom() / lineHeight)) - 1, 0, n - 1);

	Vector<float> columns;
	Vector<String> columnNames;

	const float width = getSize().x - 1;
	const float numWidth = 40;
	const float keyWidth = 250;
	const float priorityWidth = 30;
	const float commentWidth = 30;
	const float contextWidth = 30;
	const float fixedWidth = numWidth + keyWidth + priorityWidth + commentWidth + contextWidth;
	columns.push_back(numWidth);
	columnNames.push_back("#");
	columns.push_back(keyWidth);
	columnNames.push_back("Key");

	const float dynamicWidth = std::floor((width - fixedWidth) / (translatedData && origData ? 2 : 1));
	if (origData) {
		columns.push_back(dynamicWidth);
		columnNames.push_back("Original");
	}
	if (translatedData) {
		columns.push_back(dynamicWidth);
		columnNames.push_back("Translated");
	}

	columns.push_back(priorityWidth);
	columnNames.push_back("Pri");
	columns.push_back(commentWidth);
	columnNames.push_back("Com");
	columns.push_back(contextWidth);
	columnNames.push_back("Ctx");

	// Entries
	auto p2 = painter.withClip(relClip.grow(0, -lineHeight - 1, 0, 0) + getPosition());
	for (int i = firstLine; i <= lastLine; ++i) {
		drawLine(p2, i, columns);
	}

	// Draw grid
	const auto gridCol = factory.getColourScheme()->getColour("ui_text");
	p2.draw([firstLine, lastLine, pos = getPosition(), columns, size = getSize(), gridCol, this](Painter& painter)
	{
		// Backgrounds
		std::optional<Rect4f> drewSelectedLine;
		for (int i = firstLine; i <= lastLine + 1; ++i) {
			std::optional<Colour4f> col0 = colours[i];
			std::optional<Colour4f> col1;
			std::optional<Colour4f> col2;

			const auto linePos = pos + Vector2f(0, static_cast<float>(i + 1) * lineHeight);
			const auto rect = Rect4f(linePos, size.x, lineHeight);

			if (i % 2 == 0) {
				col1 = Colour4f(0, 0, 0).withAlpha(0.1f);
			}
			if (selectedLines.contains(i)) {
				col2 = Colour4f(1, 1, 1, 0.2f);
			} else if (lineUnderMouse == i) {
				col2 = Colour4f(1, 1, 1, 0.1f);
			}
			if (activeSelectedLine == i) {
				drewSelectedLine = rect;
			}

			if (const auto col = blendColours(col0, col1, col2)) {
				painter.drawPolygon(Polygon(rect), *col);
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
	drawLine(painter, relClip.getTopLeft() + getPosition(), columns, columnNames.const_span(), {});
}

void LocalisationGrid::drawLine(UIPainter& painter, int idx, const Vector<float>& columns) const
{
	const auto basePos = getPosition() + Vector2f(0, static_cast<float>(idx + 1) * lineHeight);

	Vector<String> strs;
	Vector<Colour4f> colours;

	if (origData) {
		const auto& entry = origData->getEntry(idx);
		strs.push_back(toString(idx + 1));
		strs.push_back(entry.key);
		strs.push_back(entry.value);
		colours.resize(3, textCol);

		if (translatedData) {
			if (auto* translatedEntry = translatedData->tryGetEntry(entry.key)) {
				strs.push_back(translatedEntry->value);
				colours.push_back(entry.version == translatedEntry->origVersion ? textCol : outdatedCol);
			}
		}
	}

	drawLine(painter, basePos, columns, strs.const_span(), colours.const_span());
}

void LocalisationGrid::drawLine(UIPainter& painter, Vector2f pos, gsl::span<const float> columns, gsl::span<const String> strings, gsl::span<const Colour4f> colours) const
{
	float curPos = 0;

	auto drawColumn = [&] (float width, const String& str, std::optional<Colour4f> col)
	{
		auto t = text.clone()
			.setPosition(pos + Vector2f(curPos + cellBorder, cellBorder))
			.setText(str)
			.setClip(Rect4f(0, 0, width - 2 * cellBorder, lineHeight));

		if (col) {
			t.setColour(*col);
		}

		painter.draw(std::move(t));

		curPos += width;
	};

	for (size_t i = 0; i < strings.size(); ++i) {
		drawColumn(columns[i], strings[i], colours.size() > i ? std::optional(colours[i]) : std::nullopt);
	}
}

void LocalisationGrid::setData(const ILocOriginalData* origData, LocTranslationData* translatedData)
{
	this->origData = origData;
	this->translatedData = translatedData;

	const int numLines = origData ? static_cast<int>(origData->getNumEntries()) : 0;

	setMinSize(Vector2f(0, lineHeight * (static_cast<float>(numLines + 2))));
	setSelectedLine(0);

	colours.resize(numLines);
	if (lineColourFilter) {
		for (int i = 0; i < numLines; ++i) {
			colours[i] = lineColourFilter(i);
		}
	}
}

void LocalisationGrid::setLineColourFilter(LineColourCallback callback)
{
	lineColourFilter = std::move(callback);
}

int LocalisationGrid::getActiveSelectedLine() const
{
	return activeSelectedLine.value_or(-1);
}

const String& LocalisationGrid::getActiveSelectedKey() const
{
	if (origData && activeSelectedLine) {
		return origData->getEntry(*activeSelectedLine).key;
	} else {
		return String::emptyString();
	}
}

void LocalisationGrid::setSelectedLine(int line)
{
	onClickLine(line, KeyMods::None);
}

void LocalisationGrid::onMouseOver(Vector2f mousePos)
{
	const auto nLines = static_cast<int>(origData->getNumEntries());
	const auto line = static_cast<int>((mousePos.y - getPosition().y) / lineHeight) - 1;
	boundedLineUnderMouse = nLines >= 0 ? std::optional<int>(clamp(line, 0, nLines - 1)) : std::nullopt;

	if (line != boundedLineUnderMouse) {
		lineUnderMouse = {};
	} else {
		lineUnderMouse = line;
	}

	if (holdingLine && boundedLineUnderMouse != holdingLine) {
		holdingMoved = true;
	}

	if (holdingLine && boundedLineUnderMouse && holdingMoved) {
		selectedLines.clear();
		const int start = std::min(*holdingLine, *boundedLineUnderMouse);
		const int end = std::max(*holdingLine, *boundedLineUnderMouse);
		for (int i = start; i <= end; ++i) {
			selectedLines.insert(i);
		}
	}

	if (scrollCooldown <= 0 && drawClip) {
		const auto clipRect = *drawClip;
		const auto rect = clipRect - getPosition();
		constexpr auto scrollDelta = 20.0f;
		constexpr auto cooldown = 0.016;
		if (mousePos.y < clipRect.getTop()) {
			sendEvent(UIEvent(UIEventType::MakeAreaVisible, getId(), Rect4f(rect.getTopLeft() - Vector2f(0, scrollDelta), rect.getTopRight())));
			scrollCooldown = cooldown;
		} else if (mousePos.y > clipRect.getBottom()) {
			sendEvent(UIEvent(UIEventType::MakeAreaVisible, getId(), Rect4f(rect.getBottomLeft(), rect.getBottomRight() + Vector2f(0, scrollDelta))));
			scrollCooldown = cooldown;
		}
	}
}

void LocalisationGrid::onMouseLeft(Vector2f mousePos)
{
	lineUnderMouse = std::nullopt;
}

void LocalisationGrid::pressMouse(Vector2f mousePos, int button, KeyMods keyMods)
{
	if (button == 0) {
		onClickLine(lineUnderMouse.value_or(-1), keyMods);
		holdingLine = lineUnderMouse;
		holdingMoved = false;
		focus();
	}
}

void LocalisationGrid::releaseMouse(Vector2f mousePos, int button)
{
	if (button == 0) {
		holdingLine = {};
		holdingMoved = false;
	}
}

bool LocalisationGrid::isFocusLocked() const
{
	return holdingLine.has_value();
}

bool LocalisationGrid::canReceiveFocus() const
{
	return true;
}

void LocalisationGrid::onClickLine(std::optional<int> line, KeyMods mods)
{
	if (!origData || line < 0 || line >= static_cast<int>(origData->getNumEntries())) {
		line = {};
	}

	const auto prevActive = activeSelectedLine;

	if (mods == KeyMods::None || (mods == KeyMods::Shift && !activeSelectedLine)) {
		selectedLines.clear();
		if (line) {
			selectedLines.insert(*line);
		}
		activeSelectedLine = line;
	} else if (mods == KeyMods::Ctrl) {
		if (line) {
			if (selectedLines.contains(*line)) {
				selectedLines.erase(*line);
			} else {
				selectedLines.insert(*line);
				activeSelectedLine = line;
			}
		}
	} else if (mods == KeyMods::Shift) {
		if (line) {
			selectedLines.clear();
			int start = std::min(*line, *activeSelectedLine);
			int end = std::max(*line, *activeSelectedLine);
			for (int i = start; i <= end; ++i) {
				selectedLines.insert(i);
			}
		}
	}

	if (activeSelectedLine != prevActive) {
		auto id = activeSelectedLine ? origData->getEntry(*activeSelectedLine).key : "";
		sendEvent(UIEvent(UIEventType::ListSelectionChanged, getId(), id, activeSelectedLine.value_or(-1)));
	}

}

