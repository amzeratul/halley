#pragma once

#include <halley/maths/vector2.h>
#include <halley/maths/rect.h>
#include <memory>
#include <halley/resources/resource.h>
#include <halley/text/halleystring.h>
#include <halley/data_structures/hash_map.h>
#include <gsl/span>
#include "halley/maths/vector4.h"

namespace Halley
{
	class Sprite;
	class Resources;
	class Serializer;
	class Deserializer;
	class ResourceDataStatic;
	class Texture;
	class ResourceLoader;
	class Material;
	class SpriteSheet;

	class SpriteSheetEntry
	{
	public:
		Vector2f pivot;
		Vector2i origPivot;
		Vector2f size;
		Rect4f coords;
		Vector4s trimBorder;
		Vector4s slices;
		int duration = 0;
		bool rotated = false;
		bool sliced = false;

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);

#ifdef ENABLE_HOT_RELOAD
		SpriteSheet* parent = nullptr;
		uint32_t idx = 0;
#endif
	};

	class SpriteSheetFrameTag
	{
	public:
		String name;
		int from = 0;
		int to = 0;

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);
	};

	class SpriteHotReloader {
	public:
		virtual ~SpriteHotReloader() = default;

#ifdef ENABLE_HOT_RELOAD
		void addSprite(Sprite* sprite, uint32_t idx) const;
		void removeSprite(Sprite* sprite) const;
		void clearSpriteRefs();

	protected:
		class SpritePointerHasher {
		public:
			std::size_t operator()(Sprite* ptr) const noexcept;
		};
		
		mutable std::unordered_map<Sprite*, uint32_t, SpritePointerHasher> spriteRefs;
#endif
	};
	
	class SpriteSheet final : public Resource, public SpriteHotReloader
	{
	public:
		SpriteSheet();
		~SpriteSheet();
		
		const std::shared_ptr<const Texture>& getTexture() const;
		const SpriteSheetEntry& getSprite(const String& name) const;
		const SpriteSheetEntry& getSprite(size_t idx) const;

		const std::vector<SpriteSheetFrameTag>& getFrameTags() const;
		std::vector<String> getSpriteNames() const;
		const HashMap<String, uint32_t>& getSpriteNameMap() const;

		size_t getSpriteCount() const;
		size_t getIndex(const String& name) const;
		bool hasSprite(const String& name) const;

		void loadJson(gsl::span<const gsl::byte> data);

		void addSprite(String name, const SpriteSheetEntry& sprite);
		void setTextureName(String name);

		std::shared_ptr<Material> getMaterial(const String& name) const;
		void setDefaultMaterialName(String materialName);
		const String& getDefaultMaterialName() const;
		void clearMaterialCache() const;

		static std::unique_ptr<SpriteSheet> loadResource(ResourceLoader& loader);
		constexpr static AssetType getAssetType() { return AssetType::SpriteSheet; }
		void reload(Resource&& resource) override;

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);

	private:
		constexpr static int version = 1;
		
		Resources* resources = nullptr;

		std::vector<SpriteSheetEntry> sprites;
		HashMap<String, uint32_t> spriteIdx;
		std::vector<SpriteSheetFrameTag> frameTags;

		String textureName;
		mutable std::shared_ptr<const Texture> texture;

		String defaultMaterialName;
		mutable HashMap<String, std::weak_ptr<Material>> materials;

		void loadTexture(Resources& resources) const;
		void assignIds();
	};

	class SpriteResource final : public Resource, public SpriteHotReloader
	{
	public:
		SpriteResource();
		SpriteResource(const std::shared_ptr<const SpriteSheet>& spriteSheet, size_t idx);
		~SpriteResource();

		const SpriteSheetEntry& getSprite() const;
		size_t getIdx() const;
		std::shared_ptr<const SpriteSheet> getSpriteSheet() const;

		std::shared_ptr<Material> getMaterial(const String& name) const;
		const String& getDefaultMaterialName() const;

		constexpr static AssetType getAssetType() { return AssetType::Sprite; }
		static std::unique_ptr<SpriteResource> loadResource(ResourceLoader& loader);
		void reload(Resource&& resource) override;

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);

	private:
		std::weak_ptr<const SpriteSheet> spriteSheet;
		size_t idx = -1;
		Resources* resources = nullptr;
	};
}
