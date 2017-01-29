#pragma once
#include "halley/tools/assets/iasset_importer.h"

namespace Halley
{
	class CopyFileImporter : public IAssetImporter
	{
	public:
		AssetType getType() const override { return AssetType::SimpleCopy; }

		std::vector<Path> import(const ImportingAsset& asset, const Path& dstDir, ProgressReporter reporter, AssetCollector collector) override;
	};
}
