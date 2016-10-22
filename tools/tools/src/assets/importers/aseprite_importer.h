#pragma once
#include "halley/tools/assets/iasset_importer.h"
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
		};

	public:
		AssetType getType() const override { return AssetType::Aseprite; }

		std::vector<Path> import(const ImportingAsset& asset, Path dstDir, ProgressReporter reporter, AssetCollector collector) override;

	private:
		std::vector<ImageData> importAseprite(String baseName, gsl::span<const gsl::byte> fileData);
		std::vector<ImageData> loadImagesFromPath(Path tmp);
		std::map<int, int> getSpriteDurations(Path jsonPath);
		void processFrameData(String baseName, std::vector<AsepriteImporter::ImageData>& frameData, std::map<int, int> durations);

		Animation generateAnimation(String baseName, const std::vector<ImageData>& data);

		std::unique_ptr<Image> generateAtlas(String baseName, std::vector<ImageData>& images, SpriteSheet& spriteSheet);
		std::unique_ptr<Image> makeAtlas(String baseName, const std::vector<BinPackResult>& pack, Vector2i size, SpriteSheet& spriteSheet);

		std::vector<ImageData> splitImagesInGrid(const std::vector<ImageData>& images, Vector2i grid);
	};
}
