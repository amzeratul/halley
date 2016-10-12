#pragma once
#include "halley/tools/assets/iasset_importer.h"
#include <gsl/gsl>
#include "halley/file_formats/image.h"

namespace Halley
{
	class Animation;

	class AsepriteImporter : public IAssetImporter
	{
	public:
		AssetType getType() const override { return AssetType::Aseprite; }

		std::vector<Path> import(const ImportingAsset& asset, Path dstDir, ProgressReporter reporter, AssetCollector collector) override;

	private:
		void importAseprite(String baseName, gsl::span<const gsl::byte> fileData, std::vector<std::unique_ptr<Image>>& frames, Animation& spriteSheet);
	};
}
