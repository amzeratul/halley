#ifndef DONT_INCLUDE_HALLEY_HPP
#define DONT_INCLUDE_HALLEY_HPP
#endif
#include "halley/editor_extensions/scene_editor.h"

#include "halley/editor_extensions/entity_validator.h"
#include "halley/entity/world.h"
#include "halley/api/halley_api.h"
#include "halley/game/halley_statics.h"
#include "halley/graphics/sprite/sprite.h"
#include "halley/graphics/render_context.h"
#include "halley/entity/system.h"
#include "halley/editor_extensions/scene_editor_input_state.h"
#include "halley/graphics/sprite/sprite_sheet.h"
#include "halley/graphics/text/font.h"
#include "halley/entity/components/transform_2d_component.h"
#include "components/sprite_component.h"
#include "components/camera_component.h"
#include "halley/game/frame_data.h"
#include "halley/graphics/material/material.h"
#include "halley/graphics/material/material_definition.h"
#include "halley/graphics/render_target/render_surface.h"
#include "halley/graphics/render_target/render_target_texture.h"
#include "halley/graphics/sprite/animation_player.h"
#include "halley/scripting/script_node_type.h"
#include "halley/utils/algorithm.h"
#include "halley/ui/widgets/ui_popup_menu.h"
#include "halley/utils/scoped_guard.h"

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
	gameEditorData = context.gameEditorData;

	api->core->getStatics().setupGlobals();

	coordinateInfo
		.setFont(editorResources->get<Font>("Ubuntu Bold"))
		.setSize(14)
		.setOutline(1)
		.setColour(Colour(1, 1, 1))
		.setOutlineColour(Colour())
		.setOffset(Vector2f(0, 1));

	curFrameData = makeFrameData();
}

void SceneEditor::update(Time t, SceneEditorInputState inputState, SceneEditorOutputState& outputState)
{
	if (curFrameData) {
		curFrameData->doStartFrame(false, nullptr, t);
	}
	
	pushThreadFrameData();
	auto guard = ScopedGuard([&] ()
	{
		popThreadFrameData();
	});
	
	preUpdate(t);

	// Update camera
	updateCameraPos(t);

	// Update input state
	const float zoom = getZoom();
	inputState.mousePos = inputState.rawMousePos ? roundPosition(camera.screenToWorld(inputState.rawMousePos.value(), inputState.viewRect), zoom) : std::optional<Vector2f>();
	mousePos = inputState.mousePos;
	if (!inputState.leftClickHeld) {
		holdMouseStart.reset();
	}

	// Box selection
	if (inputState.leftClickPressed || inputState.rightClickPressed || inputState.middleClickPressed) {
		cameraPanAnimation.stop();
	}
	if (mousePos) {
		if (holdMouseStart && (holdMouseStart.value() - mousePos.value()).length() > 3 / getZoom()) {
			onStartSelectionBox();
		}
	}
	inputState.selectionBox = selBox;
	const bool canBoxSelect = gizmoCollection->canBoxSelectEntities();
	if (selBox && canBoxSelect) {
		onSelectionBox(inputState, outputState);
	}
	if (!inputState.leftClickHeld) {
		// Make sure this happens after inputState.selectionBox is set
		// This is so you still get a final selection box on release
		selBox.reset();
		selBoxStartSelectedEntities.clear();
	}

	const bool canSelectEntities = gizmoCollection->canSelectEntities();
	if (!canSelectEntities) {
		outputState.newSelection = Vector<UUID>();
		outputState.selectionMode = UIList::SelectionMode::Normal;
	}

	// Update gizmos
	const auto gizmoUpdateResult = gizmoCollection->update(t, camera, *this, inputState, outputState);
	outputState.blockRightClick = gizmoUpdateResult.blockRightClick;
	if (!gizmoUpdateResult.hasHighlight && inputState.leftClickPressed) {
		gizmoCollection->deselect();
		onClick(inputState, outputState, gizmoUpdateResult.allowEntitySpriteSelection, canSelectEntities);
	}

	// Start dragging
	if (!gizmoUpdateResult.hasHighlight && inputState.leftClickPressed) {
		holdMouseStart = mousePos;
	}

	focusEntityEnabled = inputState.altHeld && inputState.ctrlHeld;
	if (!focusEntityEnabled) {
		highlightDelta = 0;
	}
	updateEntityFocused();

	// Update world
	world->step(TimeLine::FixedUpdate, t);
	world->step(TimeLine::VariableUpdate, t);
	world->step(TimeLine::VariableUpdateUI, t);

	lastStepTime = t;

	postUpdate(t);
}

void SceneEditor::render(RenderContext& rc)
{
	auto guard = ScopedGuard([&] ()
	{
		popThreadFrameData();
	});
	pushThreadFrameData();

	preRender(rc);

	world->render(rc);

	rc.with(camera).bind([&] (Painter& painter)
	{
		gizmoCollection->draw(painter, *this);
	});

	Camera uiCamera;
	rc.with(uiCamera).bind([&] (Painter& painter)
	{
		drawOverlay(painter, painter.getWorldViewAABB());
	});

	postRender(rc);

	if (curFrameData) {
		curFrameData->doEndFrame();
	}
}

void SceneEditor::preUpdate(Time t)
{
}

void SceneEditor::postUpdate(Time t)
{
}

void SceneEditor::preRender(RenderContext& rc)
{
	viewPort = rc.getCamera().getViewPort().value_or(rc.getDefaultRenderTarget().getViewPort()).getSize();
}

void SceneEditor::postRender(RenderContext& rc)
{
}

bool SceneEditor::isReadyToCreateWorld() const
{
	return getGameResources().exists<ConfigFile>(getSceneEditorStageName());
}

void SceneEditor::createWorld(const Prefab& prefab, std::shared_ptr<const UIColourScheme> colourScheme)
{
	world = doCreateWorld(getSceneEditorStageName(), getSceneEditorSystemTag());
	createServices(*world, colourScheme, prefab);
	createEntities(*world, prefab);
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

std::unique_ptr<World> SceneEditor::doCreateWorld(const String& stageName, const std::optional<String>& systemTag) const
{
	return World::make(getAPI(), getGameResources(), stageName, systemTag, true);
}

void SceneEditor::createServices(World& world, std::shared_ptr<const UIColourScheme> colourScheme, const Prefab& prefab)
{
}

void SceneEditor::createEntities(World& world, const Prefab& prefab)
{
}

String SceneEditor::getSceneEditorStageName() const
{
	return "stages/scene_editor";
}

std::optional<String> SceneEditor::getSceneEditorSystemTag() const
{
	return std::nullopt;
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
	String drawStr = "Zoom: " + toString(camera.getZoom(), 3, '.', false) + "x";
	Vector<ColourOverride> colours;

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

		if (!selectedEntityIds.empty()) {
			const auto* t2d = world->getEntity(selectedEntityIds.front()).tryGetComponent<Transform2DComponent>();
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

Vector<EntityId> SceneEditor::createCamera()
{
	return Vector<EntityId>({
		getWorld().createEntity("editorCamera")
			.addComponent(Transform2DComponent(Vector2f(0, 0)))
			.addComponent(CameraComponent(1.0f, "main", Vector2f(), false))
			.setSerializable(false)
			.getEntityId()
	});
}

void SceneEditor::onEntitiesSelected(Vector<EntityId> entities)
{
}

void SceneEditor::setEntityFocus(Vector<EntityId> entityIds)
{
}

void SceneEditor::cycleHighlight(int delta)
{
	highlightDelta += delta;
}

void SceneEditor::setEntityHighlightedOnList(const UUID& id, bool forceShow)
{
	auto entity = world->findEntity(id).value_or(EntityRef());
	if (forceShow) {
		forceFocusEntity = entity.isValid() ? entity : std::optional<EntityRef>();
	}
	entityHighlightedOnList = entity;
}

EntityRef SceneEditor::getEntityToFocus()
{
	if (forceFocusEntity) {
		return forceFocusEntity.value();
	}
	
	if (!focusEntityEnabled) {
		return EntityRef();
	}

	EntityRef underMouse = mousePos ? getRootEntityAt(mousePos.value(), false, EntityAtPositionSelectMode::All) : EntityRef();
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
		
		Vector<EntityId> selectedIds;
		if (focusedEntity.isValid()) {
			addEntityIdToList(selectedIds, focusedEntity);
		}
		setEntityFocus(std::move(selectedIds));
	}
}

void SceneEditor::addEntityIdToList(Vector<EntityId>& dst, EntityRef entity)
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

float SceneEditor::getZoom() const
{
	return camera.getZoom();
}

std::optional<Vector2f> SceneEditor::getWorldOffset() const
{
	return {};
}

const Vector<EntityId>& SceneEditor::getCameraIds() const
{
	return cameraEntityIds;
}

void SceneEditor::dragCamera(Vector2f amount)
{
	auto camera = getWorld().getEntity(cameraEntityIds.at(0));
	const float zoom = camera.getComponent<CameraComponent>().zoom;
	auto& transform = camera.getComponent<Transform2DComponent>();
	transform.setGlobalPosition(roundPosition(transform.getGlobalPosition() + amount / zoom));

	cameraPanAnimation.deltas.emplace_back(amount / zoom, lastStepTime);
	if (cameraPanAnimation.deltas.size() > 3) {
		cameraPanAnimation.deltas.erase(cameraPanAnimation.deltas.begin());
	}
	cameraPanAnimation.inertiaVel.reset();
	cameraPanAnimation.updatedLastFrame = true;

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

void SceneEditor::adjustView(int zoomChange, bool zoomToFit, bool centre)
{
	auto cameraEntity = getWorld().getEntity(cameraEntityIds.at(0));
	auto& camera = cameraEntity.getComponent<CameraComponent>();
	auto& transform = cameraEntity.getComponent<Transform2DComponent>();
	const float prevZoom = camera.zoom;

	float targetZoom = prevZoom;
	if (zoomToFit) {
		// TODO
		targetZoom = 1.0f;
	} else if (zoomChange != 0) {
		const int curLevel = lroundf(std::log2f(clamp(camera.zoom, 1.0f / 32.0f, 32.0f)));
		targetZoom = std::pow(2.0f, float(clamp(curLevel + zoomChange, -5, 5)));
	}

	const Vector2f prevPos = transform.getGlobalPosition();
	const Vector2f targetPos = centre ? Vector2f() : prevPos;

	cameraAnimation = CameraAnimation{ prevPos, targetPos, prevZoom, targetZoom, 0 };
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
	const float targetZoom = std::pow(2.0f, float(clamp(curLevel + amount, -5, 5)));

	// Translate to keep fixed point
	const Vector2f prevPos = transform.getGlobalPosition();
	const Vector2f translate = cursorPosRelToCamera * (1.0f / prevZoom - 1.0f / targetZoom);
	const Vector2f targetPos = roundPosition(prevPos + translate, targetZoom);

	cameraAnimation = CameraAnimation{ prevPos, targetPos, prevZoom, targetZoom, 0 };
}

void SceneEditor::updateCameraPos(Time t)
{
	auto cameraEntity = getWorld().getEntity(cameraEntityIds.at(0));
	auto& camera = cameraEntity.getComponent<CameraComponent>();
	auto& transform = cameraEntity.getComponent<Transform2DComponent>();
	bool changed = false;

	if (cameraAnimation) {
		cameraPanAnimation.inertiaVel = Vector2f();
		cameraPanAnimation.deltas.clear();

		auto& ca = *cameraAnimation;
		ca.t = advance(ca.t, 1.0f, static_cast<float>(t) * 4.0f);
		const float t = std::sin(ca.t * pif() * 0.5f);
		camera.zoom = 1.0f / lerp(1.0f / ca.z0, 1.0f / ca.z1, t);
		transform.setGlobalPosition(lerp(ca.p0, ca.p1, t));

		if (ca.t >= 0.999f) {
			camera.zoom = ca.z1;
			transform.setGlobalPosition(ca.p1);
			cameraAnimation.reset();
			changed = true;
		}
	}

	if (!cameraPanAnimation.updatedLastFrame) {
		auto& vel = cameraPanAnimation.inertiaVel;
		if (!vel && cameraPanAnimation.deltas.size() >= 3) {
			Vector2f ds;
			Time dt = 0;
			for (const auto& d: cameraPanAnimation.deltas) {
				ds += d.first;
				dt += d.second;
			}
			vel = ds / static_cast<float>(dt);
			cameraPanAnimation.deltas.clear();
			if (vel->length() < 400.0f / getZoom()) {
				vel.reset();
			}
		}

		if (vel && vel->length() > 60.0f / getZoom()) {
			transform.setGlobalPosition(transform.getGlobalPosition() + *vel * static_cast<float>(t));
			vel = damp(*vel, Vector2f(), 5.0f, static_cast<float>(t));
		}
	}
	cameraPanAnimation.updatedLastFrame = false;

	if (changed) {
		saveCameraPos();
	}

	this->camera.setPosition(roundCameraPosition(transform.getGlobalPosition(), camera.zoom)).setZoom(camera.zoom);
}

void SceneEditor::saveCameraPos()
{
	Expects(editorInterface);

	auto cameraEntity = getWorld().getEntity(cameraEntityIds.at(0));
	auto& camera = cameraEntity.getComponent<CameraComponent>();
	auto& transform = cameraEntity.getComponent<Transform2DComponent>();
	editorInterface->setAssetSetting("cameraPos", ConfigNode(transform.getGlobalPosition()));
	editorInterface->setAssetSetting("cameraZoom", ConfigNode(camera.zoom));
}

void SceneEditor::pushThreadFrameData() const
{
	if (curFrameDataDepth == 0) {
		BaseFrameData::setThreadFrameData(curFrameData.get());
	}
	++curFrameDataDepth;
}

void SceneEditor::popThreadFrameData() const
{
	assert(curFrameDataDepth > 0);
	--curFrameDataDepth;
	if (curFrameDataDepth == 0) {
		BaseFrameData::setThreadFrameData(nullptr);
	}
}

bool SceneEditor::loadCameraPos()
{
	Expects(editorInterface);
	
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

void SceneEditor::setSelectedEntities(Vector<UUID> uuids, Vector<EntityData*> entityDatas)
{
	Expects(uuids.size() == entityDatas.size());

	pushThreadFrameData();
	auto guard = ScopedGuard([&] ()
	{
		popThreadFrameData();
	});
	
	selectedEntityIds.resize(uuids.size());
	for (size_t i = 0; i < uuids.size(); ++i) {
		selectedEntityIds[i] = getWorld().findEntity(uuids[i]).value_or(EntityRef()).getEntityId();
	}

	auto selectedEntities = std_ex::transform(selectedEntityIds, [=] (EntityId id) { return getWorld().getEntity(id); });
	gizmoCollection->setSelectedEntities(std::move(selectedEntities), std::move(entityDatas));

	onEntitiesSelected(selectedEntityIds);
	updateEntityFocused();
}

void SceneEditor::showEntity(const UUID& id)
{
	auto e = getWorld().findEntity(id);
	
	if (e) {
		const auto aabb = assetPreviewGenerator->getSpriteTreeBounds(e.value());
		moveCameraTo2D(aabb.getCenter());
	}
}

void SceneEditor::onToolSet(String& tool, String& componentName, String& fieldName)
{
}

void SceneEditor::setupConsoleCommands(UIDebugConsoleCommands& commands, ISceneEditorWindow& sceneEditor)
{
}

void SceneEditor::refreshAssets()
{
}

Vector<UIPopupMenuItem> SceneEditor::getSceneContextMenu(const Vector2f& mousePos) const
{
	Vector<UIPopupMenuItem> result;

	const auto entities = getRootEntitiesAt(mousePos, true, EntityAtPositionSelectMode::All);
	for (const auto& e: entities) {
		if (e.isSerializable()) {
			const auto name = LocalisedString::fromHardcodedString("Select: " + e.getName());
			const auto tooltip = LocalisedString::fromHardcodedString("Selects entity o" + e.getName());
			const auto icon = editorInterface->getEntityIcon(e.getInstanceUUID().toString());
			result.emplace_back(UIPopupMenuItem{ "entity:" + e.getInstanceUUID(), name, icon, tooltip });
		}
	}

	if (const auto clipboard = getAPI().system->getClipboard()) {
		const auto worldOffset = getWorldOffset();
		const Vector2i scenePos = Vector2i(mousePos.round());
		const Vector2i worldPos = scenePos + Vector2i(worldOffset.value_or(Vector2f()));

		result.emplace_back(UIPopupMenuItem{ "copyToClipboard:" + toString(scenePos), LocalisedString::fromUserString("Copy Scene Coordinates"), {}, LocalisedString::fromUserString("Copy scene coordinates at mouse cursor to clipboard.") });
		if (worldOffset) {
			result.emplace_back(UIPopupMenuItem{ "copyToClipboard:" + toString(worldPos), LocalisedString::fromUserString("Copy World Coordinates"), {}, LocalisedString::fromUserString("Copy world coordinates at mouse cursor to clipboard.") });
		}
	}

	return result;
}

void SceneEditor::onSceneContextMenuSelection(const String& id)
{
	if (id.startsWith("scene:")) {
		editorInterface->openAsset(AssetType::Scene, id.mid(6));
	} else if (id.startsWith("scene_here:")) {
		editorInterface->openAssetHere(AssetType::Scene, id.mid(11));
	} else if (id.startsWith("entity:")) {
		editorInterface->selectEntity(id.mid(7));
	} else if (id.startsWith("copyToClipboard:")) {
		if (const auto clipboard = getAPI().system->getClipboard()) {
			clipboard->setData(id.mid(16));
		}
	}
}

void SceneEditor::onSceneContextMenuHighlight(const String& id)
{
	if (id.startsWith("entity:")) {
		forceFocusEntity = getEntity(UUID(id.mid(7)));
	} else {
		forceFocusEntity.reset();
	}
}

Vector2f SceneEditor::roundPosition(Vector2f pos) const
{
	return roundPosition(pos, camera.getZoom());
}

Vector2f SceneEditor::roundPosition(Vector2f pos, float zoom) const
{
	return (pos * zoom).round() / zoom;
}

Vector2f SceneEditor::roundCameraPosition(Vector2f pos, float z) const
{
	const auto zoom = std::max(1.0f, z);
	const auto offset = Vector2f(viewPort % Vector2i(2, 2)) * 0.5f;

	return ((pos * zoom).round() + offset) / zoom;
}

void SceneEditor::onInit(std::shared_ptr<const UIColourScheme> colourScheme)
{
}

EntityRef SceneEditor::getEntity(const UUID& id) const
{
	return getWorld().findEntity(id).value();
}

bool SceneEditor::doesAreaOverlapSprite(EntityRef& e, Rect4f area) const
{
	const auto* spriteComponent = e.tryGetComponent<SpriteComponent>();
	if (!spriteComponent) {
		return false;
	}
	
	const auto& sprite = spriteComponent->sprite;
	return sprite.hasPointVisible(area);
}

float SceneEditor::getSpriteDepth(EntityRef& e, Rect4f rect) const
{
	const auto* sprite = e.tryGetComponent<SpriteComponent>();
	if (sprite) {
		return static_cast<float>(sprite->layer);
	} else {
		return -std::numeric_limits<float>::infinity();
	}
}

std::unique_ptr<BaseFrameData> SceneEditor::makeFrameData()
{
	return {};
}

void SceneEditor::CameraPanAnimation::stop()
{
	deltas.clear();
	inertiaVel = Vector2f();
}

Vector<EntityRef> SceneEditor::getEntitiesAt(Rect4f area, bool allowUnselectable, EntityAtPositionSelectMode mode) const
{
	Vector<std::pair<EntityRef, float>> temp;
	
	for (auto& e: world->getEntities()) {
		if ((allowUnselectable || e.isSelectable()) && doesAreaOverlapSprite(e, area)) {
			const float depth = getSpriteDepth(e, area);
			temp.emplace_back(e, depth);
		}
	}

	std::sort(temp.begin(), temp.end(), [] (const auto& a, const auto& b) { return a.second > b.second; });

	Vector<EntityRef> result;
	result.reserve(temp.size());
	for (auto& e: temp) {
		if (e.first.isValid()) {
			result.push_back(e.first);
		}
	}
	return result;
}

Vector<EntityRef> SceneEditor::getRootEntitiesAt(Vector2f point, bool allowUnselectable, EntityAtPositionSelectMode mode) const
{
	return getRootEntitiesAt(Rect4f(point, point), allowUnselectable, mode);
}

Vector<EntityRef> SceneEditor::getRootEntitiesAt(Rect4f area, bool allowUnselectable, EntityAtPositionSelectMode mode) const
{
	pushThreadFrameData();
	auto guard = ScopedGuard([&] ()
	{
		popThreadFrameData();
	});

	const auto entities = getEntitiesAt(area, allowUnselectable, mode);

	Vector<EntityRef> result;
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

EntityRef SceneEditor::getRootEntityAt(Vector2f point, bool allowUnselectable, EntityAtPositionSelectMode mode) const
{
	const auto entities = getRootEntitiesAt(point, allowUnselectable, mode);
	if (entities.empty()) {
		highlightDelta = 0;
		return EntityRef();
	}

	highlightDelta = modulo(highlightDelta, static_cast<int>(entities.size()));
	
	return entities.at(highlightDelta);
}

void SceneEditor::onClick(const SceneEditorInputState& input, SceneEditorOutputState& output, bool canSelectSprite, bool canSelectEntities)
{
	if (!input.mousePos) {
		return;
	} 
	if (input.spaceHeld || input.shiftHeld || (input.altHeld && !input.ctrlHeld)) {
		return;
	}
	if (!canSelectEntities) {
		return;
	}
	
	output.selectionMode = input.ctrlHeld ? UIList::SelectionMode::CtrlSelect : UIList::SelectionMode::Normal;
	const auto bestEntity = getRootEntityAt(input.mousePos.value(), false, canSelectSprite ? EntityAtPositionSelectMode::All : EntityAtPositionSelectMode::NonSprite);
	if (bestEntity.isValid()) {
		output.newSelection = Vector<UUID>();
		output.newSelection->push_back(bestEntity.getInstanceUUID());
	} else if (canSelectSprite) {
		output.newSelection = Vector<UUID>();
	}
}

void SceneEditor::onStartSelectionBox()
{
	if (!selBox) {
		selBoxStartSelectedEntities.clear();
		selBoxStartSelectedEntities.reserve(selectedEntityIds.size());
		for (const auto& e: selectedEntityIds) {
			selBoxStartSelectedEntities.push_back(world->getEntity(e).getInstanceUUID());
		}
	}
	selBox = Rect4f(holdMouseStart.value(), mousePos.value());
}

void SceneEditor::onSelectionBox(const SceneEditorInputState& input, SceneEditorOutputState& output)
{
	const auto& entities = getRootEntitiesAt(*input.selectionBox, false, EntityAtPositionSelectMode::All);
	Vector<UUID> results;

	if (input.shiftHeld || input.altHeld) {
		results = selBoxStartSelectedEntities;

		if (input.shiftHeld) {
			// Add to selection
			for (auto& e: entities) {
				if (!std_ex::contains(results, e.getInstanceUUID())) {
					results.push_back(e.getInstanceUUID());
				}
			}
		} else if (input.altHeld) {
			// Subtract from selection
			std::set<UUID> toErase;
			for (auto& e: entities) {
				toErase.insert(e.getInstanceUUID());
			}
			std_ex::erase_if(results, [&] (UUID id) { return std_ex::contains(toErase, id); });
		}
	} else {
		results.reserve(entities.size());
		for (auto& e: entities) {
			results.push_back(e.getInstanceUUID());
		}
	}

	output.newSelection = std::move(results);
	output.selectionMode = UIList::SelectionMode::Normal;
}

void SceneEditor::setAssetPreviewGenerator(AssetPreviewGenerator& generator)
{
	assetPreviewGenerator = &generator;
}

Transform2DComponent* SceneEditor::getTransform(const String& entityId)
{
	auto e = world->findEntity(UUID(entityId));
	if (e.has_value()) {
		return e->tryGetComponent<Transform2DComponent>();
	}
	return nullptr;
}

void SceneEditor::initializeEntityValidator(EntityValidator& validator)
{
	validator.addDefaults();
}

bool SceneEditor::shouldDrawOutline(const Sprite& sprite) const
{
	return true;
}

String SceneEditor::getSceneNameForComments(AssetType assetType, const String& assetId) const
{
	return assetId;
}

void SceneEditor::setGameInstance(Game* game)
{
	this->game = game;
}

Game* SceneEditor::getGameInstance() const
{
	return game;
}

void SceneEditor::onEntityModified(const String& id, const EntityData* prevData, const EntityData& newData)
{
}
