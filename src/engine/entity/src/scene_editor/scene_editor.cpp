#include "scene_editor/scene_editor.h"
#include "world.h"
#include "halley/core/api/halley_api.h"
#include "halley/core/game/halley_statics.h"
#include "halley/core/graphics/sprite/sprite.h"
#include "halley/core/graphics/render_context.h"
#include "system.h"
#include "components/sprite_component.h"
#include "components/camera_component.h"
#include "components/transform_2d_component.h"
#include "halley/core/graphics/sprite/sprite_sheet.h"
#include "halley/core/graphics/text/font.h"

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
	inputState.mousePos = camera.screenToWorld(inputState.rawMousePos, inputState.viewRect);
	mousePos = inputState.mousePos;
	if (!inputState.leftClickHeld) {
		holdMouseStart.reset();
	}
	if (holdMouseStart && (holdMouseStart.value() - mousePos).length() > 3) {
		selBox = Rect4f(holdMouseStart.value(), mousePos);
	}
	inputState.selectionBox = selBox;
	if (!inputState.leftClickHeld) {
		// Make sure this happens after inputState.selectionBox is set
		// This is so you still get a final selection box on release
		selBox.reset();
	}

	// Update gizmos
	const bool hasHighlightedGizmo = gizmoCollection->update(t, camera, inputState, outputState);
	if (!hasHighlightedGizmo && inputState.leftClickPressed) {
		gizmoCollection->deselect();
		onClick(inputState, outputState);
	}

	// Start dragging
	if (!hasHighlightedGizmo && inputState.leftClickPressed) {
		holdMouseStart = mousePos;
	}
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

void SceneEditor::createWorld()
{
	world = doCreateWorld();
	createServices(*world);
	createEntities(*world);
	cameraEntityIds = createCamera();

	onInit();
}

World& SceneEditor::getWorld() const
{
	Expects(world);
	return *world;
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

void SceneEditor::onSceneLoaded(AssetType assetType, const String& assetId)
{
}

std::unique_ptr<World> SceneEditor::doCreateWorld()
{
	auto world = std::make_unique<World>(getAPI(), getGameResources(), true, CreateEntityFunctions::getCreateComponent());
	const auto& sceneConfig = getGameResources().get<ConfigFile>(getSceneEditorStageName())->getRoot();
	world->loadSystems(sceneConfig, CreateEntityFunctions::getCreateSystem());

	return world;
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
	String drawStr = "[" + toString(camera.getZoom()) + "x] " + Vector2i(mousePos.round()).toString();
	std::vector<ColourOverride> colours;

	if (selectedEntity) {
		const auto* t2d = selectedEntity.value().tryGetComponent<Transform2DComponent>();
		if (t2d) {
			colours.emplace_back(drawStr.size(), Colour4f(0.5f, 0.5f, 0.5f));
			drawStr += " " + t2d->inverseTransformPoint(mousePos.round()).toString();
		}
	}
	
	coordinateInfo
		.setPosition(drawPos)
		.setText(drawStr)
		.setColourOverride(colours)
		.draw(painter);
}

std::vector<EntityId> SceneEditor::createCamera()
{
	return std::vector<EntityId>({
		getWorld().createEntity("editorCamera")
			.addComponent(Transform2DComponent(Vector2f(0, 0)))
			.addComponent(CameraComponent(1.0f, Colour4f(0.2f, 0.2f, 0.2f), 0, 1.0f, 0x7FFFFFFF, 0))
			.getEntityId()
	});
}

void SceneEditor::onEntitySelected(std::optional<EntityRef> entity)
{
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
}

void SceneEditor::moveCameraTo2D(Vector2f pos)
{
	auto camera = getWorld().getEntity(cameraEntityIds.at(0));
	auto& transform = camera.getComponent<Transform2DComponent>();
	transform.setGlobalPosition(roundPosition(pos));
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
}

void SceneEditor::setSelectedEntity(const UUID& id, ConfigNode& entityData)
{
	const auto curId = selectedEntity ? selectedEntity.value().getUUID() : UUID();
	if (id != curId) {
		selectedEntity.reset();
		if (id.isValid()) {
			selectedEntity = getWorld().findEntity(id);
		}
	}	

	gizmoCollection->setSelectedEntity(selectedEntity, entityData);

	onEntitySelected(selectedEntity);
}

void SceneEditor::onEntityAdded(const UUID& id, const ConfigNode& entityData)
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

void SceneEditor::onEntityMoved(const UUID& id, const ConfigNode& entityData)
{
	if (id.isValid()) {
		onEntityMoved(getEntity(id), entityData);
	}
}

void SceneEditor::onEntityModified(const UUID& id, const ConfigNode& entityData)
{
	if (id.isValid()) {
		onEntityModified(getEntity(id), entityData);
	}
}

void SceneEditor::onEntityModified(EntityRef entity, const ConfigNode& entityData)
{}

void SceneEditor::onEntityAdded(EntityRef entity, const ConfigNode& entityData)
{}

void SceneEditor::onEntityRemoved(EntityRef entity)
{}

void SceneEditor::onEntityMoved(EntityRef entity, const ConfigNode& entityData)
{}

void SceneEditor::showEntity(const UUID& id)
{
	auto e = getWorld().findEntity(id);
	
	if (e) {
		const auto aabb = getSpriteTreeBounds(e.value());
		moveCameraTo2D(aabb.getCenter());
	}
}

ConfigNode SceneEditor::onToolSet(SceneEditorTool tool, const String& componentName, const String& fieldName, ConfigNode options)
{
	return options;
}

Rect4f SceneEditor::getSpriteTreeBounds(const EntityRef& e)
{
	std::optional<Rect4f> rect;
	doGetSpriteTreeBounds(e, rect);
	return rect.value_or(Rect4f());
}

void SceneEditor::doGetSpriteTreeBounds(const EntityRef& e, std::optional<Rect4f>& rect)
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

std::optional<Rect4f> SceneEditor::getSpriteBounds(const EntityRef& e)
{
	const auto transform2d = e.tryGetComponent<Transform2DComponent>();

	if (transform2d) {
		const auto sprite = e.tryGetComponent<SpriteComponent>();

		if (sprite) {
			return transform2d->getSpriteAABB(sprite->sprite);
		} else {
			auto pos = transform2d->getGlobalPosition();
			return Rect4f(pos, pos);
		}
	}
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

void SceneEditor::onInit()
{
}

EntityRef SceneEditor::getEntity(const UUID& id) const
{
	const auto curId = selectedEntity ? selectedEntity.value().getUUID() : UUID();
	if (curId == id) {
		return selectedEntity.value();
	} else {
		return getWorld().findEntity(id).value();
	}
}

bool SceneEditor::isPointInSprite(EntityRef& e, Vector2f point) const
{
	const auto* transform2d = e.tryGetComponent<Transform2DComponent>();
	const auto* spriteComponent = e.tryGetComponent<SpriteComponent>();

	if (!transform2d || !spriteComponent) {
		return false;
	}
	
	const auto& sprite = spriteComponent->sprite;
	const auto localPoint = transform2d->inverseTransformPoint(point);
	return sprite.isPointVisible(localPoint);
}

int SceneEditor::getSpriteLayer(EntityRef& e) const
{
	const auto* sprite = e.tryGetComponent<SpriteComponent>();
	if (sprite) {
		return sprite->layer;
	} else {
		return std::numeric_limits<int>::lowest();
	}
}

void SceneEditor::onClick(const SceneEditorInputState& input, SceneEditorOutputState& output)
{
	if (input.spaceHeld || input.ctrlHeld || input.shiftHeld) {
		return;
	}
	
	EntityRef bestEntity;
	int bestLayer = std::numeric_limits<int>::lowest();
	
	for (auto& e: world->getEntities()) {
		if (isPointInSprite(e, input.mousePos)) {
			const int layer = getSpriteLayer(e);
			if (layer > bestLayer) {
				bestLayer = layer;
				bestEntity = e;
			}
		}
	}

	if (bestEntity.isValid()) {
		std::vector<UUID> uuids;
		for (auto e = bestEntity; e.isValid(); e = e.getParent()) {
			uuids.push_back(e.getUUID());
		}
		output.newSelection = uuids;
	} else {
		output.newSelection.reset();
	}
}
