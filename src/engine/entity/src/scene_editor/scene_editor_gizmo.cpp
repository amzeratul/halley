#include "scene_editor/scene_editor_gizmo.h"
#include "components/transform_2d_component.h"
#include "halley/core/graphics/camera.h"
using namespace Halley;

void SceneEditorGizmo::update(Time time)
{}

void SceneEditorGizmo::draw(Painter& painter) const
{}

void SceneEditorGizmo::setSelectedEntity(const std::optional<EntityRef>& entity)
{
	if (curEntity != entity) {
		curEntity = entity;
		onEntityChanged();
	}
}

void SceneEditorGizmo::setCamera(const Camera& camera)
{
	zoom = camera.getZoom();
}

void SceneEditorGizmo::onEntityChanged()
{}

const Transform2DComponent* SceneEditorGizmo::getTransform() const
{
	if (curEntity) {
		return curEntity->tryGetComponent<Transform2DComponent>();
	} else {
		return nullptr;
	}
}

Transform2DComponent* SceneEditorGizmo::getTransform()
{
	if (curEntity) {
		return curEntity->tryGetComponent<Transform2DComponent>();
	} else {
		return nullptr;
	}
}

const std::optional<EntityRef>& SceneEditorGizmo::getEntity() const
{
	return curEntity;
}

std::optional<EntityRef>& SceneEditorGizmo::getEntity()
{
	return curEntity;
}

float SceneEditorGizmo::getZoom() const
{
	return zoom;
}
