#pragma once
#include "halley/tools/assets/iasset_importer.h"
#include <gsl/gsl>

namespace Halley
{
	class ConfigFile;

	class ConfigImporter : public IAssetImporter
	{
	public:
		AssetType getType() const override { return AssetType::Config; }

		std::vector<Path> import(const ImportingAsset& asset, const Path& dstDir, ProgressReporter reporter, AssetCollector collector) override;

		static void parseConfig(ConfigFile& config, gsl::span<const gsl::byte> data);
	};
}
