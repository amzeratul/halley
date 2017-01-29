#pragma once
#include "halley/plugin/iasset_importer.h"

namespace Halley
{
	class FontImporter : public IAssetImporter
	{
	public:
		AssetType getType() const override { return AssetType::Font; }

		std::vector<Path> import(const ImportingAsset& asset, const Path& dstDir, ProgressReporter reporter, AssetCollector collector) override;
	};
}
