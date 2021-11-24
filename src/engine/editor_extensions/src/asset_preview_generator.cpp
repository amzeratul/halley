#define DONT_INCLUDE_HALLEY_HPP

#include "asset_preview_generator.h"

#include <components/sprite_component.h>

#include "halley/core/api/halley_api.h"
#include "halley/core/graphics/camera.h"
#include "halley/core/graphics/painter.h"
#include "halley/core/graphics/render_context.h"
#include "halley/core/graphics/material/material.h"
#include "halley/core/graphics/material/material_definition.h"
#include "halley/core/graphics/render_target/render_surface.h"
#include "halley/core/graphics/render_target/render_target_texture.h"
#include "halley/core/graphics/sprite/animation_player.h"
#include "halley/entity/entity_factory.h"
#include "halley/entity/entity_scene.h"
#include "halley/entity/components/transform_2d_component.h"
#include "halley/file_formats/image.h"
using namespace Halley;

AssetPreviewGenerator::AssetPreviewGenerator(const HalleyAPI& api, Resources& resources)
	: api(api)
	, resources(resources)
	, renderExecutor(renderQueue)
{
	worldRenderTarget = api.video->createTextureRenderTarget();

	auto& system = *api.system;
	previewExecutor = std::make_unique<SingleThreadExecutor>("Preview", [&system] (const String& name, std::function<void()> f)
	{
		return system.createThread(name, ThreadPriority::Normal, std::move(f));
	});
}

Future<AssetPreviewData> AssetPreviewGenerator::getAssetPreviewData(AssetType assetType, const String& id, Vector2i size)
{
	if (assetType == AssetType::Prefab || assetType == AssetType::Scene) {
		return getPrefabPreviewData(assetType, id, size);
	} else if (assetType == AssetType::Sprite || assetType == AssetType::Animation) {
		return getSpritePreviewData(assetType, id, size);
	}

	return Future<AssetPreviewData>::makeImmediate({});
}

void AssetPreviewGenerator::render(RenderContext& rc)
{
	curRC = &rc;
	renderExecutor.runPending();
	curRC = nullptr;
}

AssetPreviewData AssetPreviewGenerator::makeSpritePreviewData(AssetType assetType, const String& id, Vector2i size, RenderContext& curRC)
{
	RenderSurface surface(*(api.video), RenderSurfaceOptions());
	surface.setSize(size);

	auto image = std::make_shared<Image>(Image::Format::RGBA, size);

	Sprite sprite;
	if (assetType == AssetType::Sprite) {
		sprite = Sprite().setImage(resources, id);
	} else if (assetType == AssetType::Animation) {
		auto player = AnimationPlayer(resources.get<Animation>(id));
		player.update(0);
		player.updateSprite(sprite);
	}

	auto spriteSize = sprite.getAABB().getSize().round();
	const int maxDimension = std::max(8, nextPowerOf2<int>(lroundl(std::max(spriteSize.x, spriteSize.y))));
	const float zoom = std::min(128.0f / maxDimension, 3.0f);

	Camera camera;
	camera.setPosition(sprite.getAABB().getCenter());
	camera.setZoom(zoom);

	auto rc = curRC.with(surface.getRenderTarget()).with(camera);
	rc.bind([&] (Painter& painter)
	{
		painter.clear(Colour4f(0, 0, 0, 0));
		sprite.draw(painter);
	});
	curRC.bind([&] (Painter& painter)
	{
		surface.getRenderTarget().getTexture(0)->copyToImage(painter, *image);
	});

	AssetPreviewData data;
	data.image = std::move(image);
	return data;
}

Future<AssetPreviewData> AssetPreviewGenerator::getSpritePreviewData(AssetType assetType, const String& id, Vector2i size)
{
	return Concurrent::execute(renderQueue, [=] () -> AssetPreviewData
	{
		return makeSpritePreviewData(assetType, id, size, *curRC);
	});
}

Future<AssetPreviewData> AssetPreviewGenerator::getPrefabPreviewData(AssetType assetType, const String& id, Vector2i size)
{
	return Concurrent::execute(previewExecutor->getQueue(), [this, id, size] () -> AssetPreviewInfo {
		// Create world
		auto world = std::shared_ptr<World>(World::make(api, resources, "stages/prefab_preview", true));
		setupPrefabPreviewWorld(*world, size);

		// Add prefab
		const auto prefab = resources.get<Prefab>(id);
		auto entityFactory = std::make_shared<EntityFactory>(*world, resources);
		auto sceneCreated = entityFactory->createScene(prefab, true);

		// Run a simulation step
		world->step(TimeLine::VariableUpdate, 0.1);
		
		// Get main entity info
		Rect4f spriteBounds;
		String name;
		for (auto& e: sceneCreated.getEntities()) {
			if (!e.hasParent()) {
				spriteBounds = getSpriteTreeBounds(e);
				name = e.getName();
				break;
			}
		}
		if (name.isEmpty()) {
			name = Path(prefab->getAssetId()).getFilename().toString();
		}

		// Compute bounds
		const int maxDimension = std::max(8, nextPowerOf2<int>(lroundl(std::max(spriteBounds.getWidth(), spriteBounds.getHeight()))));
		const Vector2i screenCentre = Vector2i(spriteBounds.getCenter());
		const float zoom = std::min(128.0f / maxDimension, 3.0f);
		const Vector2i diag = Vector2i(maxDimension, maxDimension) / 2;
		auto rect = Rect4i(screenCentre - diag, screenCentre + diag);

		// Setup camera
		createPrefabPreviewCamera(*entityFactory, *world, spriteBounds, zoom);

		// Simulate again
		world->step(TimeLine::VariableUpdate, 0.1);

		return AssetPreviewInfo{ world, rect, zoom, name };
	}).then(renderQueue, [this] (AssetPreviewInfo info) -> AssetPreviewData
	{
		auto rc2 = curRC->with(*worldRenderTarget);
		return renderAssetPreview(info, rc2);
	});
}

void AssetPreviewGenerator::setupPrefabPreviewWorld(World& world, Vector2i canvasSize)
{
}

void AssetPreviewGenerator::createPrefabPreviewCamera(EntityFactory& entityFactory, World& world, Rect4f spriteBounds, float zoom)
{
}

AssetPreviewData AssetPreviewGenerator::renderAssetPreview(const AssetPreviewInfo& info, RenderContext& rc)
{
	return AssetPreviewData{};
}

Rect4f AssetPreviewGenerator::getSpriteTreeBounds(const EntityRef& e) const
{
	std::optional<Rect4f> rect;
	doGetSpriteTreeBounds(e, rect);
	return rect.value_or(Rect4f());
}

void AssetPreviewGenerator::doGetSpriteTreeBounds(const EntityRef& e, std::optional<Rect4f>& rect) const
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

std::optional<Rect4f> AssetPreviewGenerator::getSpriteBounds(const EntityRef& e) const
{
	const auto transform2d = e.tryGetComponent<Transform2DComponent>();

	if (transform2d) {
		const auto sprite = e.tryGetComponent<SpriteComponent>();

		if (sprite) {
			const int mask = sprite->mask.value_or(sprite->sprite.hasMaterial() ? sprite->sprite.getMaterial().getDefinition().getDefaultMask() : 0);
			if (isSpriteVisibleOnCamera(sprite->sprite, mask)) {
				return transform2d->getSpriteAABB(sprite->sprite);
			}
		} else {
			auto pos = transform2d->getGlobalPosition();
			return Rect4f(pos, pos);
		}
	}
	return {};
}

bool AssetPreviewGenerator::isSpriteVisibleOnCamera(const Sprite& sprite, int mask) const
{
	return true;
}
