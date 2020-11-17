#include "scene_editor_gizmo.h"
#include "halley/entity/components/transform_2d_component.h"
#include "halley/core/editor_extensions/scene_editor_interface.h"
#include "halley/core/graphics/camera.h"
using namespace Halley;

SceneEditorGizmoHandle::SceneEditorGizmoHandle()
{
}

void SceneEditorGizmoHandle::update(const SceneEditorInputState& inputState, gsl::span<SceneEditorGizmoHandle> handles)
{
	if (!holding) {
		over = boundsCheck ? boundsCheck(pos, inputState.mousePos) : false;
		if (canDrag && over && inputState.leftClickPressed) {
			holding = true;
			startOffset = pos - inputState.mousePos;
		}
	}

	if (holding) {
		const auto oldPos = pos;
		setPosition(inputState.mousePos + startOffset, true);
		const Vector2f delta = pos - oldPos;

		// Drag all other selected handles too
		for (auto& handle: handles) {
			if (&handle != this && handle.isSelected()) {
				handle.setPosition(handle.getPosition() + delta, false);
			}
		}
		
		if (!inputState.leftClickHeld) {
			holding = false;
		}
	}

	if (inputState.selectionBox) {
		selected = (selected && inputState.shiftHeld) || inputState.selectionBox.has_value() && inputState.selectionBox.value().contains(pos);
	}
}

void SceneEditorGizmoHandle::setId(int id)
{
	this->id = id;
}

int SceneEditorGizmoHandle::getId() const
{
	return id;
}

void SceneEditorGizmoHandle::setBoundsCheck(BoundsCheckFunction bc)
{
	boundsCheck = std::move(bc);
}

void SceneEditorGizmoHandle::setSnap(SnapFunction sf)
{
	snapFunc = std::move(sf);
}

void SceneEditorGizmoHandle::setGridSnap(GridSnapMode gridSnap)
{
	switch (gridSnap) {
	case GridSnapMode::Disabled:
		snapFunc = {};
		break;
	case GridSnapMode::Pixel:
		snapFunc = [] (int, Vector2f p)
		{
			return p.round();
		};
		break;
	}
}

void SceneEditorGizmoHandle::setPosition(Vector2f p, bool snap)
{
	if (!std::isnan(p.x) && !std::isnan(p.y)) {
		pos = snapFunc && snap ? snapFunc(id, p) : p;
	}
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

bool SceneEditorGizmoHandle::isSelected() const
{
	return selected;
}

void SceneEditorGizmoHandle::setCanDrag(bool enabled)
{
	canDrag = enabled;
	if (!canDrag) {
		holding = false;
	}
}

void SceneEditorGizmoHandle::setNotOver()
{
	over = false;
	holding = false;
}

void SceneEditorGizmoHandle::setSelected(bool sel)
{
	selected = sel;
}

SceneEditorGizmo::SceneEditorGizmo(SnapRules rules)
	: snapRules(rules)
{
}

void SceneEditorGizmo::update(Time time, const SceneEditorInputState& inputState)
{}

void SceneEditorGizmo::draw(Painter& painter) const
{}

std::shared_ptr<UIWidget> SceneEditorGizmo::makeUI()
{
	return {};
}

void SceneEditorGizmo::setSelectedEntity(const std::optional<EntityRef>& entity, EntityData& data)
{
	entityData = &data;
	if (curEntity != entity) {
		curEntity = entity;
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

bool SceneEditorGizmo::isHighlighted() const
{
	return false;
}

void SceneEditorGizmo::deselect()
{
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

EntityData& SceneEditorGizmo::getEntityData()
{
	return *entityData;
}

ConfigNode* SceneEditorGizmo::getComponentData(const String& name)
{
	auto& components = (*entityData).getComponents();
	for (auto& [curName, value]: components) {
		if (curName == name) {
			return &value;
		}
	}
	return nullptr;
}

const ConfigNode* SceneEditorGizmo::getComponentData(const String& name) const
{
	auto& components = (*entityData).getComponents();
	for (auto& [curName, value]: components) {
		if (curName == name) {
			return &value;
		}
	}
	return nullptr;
}

void SceneEditorGizmo::markModified(const String& component, const String& field)
{
	if (outputState) {
		outputState->fieldsChanged.emplace_back(component, field);
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

SceneEditorGizmo::SnapRules SceneEditorGizmo::getSnapRules() const
{
	return snapRules;
}

constexpr static float snapThreshold = 5.0f;

Vector2f SceneEditorGizmo::solveLineSnap(Vector2f cur, std::optional<Vector2f> prev, std::optional<Vector2f> next) const
{
	if (snapRules.line == LineSnapMode::Disabled) {
		return cur;
	}

	std::optional<Line> line0;
	std::optional<Line> line1;
	if (prev) {
		line0 = findSnapLine(cur, prev.value());
	}
	if (next) {
		line1 = findSnapLine(cur, next.value());
	}

	if (line0 && line1) {
		// Can snap to both. The only way to do this is to snap to their intersection
		const auto intersection = line0->intersection(line1.value());
		if (intersection) {
			// We got an intersection, but only actually snap to it if it's close enough
			auto snapPoint = intersection.value();
			if ((snapPoint - cur).length() < snapThreshold) {
				return snapPoint;
			} else {
				// Too far away for an intersection snap. Clear whichever line is farthest from point so the rest of algorithm can take over
				const auto dist0 = line0->getDistance(cur);
				const auto dist1 = line1->getDistance(cur);
				if (dist0 > dist1) {
					line0.reset();
				} else {
					line1.reset();
				}
			}
		} else {
			// If they're parallel, we'll clear line1 so the rest of the algorithm can snap to line0
			line1.reset();
		}
	}

	if (line0 || line1) {
		// Snap to one of them. Pick whichever it is and snap to that
		const auto& line = line0 ? line0.value() : line1.value();
		return line.getClosestPoint(cur);
	}

	// No snapping today
	return cur;
}

std::optional<Line> SceneEditorGizmo::findSnapLine(Vector2f cur, Vector2f ref) const
{	
	Vector2f dirs[] = { Vector2f(1, 0), Vector2f(0, 1), Vector2f(2, 1), Vector2f(2, -1) };
	const int nDirs = snapRules.line == LineSnapMode::IsometricAxisAligned ? 4 : 2;

	int bestDir = -1;
	float bestDistance = snapThreshold;
	
	for (int i = 0; i < nDirs; ++i) {
		auto line = Line(ref, dirs[i]);
		const float distance = line.getDistance(cur);
		if (distance < bestDistance) {
			bestDir = i;
			bestDistance = distance;
		}
	}

	if (bestDir == -1) {
		// No snaps here
		return {};
	} else {
		return Line(ref, dirs[bestDir]);
	}
}

