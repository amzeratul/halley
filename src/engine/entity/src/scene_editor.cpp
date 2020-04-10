#include "scene_editor.h"
#include "world.h"
#include "halley/core/api/halley_api.h"
#include "halley/core/game/halley_statics.h"
#include "halley/core/graphics/sprite/sprite.h"
#include "system.h"
#include "registry.h"

#define DONT_INCLUDE_HALLEY_HPP
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
}

void SceneEditor::update(Time t)
{
	world->step(TimeLine::FixedUpdate, t);
	world->step(TimeLine::VariableUpdate, t);
}

void SceneEditor::render(RenderContext& rc)
{
	world->render(rc);
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
{}

void SceneEditor::showEntity(const UUID& id)
{
	auto e = getWorld().findEntity(id);
	
	if (e) {
		const auto aabb = getSpriteTreeBounds(e.value());
		moveCameraTo2D(aabb.getCenter());
	}
}

Rect4f SceneEditor::getSpriteTreeBounds(EntityRef& e)
{
	std::optional<Rect4f> rect;
	doGetSpriteTreeBounds(e, rect);
	return rect.value_or(Rect4f());
}

void SceneEditor::doGetSpriteTreeBounds(EntityRef& e, std::optional<Rect4f>& rect)
{
	auto cur = getSpriteBounds(e);
	if (!rect) {
		rect = cur;
	}
	if (cur) {
		rect = rect->merge(*cur);
	}

	auto& world = getWorld();
	for (auto& c: e.getRawChildren()) {
		auto child = EntityRef(*c, world);
		doGetSpriteTreeBounds(child, rect);
	}
}

std::optional<Rect4f> SceneEditor::getSpriteBounds(EntityRef& e)
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
