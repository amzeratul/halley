#pragma once
#include "halley/plugin/iasset_importer.h"
#include <gsl/gsl>
#include "halley/file_formats/image.h"
#include "halley/core/graphics/sprite/sprite_sheet.h"
#include "halley/data_structures/bin_pack.h"

namespace Halley
{
	class Animation;

	class AsepriteImporter : public IAssetImporter
	{
		struct ImageData
		{
			int frameNumber;
			int duration;
			String sequenceName;
			String filename;
			std::unique_ptr<Image> img;
			Rect4i clip;
		};

	public:
		ImportAssetType getType() const override { return ImportAssetType::Aseprite; }

		void import(const ImportingAsset& asset, IAssetCollector& collector) override;

	private:
		std::vector<ImageData> importAseprite(String baseName, gsl::span<const gsl::byte> fileData);
		std::vector<ImageData> loadImagesFromPath(Path tmp);
		std::map<int, int> getSpriteDurations(Path jsonPath);
		void processFrameData(String baseName, std::vector<AsepriteImporter::ImageData>& frameData, std::map<int, int> durations);

		Animation generateAnimation(String baseName, Path spriteSheetPath, const std::vector<ImageData>& data);

		std::unique_ptr<Image> generateAtlas(Path imageName, std::vector<ImageData>& images, SpriteSheet& spriteSheet, Vector2i pivot);
		std::unique_ptr<Image> makeAtlas(Path imageName, const std::vector<BinPackResult>& pack, Vector2i size, SpriteSheet& spriteSheet, Vector2i pivot);

		std::vector<ImageData> splitImagesInGrid(const std::vector<ImageData>& images, Vector2i grid);
	};
}
