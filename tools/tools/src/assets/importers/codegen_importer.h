#pragma once
#include "halley/tools/assets/iasset_importer.h"

namespace Halley
{
	class CodegenImporter : public IAssetImporter
	{
	public:
		AssetType getType() const override { return AssetType::Codegen; }

		String getAssetId(const Path& file) const override;
		std::vector<Path> import(const ImportingAsset& asset, const Path& dstDir, ProgressReporter reporter, AssetCollector collector) override;
	};
}
