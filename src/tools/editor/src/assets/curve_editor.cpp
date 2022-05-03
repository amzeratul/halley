#include "curve_editor.h"

using namespace Halley;

CurveEditor::CurveEditor(const String& id, UIStyle style)
	: UIWidget(id)
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
}

void CurveEditor::setRange(Range<float> range)
{
	this->range = range;
}

Range<float> CurveEditor::getRange() const
{
	return range;
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

void CurveEditor::onMouseOver(Vector2f mousePos)
{
	
}

void CurveEditor::pressMouse(Vector2f mousePos, int button, KeyMods keyMods)
{
	
}

void CurveEditor::releaseMouse(Vector2f mousePos, int button)
{
	
}
