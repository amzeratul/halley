#include "curve_editor.h"

#include "halley/core/graphics/painter.h"

using namespace Halley;

CurveEditor::CurveEditor(String id, UIStyle style)
	: UIWidget(std::move(id))
{
	styles.push_back(std::move(style));
	setInteractWithMouse(true);

	background = styles.back().getSprite("background");
	display = styles.back().getSprite("display");
	lineColour = styles.back().getColour("lineColour");
}

void CurveEditor::update(Time time, bool moved)
{
	if (!dragging && !isMouseOver()) {
		curAnchor = {};
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
	});
}

void CurveEditor::setHorizontalRange(Range<float> range)
{
	horizontalRange = range;
}

Range<float> CurveEditor::getHorizontalRange() const
{
	return horizontalRange;
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
}

void CurveEditor::pressMouse(Vector2f mousePos, int button, KeyMods keyMods)
{
	if (button == 0) {
		const auto anchor = getAnchorAt(mousePos);
		curAnchor = anchor;
		if (anchor) {
			dragging = true;
			updateDragging(mousePos);
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
		p.x = clamp(p.x, horizontalRange.start, horizontalRange.end);
		p.y = clamp(p.y, 0.0f, 1.0f);
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

void CurveEditor::updateDragging(Vector2f mousePos)
{
	if (dragging && curAnchor) {
		points[*curAnchor] = mouseToCurveSpace(mousePos);
		normalizePoints();
	}
}
