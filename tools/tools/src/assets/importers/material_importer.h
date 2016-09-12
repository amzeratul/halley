#pragma once
#include "halley/tools/assets/iasset_importer.h"

namespace Halley
{
	class MaterialDefinition;

	class MaterialImporter : public IAssetImporter
	{
	public:
		AssetType getType() const override { return AssetType::MATERIAL; }

		std::vector<Path> import(const ImportingAsset& asset, Path dstDir, ProgressReporter reporter, AssetCollector collector) override;

		static void parseAnimation(MaterialDefinition& animation, gsl::span<const gsl::byte> data);
	};
}
