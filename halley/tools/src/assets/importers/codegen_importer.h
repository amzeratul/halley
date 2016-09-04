#pragma once
#include "halley/tools/assets/iasset_importer.h"

namespace Halley
{
	class CodegenImporter : public IAssetImporter
	{
	public:
		AssetType getType() const override { return AssetType::CODEGEN; }

		String getAssetId(Path file) const override;
		std::vector<Path> import(const ImportAssetsDatabaseEntry& asset, Path dstDir, ProgressReporter reporter) override;
	};
}
