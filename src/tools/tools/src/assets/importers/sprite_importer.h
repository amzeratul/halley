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
		int frameNumber = 0;
		int origFrameNumber = 0;
		int duration = 0;
		String sequenceName;
		String direction;
		Rect4i clip;
		Vector2i pivot;
		Vector4s slices;

		std::unique_ptr<Image> img;
		Vector<String> filenames;
		String origFilename;

		bool operator==(const ImageData& other) const;
		bool operator!=(const ImageData& other) const;

	private:
		friend class SpriteImporter;
		
		bool isDuplicate = false;
		Vector<ImageData*> duplicatesOfThis;
	};

	class SpriteImporter : public IAssetImporter
	{
	public:
		ImportAssetType getType() const override { return ImportAssetType::Sprite; }

		void import(const ImportingAsset& asset, IAssetCollector& collector) override;
		String getAssetId(const Path& file, const std::optional<Metadata>& metadata) const override;

	private:
		Animation generateAnimation(const String& spriteName, const String& spriteSheetName, const String& materialName, const Vector<ImageData>& frameData);

		std::unique_ptr<Image> generateAtlas(const String& atlasName, Vector<ImageData>& images, SpriteSheet& spriteSheet, ConfigNode& spriteInfo);
		std::unique_ptr<Image> makeAtlas(const Vector<BinPackResult>& result, SpriteSheet& spriteSheet, ConfigNode& spriteInfo);
		Vector2i computeAtlasSize(const Vector<BinPackResult>& results) const;
		void markDuplicates(Vector<ImageData>& images) const;

		Vector<ImageData> splitImagesInGrid(const Vector<ImageData>& images, Vector2i grid);
	};
}
