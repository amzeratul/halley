#pragma once
#include "halley/concurrency/future.h"
#include "halley/core/graphics/render_target/render_target_texture.h"
#include "halley/core/graphics/sprite/sprite.h"
#include "halley/entity/world.h"
#include "halley/maths/vector2.h"
#include "halley/resources/resource.h"

namespace Halley {
	class EntityFactory;
	class Image;

	struct AssetPreviewInfo {
		std::shared_ptr<World> world;
		Rect4i rect;
		float zoom;
		String name;
	};

	struct AssetPreviewData {
		Sprite sprite;
		String name;
		std::shared_ptr<Image> image;
	};

	class AssetPreviewGenerator {
	public:
		AssetPreviewGenerator(const HalleyAPI& api, Resources& resources);
		virtual ~AssetPreviewGenerator() = default;

		Future<AssetPreviewData> getAssetPreviewData(AssetType assetType, const String& id, Vector2i size);
		void render(RenderContext& rc);

		Rect4f getSpriteTreeBounds(const EntityRef& e) const;
		std::optional<Rect4f> getSpriteBounds(const EntityRef& e) const;

	protected:
		const HalleyAPI& api;
		Resources& resources;

		AssetPreviewData makeSpritePreviewData(AssetType assetType, const String& id, Vector2i size, RenderContext& renderContext);
		virtual Future<AssetPreviewData> getSpritePreviewData(AssetType assetType, const String& id, Vector2i size);
		virtual Future<AssetPreviewData> getPrefabPreviewData(AssetType assetType, const String& id, Vector2i size);

		virtual void setupPrefabPreviewWorld(World& world, Vector2i canvasSize);
		virtual void createPrefabPreviewCamera(EntityFactory& entityFactory, World& world, Rect4f spriteBounds, float zoom);
		virtual AssetPreviewData renderAssetPreview(const AssetPreviewInfo& info, RenderContext& rc);
		virtual bool isSpriteVisibleOnCamera(const Sprite& sprite, int mask) const;

	private:
		ExecutionQueue renderQueue;
		Executor renderExecutor;
		std::unique_ptr<SingleThreadExecutor> previewExecutor;
		RenderContext* curRC = nullptr;
		std::unique_ptr<TextureRenderTarget> worldRenderTarget;

		void doGetSpriteTreeBounds(const EntityRef& e, std::optional<Rect4f>& rect) const;
	};
}
