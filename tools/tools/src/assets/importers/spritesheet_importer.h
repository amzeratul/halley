#pragma once
#include "halley/tools/assets/iasset_importer.h"

namespace Halley
{
	class SpriteSheetImporter : public IAssetImporter
	{
	public:
		AssetType getType() const override { return AssetType::SpriteSheet; }

		std::vector<Path> import(const ImportingAsset& asset, Path dstDir, ProgressReporter reporter, AssetCollector collector) override;
	};
}
