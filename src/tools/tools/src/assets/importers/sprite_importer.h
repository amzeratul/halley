#pragma once
#include "halley/plugin/iasset_importer.h"
#include "halley/file_formats/image.h"
#include "halley/core/graphics/sprite/sprite_sheet.h"
#include "halley/data_structures/bin_pack.h"

namespace Halley
{
	class Animation;
	class SpriteImporter;
	
	struct ImageData
	{
		int frameNumber;
		int duration;
		String sequenceName;
		String direction;
		Rect4i clip;
		Vector2i pivot;
		Vector4s slices;

		std::unique_ptr<Image> img;
		std::vector<String> filenames;

		bool operator==(const ImageData& other) const;
		bool operator!=(const ImageData& other) const;

	private:
		friend class SpriteImporter;
		
		bool isDuplicate = false;
		std::vector<ImageData*> duplicatesOfThis;
	};

	class SpriteImporter : public IAssetImporter
	{
	public:
		ImportAssetType getType() const override { return ImportAssetType::Sprite; }

		void import(const ImportingAsset& asset, IAssetCollector& collector) override;
		String getAssetId(const Path& file, const std::optional<Metadata>& metadata) const override;

	private:
		Animation generateAnimation(const String& spriteName, const String& spriteSheetName, const String& materialName, const std::vector<ImageData>& frameData);

		std::unique_ptr<Image> generateAtlas(const String& atlasName, std::vector<ImageData>& images, SpriteSheet& spriteSheet);
		std::unique_ptr<Image> makeAtlas(const std::vector<BinPackResult>& result, SpriteSheet& spriteSheet);
		Vector2i computeAtlasSize(const std::vector<BinPackResult>& results) const;
		void markDuplicates(std::vector<ImageData>& images) const;

		std::vector<ImageData> splitImagesInGrid(const std::vector<ImageData>& images, Vector2i grid);
	};
}
