#include "scene_editor/scene_editor_gizmo.h"
#include "components/transform_2d_component.h"
#include "halley/core/game/scene_editor_interface.h"
#include "halley/core/graphics/camera.h"
using namespace Halley;

void SceneEditorGizmoHandle::update(const SceneEditorInputState& inputState)
{
	if (!holding) {
		over = boundsCheck(pos, inputState.mousePos);
		if (over && inputState.leftClickPressed) {
			holding = true;
			startOffset = pos - inputState.mousePos;
		}
	}

	if (holding) {
		pos = inputState.mousePos + startOffset;
		if (!inputState.leftClickHeld) {
			holding = false;
		}
	}
}

void SceneEditorGizmoHandle::setBoundsCheck(std::function<bool(Vector2f, Vector2f)> bc)
{
	boundsCheck = std::move(bc);
}

void SceneEditorGizmoHandle::setPosition(Vector2f p)
{
	pos = p;
}

Vector2f SceneEditorGizmoHandle::getPosition() const
{
	return pos;
}

bool SceneEditorGizmoHandle::isOver() const
{
	return over;
}

bool SceneEditorGizmoHandle::isHeld() const
{
	return holding;
}

void SceneEditorGizmo::update(Time time, const SceneEditorInputState& inputState)
{}

void SceneEditorGizmo::draw(Painter& painter) const
{}

void SceneEditorGizmo::setSelectedEntity(const std::optional<EntityRef>& entity, ConfigNode& data)
{
	if (curEntity != entity) {
		curEntity = entity;
		entityData = &data;
		onEntityChanged();
	}
}

void SceneEditorGizmo::setCamera(const Camera& camera)
{
	zoom = camera.getZoom();
}

void SceneEditorGizmo::setOutputState(SceneEditorOutputState& state)
{
	outputState = &state;
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

ConfigNode& SceneEditorGizmo::getEntityData()
{
	return *entityData;
}

ConfigNode* SceneEditorGizmo::getComponentData(const String& name)
{
	auto& components = (*entityData)["components"];
	if (components.getType() == ConfigNodeType::Sequence) {
		for (auto& compNode: components.asSequence()) {
			for (auto& [curName, value]: compNode.asMap()) {
				if (curName == name) {
					return &value;
				}
			}
		}
	}
	return nullptr;
}

void SceneEditorGizmo::markModified(const String& component, const String& field)
{
	if (outputState) {
		outputState->fieldsChanged.push_back(std::make_pair(component, field));
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
