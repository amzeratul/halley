#include "curve_editor.h"

#include "halley/core/graphics/painter.h"

using namespace Halley;

CurveEditor::CurveEditor(String id, UIStyle _style)
	: UIWidget(std::move(id))
{
	styles.push_back(std::move(_style));
	setInteractWithMouse(true);

	const auto& style = styles.back();
	background = style.getSprite("background");
	display = style.getSprite("display");
	lineColour = style.getColour("lineColour");
	gridLine = style.getSprite("gridLine");
	tooltipLabel = style.getTextRenderer("tooltipLabel");
}

void CurveEditor::update(Time time, bool moved)
{
	if (!dragging && !isMouseOver()) {
		curAnchor = {};
		mouseAnchor = {};
		tooltipLabel.setText("");
	}

	if (moved) {
		const auto drawArea = getDrawArea();
		background.setPosition(getPosition()).scaleTo(getSize());
		display.setPos(drawArea.getTopLeft()).scaleTo(drawArea.getSize());
	}
}

void CurveEditor::draw(UIPainter& painter) const
{
	painter.draw(background);
	painter.draw(display);

	// Horizontal grid
	const auto drawArea = getDrawArea();
	const auto vLine = gridLine.clone().scaleTo(Vector2f(1, drawArea.getHeight()));
	for (size_t i = 0; i <= nHorizontalDividers; ++i) {
		float t = static_cast<float>(i) / static_cast<float>(nHorizontalDividers);
		painter.draw(vLine.clone().setPosition(lerp(drawArea.getTopLeft(), drawArea.getTopRight(), t)), true);
	}

	// Vertical grid
	const auto hLine = gridLine.clone().scaleTo(Vector2f(drawArea.getWidth(), 1));
	for (size_t i = 0; i <= nVerticalDividers; ++i) {
		float t = static_cast<float>(i) / static_cast<float>(nVerticalDividers);
		painter.draw(hLine.clone().setPosition(lerp(drawArea.getTopLeft(), drawArea.getBottomLeft(), t)), true);
	}

	painter.draw([this] (Painter& painter)
	{
		Vector<Vector2f> ps;
		ps.resize(points.size());

		for (size_t i = 0; i < points.size(); ++i) {
			ps[i] = curveToMouseSpace(points[i]);
		}

		painter.drawLine(ps, 1.0f, lineColour, false);

		for (size_t i = 0; i < points.size(); ++i) {
			drawAnchor(painter, ps[i], curAnchor == i);
		}

		if (mouseAnchor) {
			painter.drawCircle(curveToMouseSpace(*mouseAnchor), 1.5f, 3.0f, lineColour);
		}
	});
	painter.draw(tooltipLabel);
}

void CurveEditor::setHorizontalRange(Range<float> range)
{
	horizontalRange = range;
	normalizePoints();
}

Range<float> CurveEditor::getHorizontalRange() const
{
	return horizontalRange;
}

void CurveEditor::setHorizontalDividers(size_t n)
{
	nHorizontalDividers = n;
}

void CurveEditor::setVerticalDividers(size_t n)
{
	nVerticalDividers = n;
}

void CurveEditor::setPoints(Vector<Vector2f> pts)
{
	points = std::move(pts);
	normalizePoints();
}

const Vector<Vector2f>& CurveEditor::getPoints() const
{
	return points;
}

Vector<Vector2f>& CurveEditor::getPoints()
{
	return points;
}

void CurveEditor::setChangeCallback(Callback callback)
{
	this->callback = std::move(callback);
}

void CurveEditor::onMouseOver(Vector2f mousePos)
{
	if (!dragging) {
		curAnchor = getAnchorAt(mousePos);
	}
	updateDragging(mousePos);

	if (curAnchor) {
		mouseAnchor = {};
	} else {
		mouseAnchor = clampPoint(mouseToCurveSpace(mousePos));
	}

	const auto drawArea = getDrawArea();
	const bool left = mousePos.x > drawArea.getCenter().x;
	const auto curPos = curAnchor ? points[*curAnchor] : *mouseAnchor;
	tooltipLabel
		.setOffset(Vector2f(left ? 0.0f : 1.0f, 0.0f))
		.setPosition(left ? drawArea.getTopLeft() : drawArea.getTopRight())
		.setText(toString(curPos.x, 2) + ", " + toString(curPos.y, 2));
}

void CurveEditor::pressMouse(Vector2f mousePos, int button, KeyMods keyMods)
{
	if (button == 0) {
		const auto anchor = getAnchorAt(mousePos);
		curAnchor = anchor;
		if (anchor) {
			dragging = true;
			updateDragging(mousePos);
		} else {
			insertPoint(mouseToCurveSpace(mousePos));
		}
	}

	if (button == 2) {
		const auto anchor = getAnchorAt(mousePos);
		if (anchor) {
			deletePoint(anchor.value());
		}
	}
}

void CurveEditor::releaseMouse(Vector2f mousePos, int button)
{
	if (button == 0) {
		if (dragging) {
			dragging = false;
		}
	}
}

bool CurveEditor::isFocusLocked() const
{
	return dragging;
}

bool CurveEditor::canReceiveFocus() const
{
	return true;
}

void CurveEditor::normalizePoints()
{
	auto startPoints = points;

	if (points.empty()) {
		points.push_back(Vector2f(horizontalRange.start, 0));
		points.push_back(Vector2f(horizontalRange.end, 1));
	}
	if (points.size() == 1) {
		points.push_back(points.back());
	}
	points.front().x = horizontalRange.start;
	points.back().x = horizontalRange.end;

	for (auto& p: points) {
		p = clampPoint(p);
	}

	if (points != startPoints) {
		notifyChange();
	}
}

void CurveEditor::notifyChange()
{
	if (callback) {
		callback(points);
	}
}

Rect4f CurveEditor::getDrawArea() const
{
	return Rect4f(getPosition(), getPosition() + getSize()).shrink(5);
}

void CurveEditor::drawAnchor(Painter& painter, Vector2f pos, bool highlighted) const
{
	painter.drawCircle(pos, 2.0f, highlighted ? 5.0f : 4.0f, highlighted ? lineColour.inverseMultiplyLuma(0.5f) : lineColour);
}

Vector2f CurveEditor::curveToMouseSpace(Vector2f curvePos) const
{
	const auto drawArea = getDrawArea();
	const Vector2f curveOrigin = Vector2f(horizontalRange.start, 0);
	const Vector2f curveScale = Vector2f(1.0f / horizontalRange.getLength(), -1.0f) * drawArea.getSize();

	return (curvePos - curveOrigin) * curveScale + drawArea.getBottomLeft();
}

Vector2f CurveEditor::mouseToCurveSpace(Vector2f mousePos) const
{
	const auto drawArea = getDrawArea();
	const Vector2f curveOrigin = Vector2f(horizontalRange.start, 0);
	const Vector2f curveScale = Vector2f(1.0f / horizontalRange.getLength(), -1.0f) * drawArea.getSize();

	return (mousePos - drawArea.getBottomLeft()) / curveScale + curveOrigin;
}

std::optional<size_t> CurveEditor::getAnchorAt(Vector2f mousePos) const
{
	float bestDist = 6.0f;
	std::optional<size_t> bestIdx;

	for (size_t i = 0; i < points.size(); ++i) {
		const auto pos = curveToMouseSpace(points[i]);
		const float dist = (pos - mousePos).length();
		if (dist < bestDist) {
			bestDist = dist;
			bestIdx = i;
		}
	}

	return bestIdx;
}

Vector2f CurveEditor::clampPoint(Vector2f p) const
{
	p.x = clamp(p.x, horizontalRange.start, horizontalRange.end);
	p.y = clamp(p.y, 0.0f, 1.0f);
	return p;
}

void CurveEditor::insertPoint(Vector2f curvePos)
{
	const auto pos = clampPoint(curvePos);

	for (size_t i = 0; i < points.size() - 1; ++i) {
		if (points[i].x > pos.x) {
			points.insert(points.begin() + i, pos);
			curAnchor = i;
			dragging = true;
			notifyChange();
			return;
		}
	}

	const auto idx = points.size() - 1;
	points.insert(points.begin() + idx, pos);
	curAnchor = idx;
	dragging = true;
	notifyChange();
}

void CurveEditor::deletePoint(size_t idx)
{
	if (idx > 0 && idx < points.size() - 1) {
		points.erase(points.begin() + idx);
		notifyChange();
	}
}

void CurveEditor::updateDragging(Vector2f mousePos)
{
	if (dragging && curAnchor) {
		const auto idx = curAnchor.value();

		points[idx] = mouseToCurveSpace(mousePos);
		normalizePoints();

		if (idx > 0 && points[idx].x < points[idx - 1].x) {
			std::swap(points[idx], points[idx - 1]);
			curAnchor = idx - 1;
		}

		if (idx < points.size() - 1 && points[idx].x > points[idx + 1].x) {
			std::swap(points[idx], points[idx + 1]);
			curAnchor = idx + 1;
		}

		notifyChange();
	}
}
