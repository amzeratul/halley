#include "scene_editor/scene_editor.h"
#include "world.h"
#include "halley/core/api/halley_api.h"
#include "halley/core/game/halley_statics.h"
#include "halley/core/graphics/sprite/sprite.h"
#include "halley/core/graphics/render_context.h"
#include "system.h"
#include "registry.h"
#include "components/sprite_component.h"
#include "components/camera_component.h"
#include "components/transform_2d_component.h"

using namespace Halley;

SceneEditor::SceneEditor() = default;

SceneEditor::~SceneEditor() = default;

void SceneEditor::init(SceneEditorContext& context)
{
	api = context.api;
	resources = context.resources;
	editorResources = context.editorResources;
	gizmoCollection = context.gizmos;

	api->core->getStatics().setupGlobals();
}

void SceneEditor::update(Time t, SceneEditorInputState inputState, SceneEditorOutputState& outputState)
{
	// Update world
	world->step(TimeLine::FixedUpdate, t);
	world->step(TimeLine::VariableUpdate, t);

	// Update camera
	auto cameraEntity = world->getEntity(cameraEntityId);
	auto& cameraComponent = cameraEntity.getComponent<CameraComponent>();
	auto& transformComponent = cameraEntity.getComponent<Transform2DComponent>();
	camera.setPosition(transformComponent.getGlobalPosition()).setZoom(cameraComponent.zoom);

	// Update input state
	inputState.mousePos = camera.screenToWorld(inputState.rawMousePos, inputState.viewRect);

	// Update gizmos
	gizmoCollection->update(t, camera, inputState, outputState);
}

void SceneEditor::render(RenderContext& rc)
{
	// Render world
	world->render(rc);

	// Render gizmos
	rc.with(camera).bind([&] (Painter& painter)
	{
		gizmoCollection->draw(painter);
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
	cameraEntityId = createCamera();

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

std::unique_ptr<World> SceneEditor::doCreateWorld()
{
	auto world = std::make_unique<World>(getAPI(), getGameResources(), true, createComponent);
	const auto& sceneConfig = getGameResources().get<ConfigFile>(getSceneEditorStageName())->getRoot();
	world->loadSystems(sceneConfig, createSystem);

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

EntityId SceneEditor::createCamera()
{
	return getWorld().createEntity("editorCamera")
		.addComponent(Transform2DComponent(Vector2f(0, 0)))
		.addComponent(CameraComponent(1.0f, Colour4f(0.2f, 0.2f, 0.2f), 0x7FFFFFFF, 0))
		.getEntityId();
}

void SceneEditor::onEntitySelected(std::optional<EntityRef> entity)
{
}

EntityId SceneEditor::getCameraId()
{
	return cameraEntityId;
}

void SceneEditor::dragCamera(Vector2f amount)
{
	auto camera = getWorld().getEntity(cameraEntityId);
	const float zoom = camera.getComponent<CameraComponent>().zoom;
	auto& transform = camera.getComponent<Transform2DComponent>();
	transform.setGlobalPosition(roundPosition(transform.getGlobalPosition() + amount / zoom));
}

void SceneEditor::moveCameraTo2D(Vector2f pos)
{
	auto camera = getWorld().getEntity(cameraEntityId);
	auto& transform = camera.getComponent<Transform2DComponent>();
	transform.setGlobalPosition(roundPosition(pos));
}

void SceneEditor::changeZoom(int amount, Vector2f cursorPosRelToCamera)
{
	if (amount == 0) {
		return;
	}

	auto cameraEntity = getWorld().getEntity(cameraEntityId);
	auto& camera = cameraEntity.getComponent<CameraComponent>();
	auto& transform = cameraEntity.getComponent<Transform2DComponent>();
	const float prevZoom = camera.zoom;

	// Zoom
	const int curLevel = lroundf(std::log2f(clamp(camera.zoom, 1.0f / 32.0f, 32.0f)));
	camera.zoom = std::powf(2.0f, float(clamp(curLevel + amount, -5, 5)));

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

Vector2f SceneEditor::roundPosition(Vector2f pos) const
{
	return roundPosition(pos, camera.getZoom());
}

Vector2f SceneEditor::roundPosition(Vector2f pos, float zoom) const
{
	return (pos * zoom).round() / zoom;
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
