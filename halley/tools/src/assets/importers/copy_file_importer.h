#pragma once
#include "halley/tools/assets/iasset_importer.h"

namespace Halley
{
	class CopyFileImporter : public IAssetImporter
	{
	public:
		AssetType getType() const override { return AssetType::SIMPLE_COPY; }

		std::vector<Path> import(const ImportingAsset& asset, Path dstDir, ProgressReporter reporter, AssetCollector collector) override;
	};
}
