#include "scene_editor/translate_gizmo.h"
#include "components/transform_2d_component.h"
#include "halley/core/graphics/painter.h"
using namespace Halley;

void TranslateGizmo::update(Time time)
{
	// TODO
}

void TranslateGizmo::draw(Painter& painter) const
{
	if (visible) {
		painter.drawCircle(pos, 5.0f, 1.0f, Colour4f(1, 1, 1));
	}
}

void TranslateGizmo::onEntityChanged()
{
	auto transform = getTransform();
	if (transform) {
		pos = transform->getGlobalPosition();
		visible = true;
	} else {
		visible = false;
	}
}

