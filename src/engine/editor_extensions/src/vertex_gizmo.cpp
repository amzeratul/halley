#include "vertex_gizmo.h"
#include "halley/core/game/scene_editor_interface.h"
#include "halley/core/graphics/painter.h"

#define DONT_INCLUDE_HALLEY_HPP
#include "halley/entity/components/transform_2d_component.h"

using namespace Halley;

VertexGizmo::VertexGizmo(SnapRules snapRules, String componentName, String fieldName)
	: SceneEditorGizmo(snapRules)
	, componentName(std::move(componentName))
	, fieldName(std::move(fieldName))
{
	handle.setBoundsCheck([=] (Vector2f myPos, Vector2f mousePos) -> bool
	{
		return getMainHandle().contains(mousePos);
	});
	handle.setGridSnap(snapRules.grid);
}

void VertexGizmo::update(Time time, const ISceneEditor& sceneEditor, const SceneEditorInputState& inputState)
{
	handle.update(inputState);

	const auto transform = getComponent<Transform2DComponent>();
	if (transform) {
		if (handle.isHeld()) {
			// Write to object
			updateEntityData(transform->inverseTransformPoint(handle.getPosition()));
		} else {
			// Read from object
			handle.setPosition(transform->transformPoint(readEntityData()), false);
		}
		visible = true;
	} else {
		visible = false;
	}
}

void VertexGizmo::draw(Painter& painter, const ISceneEditor& sceneEditor) const
{
	if (visible) {
		const float zoom = getZoom();
		const auto overCol = Colour4f(0.5f, 0.5f, 1);
		const auto outCol = Colour4f(0.2f, 0.2f, 1.0f);
		const auto col = handle.isOver() ? overCol : outCol;
		const auto circle = getMainHandle();

		const auto centre = circle.getCentre();
		const auto radius = circle.getRadius();
		const float lineWidth = 2.0f / zoom;
		
		painter.drawCircle(centre, radius, lineWidth, col);
	}
}

Circle VertexGizmo::getMainHandle() const
{
	const auto pos = handle.getPosition();
	return Circle(pos, 10.0f / getZoom());
}

void VertexGizmo::updateEntityData(Vector2f pos)
{
	auto* data = getComponentData(componentName);
	if (data) {
		(*data)[fieldName] = pos;
	}
	markModified(componentName, fieldName);
}

Vector2f VertexGizmo::readEntityData() const
{
	auto* data = getComponentData(componentName);
	if (data) {
		return (*data)[fieldName].asVector2f(Vector2f());
	}
	return Vector2f();
}

