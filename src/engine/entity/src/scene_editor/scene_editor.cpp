#include "scene_editor/scene_editor.h"
#include "world.h"
#include "halley/core/api/halley_api.h"
#include "halley/core/game/halley_statics.h"
#include "halley/core/graphics/sprite/sprite.h"
#include "halley/core/graphics/render_context.h"
#include "system.h"
#include "registry.h"
#include "scene_editor/scene_editor_gizmo_collection.h"
#include "components/sprite_component.h"
#include "components/camera_component.h"
#include "components/transform_2d_component.h"

using namespace Halley;

SceneEditor::SceneEditor() = default;

SceneEditor::~SceneEditor() = default;

void SceneEditor::init(SceneEditorContext& context)
{
	context.api->core->getStatics().setupGlobals();

	world = createWorld(context);
	createServices(*world, context);
	createEntities(*world, context);
	cameraEntityId = createCamera();

	gizmoCollection = std::make_unique<SceneEditorGizmoCollection>();
}

void SceneEditor::update(Time t)
{
	// Update world
	world->step(TimeLine::FixedUpdate, t);
	world->step(TimeLine::VariableUpdate, t);

	// Update camera
	auto cameraEntity = world->getEntity(cameraEntityId);
	auto& cameraComponent = cameraEntity.getComponent<CameraComponent>();
	auto& transformComponent = cameraEntity.getComponent<Transform2DComponent>();
	camera.setPosition(transformComponent.getGlobalPosition()).setZoom(cameraComponent.zoom);

	// Update gizmos
	updateGizmos(t);
}

void SceneEditor::render(RenderContext& rc)
{
	// Render world
	world->render(rc);

	// Render gizmos
	rc.with(camera).bind([&] (Painter& painter)
	{
		drawGizmos(painter);
	});
}

World& SceneEditor::getWorld()
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

std::unique_ptr<World> SceneEditor::createWorld(SceneEditorContext& context)
{
	auto world = std::make_unique<World>(*context.api, *context.resources, true, createComponent);
	const auto& sceneConfig = context.resources->get<ConfigFile>(getSceneEditorStageName())->getRoot();
	world->loadSystems(sceneConfig, createSystem);

	return world;
}

void SceneEditor::createServices(World& world, SceneEditorContext& context)
{
}

void SceneEditor::createEntities(World& world, SceneEditorContext& context)
{
}

String SceneEditor::getSceneEditorStageName()
{
	return "stages/scene_editor";
}

EntityId SceneEditor::createCamera()
{
	return getWorld().createEntity("editorCamera")
		.addComponent(Transform2DComponent(Vector2f(0, 0)))
		.addComponent(CameraComponent(1.0f, Colour4f(0.5f, 0.5f, 0.5f), 0x7FFFFFFF, 0))
		.getEntityId();
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
	transform.setGlobalPosition(transform.getGlobalPosition() + amount / zoom);
}

void SceneEditor::moveCameraTo2D(Vector2f pos)
{
	auto camera = getWorld().getEntity(cameraEntityId);
	auto& transform = camera.getComponent<Transform2DComponent>();
	transform.setGlobalPosition(pos);
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
	const int curLevel = lroundf(std::log2f(clamp(camera.zoom, 1.0f / 1024.0f, 1024.0f)));
	camera.zoom = std::powf(2.0f, float(clamp(curLevel + amount, -10, 10)));

	// Translate to keep fixed point
	const Vector2f translate = cursorPosRelToCamera * (1.0f / prevZoom - 1.0f / camera.zoom);
	transform.setGlobalPosition(transform.getGlobalPosition() + translate);
}

void SceneEditor::setSelectedEntity(const UUID& id)
{
	selectedBounds.reset();
	selectedEntity.reset();

	if (id.isValid()) {
		selectedEntity = getWorld().findEntity(id);
		if (selectedEntity) {
			selectedBounds = getSpriteTreeBounds(selectedEntity.value());
		}

		gizmoCollection->setSelectedEntity(selectedEntity);
	}
}

void SceneEditor::showEntity(const UUID& id)
{
	auto e = getWorld().findEntity(id);
	
	if (e) {
		const auto aabb = getSpriteTreeBounds(e.value());
		moveCameraTo2D(aabb.getCenter());
	}
}

void SceneEditor::setTool(SceneEditorTool tool)
{
	gizmoCollection->setTool(tool);
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
		auto child = EntityRef(*c, *world);
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

void SceneEditor::updateGizmos(Time t)
{
	gizmoCollection->update(t);
}

void SceneEditor::drawGizmos(Painter& painter) const
{
	if (selectedBounds) {
		painter.drawRect(selectedBounds.value(), 1.0f, Colour4f(0.6, 0.6f, 0.6f));
	}

	gizmoCollection->draw(painter);
}
