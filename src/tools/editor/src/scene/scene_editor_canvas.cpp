#include "scene_editor_canvas.h"
using namespace Halley;

SceneEditorCanvas::SceneEditorCanvas(String id, Resources& resources)
	: UIWidget(std::move(id))
	, resources(resources)
{
	border.setImage(resources, "whitebox.png").setColour(Colour4f());
	canvas.setImage(resources, "whitebox.png").setColour(Colour4f(0.2f, 0.2f, 0.2f));
}

void SceneEditorCanvas::update(Time t, bool moved)
{
	canvas.setPos(getPosition() + Vector2f(1, 1)).setSize(getSize() - Vector2f(2, 2));
}

void SceneEditorCanvas::draw(UIPainter& painter) const
{
	const auto pos = getPosition();
	const auto size = getSize();
	painter.draw(border.clone().setPos(pos).setSize(Vector2f(size.x, 1)), true);
	painter.draw(border.clone().setPos(pos + Vector2f(0, size.y - 1)).setSize(Vector2f(size.x, 1)), true);
	painter.draw(border.clone().setPos(pos).setSize(Vector2f(1, size.y)), true);
	painter.draw(border.clone().setPos(pos + Vector2f(size.x - 1, 0)).setSize(Vector2f(1, size.y)), true);
	painter.draw(canvas);
}
