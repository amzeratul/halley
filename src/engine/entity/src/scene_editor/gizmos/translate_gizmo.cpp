#include "translate_gizmo.h"
#include "components/transform_2d_component.h"
#include "halley/core/game/scene_editor_interface.h"
#include "halley/core/graphics/painter.h"
using namespace Halley;

TranslateGizmo::TranslateGizmo()
{
	handle.setBoundsCheck([=] (Vector2f myPos, Vector2f mousePos) -> bool
	{
		return (myPos - mousePos).length() < 10.0f / getZoom();
	});
}

void TranslateGizmo::update(Time time, const SceneEditorInputState& inputState)
{
	handle.update(inputState);
}

void TranslateGizmo::draw(Painter& painter) const
{
	if (visible) {
		const float zoom = getZoom();
		auto overCol = Colour4f(0.5f, 0.5f, 1);
		auto outCol = Colour4f(0.2f, 0.2f, 1.0f);
		painter.drawCircle(handle.getPosition(), 10.0f / zoom, 1.0f / zoom, handle.isOver() ? overCol : outCol);
	}
}

void TranslateGizmo::onEntityChanged()
{
	auto transform = getTransform();
	if (transform) {
		handle.setPosition(transform->getGlobalPosition());
		visible = true;
	} else {
		visible = false;
	}
}

