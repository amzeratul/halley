#include "curve_editor.h"

using namespace Halley;

CurveEditor::CurveEditor(String id, UIStyle style)
	: UIWidget(std::move(id))
{
	styles.push_back(std::move(style));
	setInteractWithMouse(true);

	background = styles.back().getSprite("background");
}

void CurveEditor::update(Time time, bool moved)
{
	if (moved) {
		background.setPosition(getPosition()).scaleTo(getSize());
	}
}

void CurveEditor::draw(UIPainter& painter) const
{
	painter.draw(background);
	painter.draw([this] (Painter& painter)
	{
		auto basePos = getPosition();
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

void CurveEditor::setPoints(Vector<Vector2f> points)
{
	this->points = std::move(points);
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
	
}

void CurveEditor::pressMouse(Vector2f mousePos, int button, KeyMods keyMods)
{
	
}

void CurveEditor::releaseMouse(Vector2f mousePos, int button)
{
	
}
