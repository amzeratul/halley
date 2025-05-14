#include "halley/ui/widgets/ui_grid.h"

#include "halley/graphics/painter.h"
#include "halley/maths/polygon.h"
#include "halley/ui/ui_factory.h"

using namespace Halley;

UIGrid::UIGrid(String id, UIFactory& factory)
	: UIWidget(std::move(id))
	, factory(factory)
{
	textCol = factory.getColourScheme()->getColour("ui_text");

	text = TextRenderer()
		.setFont(factory.getResources().get<Font>("Ubuntu Regular"))
		.setSize(fontSize)
		.setColour(textCol);

	setInteractWithMouse(true);
}

void UIGrid::update(Time t, bool moved)
{
	if (scrollCooldown >= 0) {
		scrollCooldown -= t;
	}

	std::tie(columns, columnNames) = getColumns();

	columnUnderMouse = std::nullopt;
	if (lastMousePos) {
		const auto px = (*lastMousePos - getPosition()).x;
		float accum = 0;
		for (int i = 0; i < static_cast<int>(columns.size()); ++i) {
			accum += columns[i];
			if (accum > px) {
				columnUnderMouse = i;
				break;
			}
		}
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

void UIGrid::draw(UIPainter& painter) const
{
	const auto clip = painter.getClip();
	drawClip = clip;
	const Rect4f relClip = (clip ? *clip : getRect()) - getPosition();

	const float lineHeight = getLineHeight();
	const auto n = static_cast<int>(getNumRows());
	const auto firstLine = clamp(static_cast<int>(std::floor(relClip.getTop() / lineHeight)) - 1, 0, n - 1);
	const auto lastLine = clamp(static_cast<int>(std::ceil(relClip.getBottom() / lineHeight)) - 1, 0, n - 1);

	// Entries
	auto p2 = painter.withClip(relClip.grow(0, -lineHeight - 1, 0, 0) + getPosition());
	for (int i = firstLine; i <= lastLine; ++i) {
		drawLine(p2, i, columns);
	}

	// Draw grid
	const auto gridCol = factory.getColourScheme()->getColour("ui_text");
	p2.draw([firstLine, lastLine, pos = getPosition(), columns = columns, size = getSize(), gridCol, lineHeight, this](Painter& painter)
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
	painter.draw([pos = getPosition(), columns = columns, size = getSize(), headerCol, relClip, lineHeight](Painter& painter)
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
	drawLine(painter, relClip.getTopLeft() + getPosition(), columns, columnNames.const_span(), {}, {});
}

void UIGrid::drawLine(UIPainter& painter, int idx, const Vector<float>& columns) const
{
	Vector<String> strs;
	Vector<Colour4f> colours;
	Vector<Sprite> sprites;
	getLineDrawData(idx, strs, colours, sprites);
	drawLine(painter, getRowBasePos(idx), columns, strs.const_span(), colours.const_span(), sprites.const_span());
}

void UIGrid::drawLine(UIPainter& painter, Vector2f pos, gsl::span<const float> columns, gsl::span<const String> strings, gsl::span<const Colour4f> colours, gsl::span<const Sprite> sprites) const
{
	const float lineHeight = getLineHeight();
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

	auto drawSprite = [&] (float width, const Sprite& sprite) {
		const auto height = getLineHeight();
		const auto cellSize = Vector2f(width, height);
		painter.draw(sprite.clone(false).setPosition((pos + Vector2f(curPos, 0) + (cellSize - sprite.getUncroppedScaledSize()) / 2).floor()), true);
	};

	for (size_t i = 0; i < strings.size(); ++i) {
		if (i < sprites.size() && sprites[i].hasMaterial()) {
			drawSprite(columns[i], sprites[i]);
		}
		drawColumn(columns[i], strings[i], colours.size() > i ? std::optional(colours[i]) : std::nullopt);
	}
}

void UIGrid::setLineColourFilter(LineColourCallback callback)
{
	lineColourFilter = std::move(callback);
}

int UIGrid::getActiveSelectedLine() const
{
	return activeSelectedLine.value_or(-1);
}

const String& UIGrid::getActiveSelectedKey() const
{
	if (activeSelectedLine) {
		return getKeyAt(*activeSelectedLine);
	} else {
		return String::emptyString();
	}
}

void UIGrid::setSelectedLine(int line)
{
	onClickLine(line, KeyMods::None);
}

const HashSet<int>& UIGrid::getSelectedLines() const
{
	return selectedLines;
}

void UIGrid::onMouseOver(Vector2f mousePos)
{
	const float lineHeight = getLineHeight();
	const auto nLines = static_cast<int>(getNumRows());
	const auto line = static_cast<int>((mousePos.y - getPosition().y) / lineHeight) - 1;
	boundedLineUnderMouse = nLines >= 0 ? std::optional<int>(clamp(line, 0, nLines - 1)) : std::nullopt;

	if (line != boundedLineUnderMouse) {
		lineUnderMouse = {};
	} else {
		lineUnderMouse = line;
	}

	lastMousePos = mousePos;

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

void UIGrid::onMouseLeft(Vector2f mousePos)
{
	lineUnderMouse = std::nullopt;
	lastMousePos = std::nullopt;
}

void UIGrid::pressMouse(Vector2f mousePos, int button, KeyMods keyMods)
{
	if (button == 0) {
		onClickLine(lineUnderMouse.value_or(-1), keyMods);
		holdingLine = lineUnderMouse;
		holdingMoved = false;
		focus();
	}
}

void UIGrid::releaseMouse(Vector2f mousePos, int button)
{
	if (button == 0) {
		holdingLine = {};
		holdingMoved = false;
	}
}

bool UIGrid::isFocusLocked() const
{
	return holdingLine.has_value();
}

bool UIGrid::canReceiveFocus() const
{
	return true;
}

size_t UIGrid::getNumRows() const
{
	return 0;
}

void UIGrid::onClickLine(std::optional<int> line, KeyMods mods)
{
	auto lines = getNumRows();
	if (lines == 0 || line < 0 || line >= static_cast<int>(lines)) {
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
		auto id = activeSelectedLine ? getKeyAt(*activeSelectedLine) : "";
		sendEvent(UIEvent(UIEventType::ListSelectionChanged, getId(), id, activeSelectedLine.value_or(-1)));
	}

}

float UIGrid::getLineHeight() const
{
	return fontSize + 2 + 2 * cellBorder;
}

void UIGrid::onDataUpdated()
{
	const auto numLines = getNumRows();
	const float lineHeight = getLineHeight();

	setMinSize(Vector2f(0, lineHeight * (static_cast<float>(numLines + 2))));
	setSelectedLine(0);

	colours.resize(numLines);
	if (lineColourFilter) {
		for (int i = 0; i < numLines; ++i) {
			colours[i] = lineColourFilter(i);
		}
	}
}

std::pair<Vector<float>, Vector<String>> UIGrid::getColumns() const
{
	return { { 100.0f, 200.0f }, { "Key", "Value" } };
}

void UIGrid::getLineDrawData(int idx, Vector<String>& strs, Vector<Colour4f>& colours, Vector<Sprite>& sprites) const
{
}

Vector2f UIGrid::getRowBasePos(int row) const
{
	return getPosition() + Vector2f(0, static_cast<float>(row + 1) * getLineHeight());
}

Vector2f UIGrid::getCellBasePos(int row, int column) const
{
	float x = 0;
	for (int i = 0; i < std::min(column, static_cast<int>(columns.size())); ++i) {
		x += columns[i];
	}

	return getRowBasePos(row) + Vector2f(x, 0);
}

const String& UIGrid::getKeyAt(int idx) const
{
	return String::emptyString();
}
