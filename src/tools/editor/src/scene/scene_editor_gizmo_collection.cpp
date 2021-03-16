#include "scene_editor_gizmo_collection.h"
#include "gizmos/translate_gizmo.h"
#include "gizmos/selected_bounds_gizmo.h"
#include "gizmos/polygon_gizmo.h"
#include "gizmos/selection_box_gizmo.h"
#include "gizmos/vertex_gizmo.h"
#include "halley/core/graphics/camera.h"
using namespace Halley;

SceneEditorGizmoCollection::SceneEditorGizmoCollection(UIFactory& factory, Resources& resources)
	: factory(factory)
	, resources(resources)
{
	// TODO: read this elsewhere
	snapRules.grid = GridSnapMode::Pixel;
	snapRules.line = LineSnapMode::IsometricAxisAligned;
	
	selectedBoundsGizmo = std::make_unique<SelectedBoundsGizmo>(snapRules, resources);
	selectionBoxGizmo = std::make_unique<SelectionBoxGizmo>(snapRules, resources);

	addTool(Tool("drag", LocalisedString::fromHardcodedString("Pan Scene"), Sprite().setImage(resources, "ui/scene_editor_drag.png")), [this] (const String& componentName, const String& fieldName, const ConfigNode& options)
	{
		return std::unique_ptr<SceneEditorGizmo>{};
	});
	addTool(Tool("translate", LocalisedString::fromHardcodedString("Move Entities"), Sprite().setImage(resources, "ui/scene_editor_move.png")), [this] (const String& componentName, const String& fieldName, const ConfigNode& options)
	{
		return std::make_unique<TranslateGizmo>(snapRules);
	});
	addTool(Tool("polygon", LocalisedString::fromHardcodedString("Edit Polygon"), Sprite().setImage(resources, "ui/scene_editor_polygon.png")), [this] (const String& componentName, const String& fieldName, const ConfigNode& options)
	{
		return std::make_unique<PolygonGizmo>(snapRules, componentName, fieldName, options, this->factory);
	});
	addTool(Tool("vertex", LocalisedString::fromHardcodedString("Edit Vertex"), Sprite().setImage(resources, "ui/scene_editor_polygon.png")), [this] (const String& componentName, const String& fieldName, const ConfigNode& options)
	{
		return std::make_unique<VertexGizmo>(snapRules, componentName, fieldName);
	});
}

bool SceneEditorGizmoCollection::update(Time time, const Camera& camera, const SceneEditorInputState& inputState, SceneEditorOutputState& outputState)
{
	selectedBoundsGizmo->setCamera(camera);
	selectedBoundsGizmo->update(time, inputState);
	selectionBoxGizmo->setCamera(camera);
	selectionBoxGizmo->update(time, inputState);
	
	if (activeGizmo) {
		activeGizmo->setCamera(camera);
		activeGizmo->setOutputState(outputState);
		activeGizmo->update(time, inputState);

		return activeGizmo->isHighlighted();
	}
	return false;
}

void SceneEditorGizmoCollection::draw(Painter& painter)
{
	selectedBoundsGizmo->draw(painter);
	selectionBoxGizmo->draw(painter);
	
	if (activeGizmo) {
		activeGizmo->draw(painter);
	}
}

void SceneEditorGizmoCollection::setSelectedEntity(const std::optional<EntityRef>& entity, EntityData& data)
{
	selectedEntity = entity;
	entityData = &data;
	
	selectedBoundsGizmo->setSelectedEntity(entity, *entityData);
	
	if (activeGizmo) {
		activeGizmo->setSelectedEntity(entity, *entityData);
	}
}

void SceneEditorGizmoCollection::refreshEntity()
{
	selectedBoundsGizmo->refreshEntity();
	if (activeGizmo) {
		activeGizmo->refreshEntity();
	}
}

std::shared_ptr<UIWidget> SceneEditorGizmoCollection::setTool(const String& tool, const String& componentName, const String& fieldName, const ConfigNode& options)
{
	currentTool = tool;
	activeGizmo.reset();
	
	const auto iter = gizmoFactories.find(tool);
	if (iter != gizmoFactories.end()) {
		activeGizmo = iter->second(componentName, fieldName, options);
	} else {
		activeGizmo.reset();
	}

	if (activeGizmo) {
		activeGizmo->setSelectedEntity(selectedEntity, *entityData);
		return activeGizmo->makeUI();
	}

	return {};
}

void SceneEditorGizmoCollection::deselect()
{
	if (activeGizmo) {
		activeGizmo->deselect();
	}
}

void SceneEditorGizmoCollection::generateList(UIList& list)
{
	list.clear();
	for (const auto& tool: tools) {
		auto image = std::make_shared<UIImage>(tool.icon);
		image->setToolTip(tool.toolTip);
		list.addImage(tool.id, std::move(image), 1, {}, UISizerAlignFlags::Centre);
	}
}

void SceneEditorGizmoCollection::addTool(const Tool& tool, GizmoFactory gizmoFactory)
{
	tools.push_back(tool);
	gizmoFactories[tool.id] = std::move(gizmoFactory);
}
