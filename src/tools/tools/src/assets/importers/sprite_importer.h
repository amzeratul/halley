#pragma once
#include "halley/plugin/iasset_importer.h"
#include <gsl/gsl>
#include "halley/file_formats/image.h"
#include "halley/core/graphics/sprite/sprite_sheet.h"
#include "halley/data_structures/bin_pack.h"

namespace Halley
{
	class Animation;

	class SpriteImporter : public IAssetImporter
	{
		struct ImageData
		{
			int frameNumber;
			int duration;
			String sequenceName;
			String filename;
			std::unique_ptr<Image> img;
			Rect4i clip;
			Vector2i pivot;
		};

	public:
		ImportAssetType getType() const override { return ImportAssetType::Sprite; }

		void import(const ImportingAsset& asset, IAssetCollector& collector) override;

	private:
		std::vector<ImageData> importAseprite(String baseName, gsl::span<const gsl::byte> fileData, Vector2i pivot);
		std::vector<ImageData> loadImagesFromPath(Path tmp, Vector2i pivot);
		std::map<int, int> getSpriteDurations(Path jsonPath);
		void processFrameData(String baseName, std::vector<SpriteImporter::ImageData>& frameData, std::map<int, int> durations);

		Animation generateAnimation(const String& spriteName, const String& atlasName, const String& materialName, const std::vector<ImageData>& data);

		std::unique_ptr<Image> generateAtlas(const String& assetName, std::vector<ImageData>& images, SpriteSheet& spriteSheet);
		std::unique_ptr<Image> makeAtlas(const String& assetName, const std::vector<BinPackResult>& pack, Vector2i size, SpriteSheet& spriteSheet);

		std::vector<ImageData> splitImagesInGrid(const std::vector<ImageData>& images, Vector2i grid);
	};
}
