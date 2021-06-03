#define DONT_INCLUDE_HALLEY_HPP

#include "scene_editor.h"
#include "halley/entity/world.h"
#include "halley/core/api/halley_api.h"
#include "halley/core/game/halley_statics.h"
#include "halley/core/graphics/sprite/sprite.h"
#include "halley/core/graphics/render_context.h"
#include "halley/entity/system.h"
#include "halley/editor_extensions/scene_editor_input_state.h"
#include "halley/core/graphics/sprite/sprite_sheet.h"
#include "halley/core/graphics/text/font.h"
#include "halley/entity/components/transform_2d_component.h"
#include "components/sprite_component.h"
#include "components/camera_component.h"
#include "halley/entity/scripting/script_node_type.h"
#include "halley/utils/algorithm.h"

using namespace Halley;

SceneEditor::SceneEditor() = default;

SceneEditor::~SceneEditor()
{
	if (resources) {
		for (const auto& ss: resources->enumerate<SpriteSheet>()) {
			resources->get<SpriteSheet>(ss)->clearMaterialCache();
		}
	}
}

void SceneEditor::init(SceneEditorContext& context)
{
	api = context.api;
	resources = context.resources;
	editorResources = context.editorResources;
	gizmoCollection = context.gizmos;
	editorInterface = context.editorInterface;

	api->core->getStatics().setupGlobals();

	coordinateInfo
		.setFont(editorResources->get<Font>("Ubuntu Bold"))
		.setSize(14)
		.setOutline(1)
		.setColour(Colour(1, 1, 1))
		.setOutlineColour(Colour())
		.setOffset(Vector2f(0, 1));
}

void SceneEditor::update(Time t, SceneEditorInputState inputState, SceneEditorOutputState& outputState)
{
	// Update world
	world->step(TimeLine::FixedUpdate, t);
	world->step(TimeLine::VariableUpdate, t);

	// Update camera
	auto cameraEntity = world->getEntity(cameraEntityIds.at(0));
	auto& cameraComponent = cameraEntity.getComponent<CameraComponent>();
	auto& transformComponent = cameraEntity.getComponent<Transform2DComponent>();
	camera.setPosition(transformComponent.getGlobalPosition()).setZoom(cameraComponent.zoom);

	// Update input state
	inputState.mousePos = inputState.rawMousePos ? camera.screenToWorld(inputState.rawMousePos.value(), inputState.viewRect) : std::optional<Vector2f>();
	mousePos = inputState.mousePos;
	if (!inputState.leftClickHeld) {
		holdMouseStart.reset();
	}
	if (mousePos) {
		if (holdMouseStart && (holdMouseStart.value() - mousePos.value()).length() > 3) {
			selBox = Rect4f(holdMouseStart.value(), mousePos.value());
		}
	}
	inputState.selectionBox = selBox;
	if (!inputState.leftClickHeld) {
		// Make sure this happens after inputState.selectionBox is set
		// This is so you still get a final selection box on release
		selBox.reset();
	}

	// Update gizmos
	const bool hasHighlightedGizmo = gizmoCollection->update(t, camera, *this, inputState, outputState);
	if (!hasHighlightedGizmo && inputState.leftClickPressed) {
		gizmoCollection->deselect();
		onClick(inputState, outputState);
	}

	// Start dragging
	if (!hasHighlightedGizmo && inputState.leftClickPressed) {
		holdMouseStart = mousePos;
	}

	focusEntityEnabled = inputState.altHeld;
	if (!focusEntityEnabled) {
		highlightDelta = 0;
	}
	updateEntityFocused();
}

void SceneEditor::render(RenderContext& rc)
{
	world->render(rc);

	rc.with(camera).bind([&] (Painter& painter)
	{
		gizmoCollection->draw(painter);
	});

	Camera uiCamera;
	rc.with(uiCamera).bind([&] (Painter& painter)
	{
		drawOverlay(painter, painter.getWorldViewAABB());
	});
}

bool SceneEditor::isReadyToCreateWorld() const
{
	return getGameResources().exists<ConfigFile>(getSceneEditorStageName());
}

void SceneEditor::createWorld(std::shared_ptr<const UIColourScheme> colourScheme)
{
	world = doCreateWorld(getSceneEditorStageName());
	createServices(*world);
	createEntities(*world);
	cameraEntityIds = createCamera();
	world->setEditor(true);

	onInit(colourScheme);
}

World& SceneEditor::getWorld() const
{
	Expects(world);
	return *world;
}

Resources& SceneEditor::getResources() const
{
	Expects(resources);
	return *resources;
}

void SceneEditor::spawnPending()
{
	Expects(world);
	world->spawnPending();
}

std::vector<std::unique_ptr<IComponentEditorFieldFactory>> SceneEditor::getComponentEditorFieldFactories()
{
	return {};
}

std::shared_ptr<UIWidget> SceneEditor::makeCustomUI()
{
	return {};
}

void SceneEditor::onSceneLoaded(Prefab& scene)
{
}

void SceneEditor::onSceneSaved()
{
}

std::unique_ptr<World> SceneEditor::doCreateWorld(const String& stageName) const
{
	return World::make(getAPI(), getGameResources(), stageName, true);
}

void SceneEditor::createServices(World& world)
{
}

void SceneEditor::createEntities(World& world)
{
}

String SceneEditor::getSceneEditorStageName() const
{
	return "stages/scene_editor";
}

const HalleyAPI& SceneEditor::getAPI() const
{
	return *api;
}

Resources& SceneEditor::getGameResources() const
{
	return *resources;
}

Resources& SceneEditor::getEditorResources() const
{
	return *editorResources;
}

void SceneEditor::drawOverlay(Painter& painter, Rect4f view)
{
	const Vector2f drawPos = view.getBottomLeft() + Vector2f(10, -10);
	String drawStr = "Zoom: " + toString(camera.getZoom()) + "x";
	std::vector<ColourOverride> colours;

	if (mousePos) {
		const auto worldOffset = getWorldOffset();
		const Vector2i scenePos = Vector2i(mousePos.value().round());
		const Vector2i worldPos = scenePos + Vector2i(worldOffset.value_or(Vector2f()));

		colours.emplace_back(drawStr.size(), Colour4f(1.0f, 1.0f, 0.8f));
		drawStr += "\nScene: " + scenePos;

		if (worldOffset) {
			colours.emplace_back(drawStr.size(), Colour4f(0.8f, 1.0f, 0.8f));
			drawStr += "\nWorld: " + Vector2i(worldPos);
		}

		if (selectedEntity) {
			const auto* t2d = selectedEntity.value().tryGetComponent<Transform2DComponent>();
			colours.emplace_back(drawStr.size(), Colour4f(0.8f, 0.8f, 1.0f));
			if (t2d) {
				const auto objectPos = Vector2i(t2d->inverseTransformPoint(Vector2f(scenePos)));
				drawStr += "\nObject: " + objectPos;
			} else {
				drawStr += "\nObject: --";
			}
		}
	}
	
	coordinateInfo
		.setPosition(drawPos)
		.setText(drawStr)
		.setColourOverride(colours)
		.setOffset(Vector2f(0, 1))
		.draw(painter);
}

std::vector<EntityId> SceneEditor::createCamera()
{
	return std::vector<EntityId>({
		getWorld().createEntity("editorCamera")
			.addComponent(Transform2DComponent(Vector2f(0, 0)))
			.addComponent(CameraComponent(1.0f, "main"))
			.getEntityId()
	});
}

void SceneEditor::onEntitySelected(std::optional<EntityRef> entity)
{
}

void SceneEditor::setEntityFocus(std::vector<EntityId> entityIds)
{
}

void SceneEditor::cycleHighlight(int delta)
{
	highlightDelta += delta;
}

void SceneEditor::setEntityHighlightedOnList(const UUID& id)
{
	entityHighlightedOnList = world->findEntity(id).value_or(EntityRef());
}

EntityRef SceneEditor::getEntityToFocus()
{
	if (!focusEntityEnabled) {
		return EntityRef();
	}

	EntityRef underMouse = mousePos ? getRootEntityAt(mousePos.value()) : EntityRef();
	if (!underMouse.isValid()) {
		return entityHighlightedOnList;
	}
	return underMouse;
}

void SceneEditor::updateEntityFocused()
{
	const EntityRef targetFocusedEntity = getEntityToFocus();
	if (targetFocusedEntity != focusedEntity) {
		focusedEntity = targetFocusedEntity;
		
		std::vector<EntityId> selectedIds;
		if (focusedEntity.isValid()) {
			addEntityIdToList(selectedIds, focusedEntity);
		}
		setEntityFocus(std::move(selectedIds));
	}
}

void SceneEditor::addEntityIdToList(std::vector<EntityId>& dst, EntityRef entity)
{
	dst.push_back(entity.getEntityId());
	if (entity.getPrefab()) {
		for (auto e: entity.getChildren()) {
			addEntityIdToList(dst, e);
		}
	}
}

std::optional<Vector2f> SceneEditor::getMousePos() const
{
	return mousePos;
}

Vector2f SceneEditor::getCameraPos() const
{
	return camera.getPosition().xy();
}

std::optional<Vector2f> SceneEditor::getWorldOffset() const
{
	return {};
}

const std::vector<EntityId>& SceneEditor::getCameraIds() const
{
	return cameraEntityIds;
}

void SceneEditor::dragCamera(Vector2f amount)
{
	auto camera = getWorld().getEntity(cameraEntityIds.at(0));
	const float zoom = camera.getComponent<CameraComponent>().zoom;
	auto& transform = camera.getComponent<Transform2DComponent>();
	transform.setGlobalPosition(roundPosition(transform.getGlobalPosition() + amount / zoom));
	saveCameraPos();
}

void SceneEditor::moveCamera(Vector2f pos)
{
	moveCameraTo2D(pos);
}

void SceneEditor::moveCameraTo2D(Vector2f pos)
{
	auto camera = getWorld().getEntity(cameraEntityIds.at(0));
	auto& transform = camera.getComponent<Transform2DComponent>();
	transform.setGlobalPosition(roundPosition(pos));
	saveCameraPos();
}

void SceneEditor::changeZoom(int amount, Vector2f cursorPosRelToCamera)
{
	if (amount == 0) {
		return;
	}

	auto cameraEntity = getWorld().getEntity(cameraEntityIds.at(0));
	auto& camera = cameraEntity.getComponent<CameraComponent>();
	auto& transform = cameraEntity.getComponent<Transform2DComponent>();
	const float prevZoom = camera.zoom;

	// Zoom
	const int curLevel = lroundf(std::log2f(clamp(camera.zoom, 1.0f / 32.0f, 32.0f)));
	camera.zoom = std::pow(2.0f, float(clamp(curLevel + amount, -5, 5)));

	// Translate to keep fixed point
	const Vector2f translate = cursorPosRelToCamera * (1.0f / prevZoom - 1.0f / camera.zoom);
	transform.setGlobalPosition(roundPosition(transform.getGlobalPosition() + translate, camera.zoom));
	saveCameraPos();
}

void SceneEditor::saveCameraPos()
{
	auto cameraEntity = getWorld().getEntity(cameraEntityIds.at(0));
	auto& camera = cameraEntity.getComponent<CameraComponent>();
	auto& transform = cameraEntity.getComponent<Transform2DComponent>();
	editorInterface->setAssetSetting("cameraPos", ConfigNode(transform.getGlobalPosition()));
	editorInterface->setAssetSetting("cameraZoom", ConfigNode(camera.zoom));
}

bool SceneEditor::loadCameraPos()
{
	auto cameraEntity = getWorld().getEntity(cameraEntityIds.at(0));

	const auto& zoom = editorInterface->getAssetSetting("cameraZoom");
	if (zoom.getType() == ConfigNodeType::Float) {
		auto& camera = cameraEntity.getComponent<CameraComponent>();
		camera.zoom = zoom.asFloat();
	}

	const auto& pos = editorInterface->getAssetSetting("cameraPos");
	if (pos.getType() == ConfigNodeType::Float2) {
		auto& transform = cameraEntity.getComponent<Transform2DComponent>();
		transform.setGlobalPosition(pos.asVector2f());
		return true;
	}
	return false;
}

void SceneEditor::setupTools(UIList& toolList, ISceneEditorGizmoCollection& gizmoCollection)
{
	gizmoCollection.generateList(toolList);
}

void SceneEditor::setSelectedEntity(const UUID& id, EntityData& entityData)
{
	const auto curId = selectedEntity ? selectedEntity.value().getInstanceUUID() : UUID();
	if (id != curId) {
		selectedEntity.reset();
		if (id.isValid()) {
			selectedEntity = getWorld().findEntity(id);
		}
	}	

	gizmoCollection->setSelectedEntity(selectedEntity, entityData);

	onEntitySelected(selectedEntity);
	updateEntityFocused();
}

void SceneEditor::onEntityAdded(const UUID& id, const EntityData& entityData)
{
	if (id.isValid()) {
		onEntityAdded(getEntity(id), entityData);
	}
}

void SceneEditor::onEntityRemoved(const UUID& id)
{
	if (id.isValid()) {
		onEntityRemoved(getEntity(id));
	}
}

void SceneEditor::onEntityMoved(const UUID& id, const EntityData& entityData)
{
	if (id.isValid()) {
		onEntityMoved(getEntity(id), entityData);
	}
}

void SceneEditor::onEntityModified(const UUID& id, const EntityData& entityData)
{
	if (id.isValid()) {
		onEntityModified(getEntity(id), entityData);
	}
}

void SceneEditor::onEntityModified(EntityRef entity, const EntityData& entityData)
{}

void SceneEditor::onEntityAdded(EntityRef entity, const EntityData& entityData)
{}

void SceneEditor::onEntityRemoved(EntityRef entity)
{}

void SceneEditor::onEntityMoved(EntityRef entity, const EntityData& entityData)
{}

void SceneEditor::showEntity(const UUID& id)
{
	auto e = getWorld().findEntity(id);
	
	if (e) {
		const auto aabb = getSpriteTreeBounds(e.value());
		moveCameraTo2D(aabb.getCenter());
	}
}

void SceneEditor::onToolSet(String& tool, String& componentName, String& fieldName)
{
}

Rect4f SceneEditor::getSpriteTreeBounds(const EntityRef& e) const
{
	std::optional<Rect4f> rect;
	doGetSpriteTreeBounds(e, rect);
	return rect.value_or(Rect4f());
}

void SceneEditor::doGetSpriteTreeBounds(const EntityRef& e, std::optional<Rect4f>& rect) const
{
	auto cur = getSpriteBounds(e);
	if (!rect) {
		rect = cur;
	}
	if (cur) {
		rect = rect->merge(*cur);
	}

	for (auto& c: e.getRawChildren()) {
		auto child = EntityRef(*c, e.getWorld());
		doGetSpriteTreeBounds(child, rect);
	}
}

std::optional<Rect4f> SceneEditor::getSpriteBounds(const EntityRef& e) const
{
	const auto transform2d = e.tryGetComponent<Transform2DComponent>();

	if (transform2d) {
		const auto sprite = e.tryGetComponent<SpriteComponent>();

		if (sprite) {
			if (isSpriteVisibleOnCamera(sprite->sprite, sprite->mask)) {
				return transform2d->getSpriteAABB(sprite->sprite);
			}
		} else {
			auto pos = transform2d->getGlobalPosition();
			return Rect4f(pos, pos);
		}
	}
	return {};
}

bool SceneEditor::isSpriteVisibleOnCamera(const Sprite& sprite, OptionalLite<int> mask) const
{
	return true;
}

void SceneEditor::setupConsoleCommands(UIDebugConsoleController& controller, ISceneEditorWindow& sceneEditor)
{
}

void SceneEditor::refreshAssets()
{
}

std::shared_ptr<ScriptNodeTypeCollection> SceneEditor::getScriptNodeTypes()
{
	return std::make_shared<ScriptNodeTypeCollection>();
}

std::vector<std::pair<String, String>> SceneEditor::getRightClickMenu(const Vector2f& mousePos) const
{
	return {};
}

Vector2f SceneEditor::roundPosition(Vector2f pos) const
{
	return roundPosition(pos, camera.getZoom());
}

Vector2f SceneEditor::roundPosition(Vector2f pos, float zoom) const
{
	return (pos * zoom).round() / zoom;
}

void SceneEditor::onInit(std::shared_ptr<const UIColourScheme> colourScheme)
{
}

EntityRef SceneEditor::getEntity(const UUID& id) const
{
	const auto curId = selectedEntity ? selectedEntity.value().getInstanceUUID() : UUID();
	if (curId == id) {
		return selectedEntity.value();
	} else {
		return getWorld().findEntity(id).value();
	}
}

bool SceneEditor::isPointInSprite(EntityRef& e, Vector2f point) const
{
	const auto* spriteComponent = e.tryGetComponent<SpriteComponent>();
	if (!spriteComponent) {
		return false;
	}
	
	const auto& sprite = spriteComponent->sprite;
	return sprite.isPointVisible(point);
}

float SceneEditor::getSpriteDepth(EntityRef& e, Vector2f pos) const
{
	const auto* sprite = e.tryGetComponent<SpriteComponent>();
	if (sprite) {
		return static_cast<float>(sprite->layer);
	} else {
		return -std::numeric_limits<float>::infinity();
	}
}

std::vector<EntityRef> SceneEditor::getEntitiesAt(Vector2f point) const
{
	std::vector<std::pair<EntityRef, float>> temp;
	
	for (auto& e: world->getEntities()) {
		if (isPointInSprite(e, point)) {
			const float depth = getSpriteDepth(e, point);
			temp.emplace_back(e, depth);
		}
	}

	std::sort(temp.begin(), temp.end(), [] (const auto& a, const auto& b) { return a.second > b.second; });

	std::vector<EntityRef> result;
	result.reserve(temp.size());
	for (auto& e: temp) {
		if (e.first.isValid()) {
			result.push_back(e.first);
		}
	}
	return result;
}

std::vector<EntityRef> SceneEditor::getRootEntitiesAt(Vector2f point) const
{
	const auto entities = getEntitiesAt(point);

	std::vector<EntityRef> result;
	for (const auto& e: entities) {
		EntityRef curResult = e;
		for (EntityRef parent = curResult.getParent(); parent.isValid(); parent = parent.getParent()) {
			if (parent.getPrefab()) {
				curResult = parent;
			}
		}

		if (!std_ex::contains(result, curResult)) {
			result.push_back(curResult);
		}
	}

	return result;
}

EntityRef SceneEditor::getRootEntityAt(Vector2f point) const
{
	const auto entities = getRootEntitiesAt(point);
	if (entities.empty()) {
		highlightDelta = 0;
		return EntityRef();
	}

	highlightDelta = modulo(highlightDelta, static_cast<int>(entities.size()));
	
	return entities.at(highlightDelta);
}

void SceneEditor::onClick(const SceneEditorInputState& input, SceneEditorOutputState& output)
{
	if (!input.mousePos) {
		return;
	}
	if (input.spaceHeld || input.ctrlHeld || input.shiftHeld) {
		return;
	}
	
	const auto bestEntity = getRootEntityAt(input.mousePos.value());

	if (bestEntity.isValid()) {
		output.newSelection = bestEntity.getInstanceUUID();
	} else {
		output.newSelection.reset();
	}
}
