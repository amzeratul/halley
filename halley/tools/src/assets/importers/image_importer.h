#pragma once
#include "halley/tools/assets/iasset_importer.h"

namespace Halley
{
	class ImageImporter : public IAssetImporter
	{
	public:
		AssetType getType() const override { return AssetType::IMAGE; }

		std::vector<Path> import(const ImportAssetsDatabaseEntry& asset, Path dstDir, ProgressReporter reporter) override;
	};
}
