#pragma once
#include "halley/tools/assets/iasset_importer.h"
#include <gsl/gsl>
#include "halley/file_formats/image.h"
#include "halley/core/graphics/sprite/sprite_sheet.h"

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
		Animation generateAnimation(String baseName, const std::vector<ImageData>& data);
		void generateAtlas(String baseName, const std::vector<ImageData>& images, Image& atlasImage, SpriteSheet& spriteSheet);
	};
}
